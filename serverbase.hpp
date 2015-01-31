/**
 * file   : serverbase.hpp
 * author : cypro666
 * date   : 2015.01.30
 * asynchronous http server implement
 */
#pragma once
#ifndef SERVER_BASE_HTTP_HPP
#define SERVER_BASE_HTTP_HPP
#include <cassert>
#include <cstdlib>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "utils.hpp"
#include "typedefs.hpp"
#include "log.hpp"

namespace basiohttp
{

const string DEFAULT_LOG_FILE = "bas.log";
const size_t MAX_THREADS = 64;

template<typename socket_type>
struct server_base
{
    typedef boost::shared_ptr<socket_type> socket_type_ptr;

    server_base(const ipv4_address addrv4,
                /* IPv4 address of server, do not use IPv6 */
                const size_t port,
                /* tcp port of http service, using 80 or 8090 or 8888 */
                const size_t num_threads,
                /* max threads number of server, not be larger than MAX_THREADS */
                const size_t req_timeout,
                /* requests timeout in seconds, should be more than 3 */
                const size_t timeout_send_or_receive)
                /* response timeout for communicate with clients */
    try:
        __endpoint(addrv4, port),
        __acceptor(__ioservice, __endpoint),
        __sigset(__ioservice),
        __num_threads(min(num_threads, MAX_THREADS)),
        __req_timeout(req_timeout),
        __con_timeout(timeout_send_or_receive),
        __loger(DEFAULT_LOG_FILE, 512)
    {
        boost::regex::flag_type flag = boost::regex::perl|boost::regex::optimize;
        __xprot.assign("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$", flag);
        __xhead.assign("^([^:]*): ?(.*)$", flag);
    }
    catch (const std::exception& e) {
        __loger.commit(__func__, e.what(), "ERROR");
        __loger.flush();
    }

    // the only method should be implemented by sub classes
    virtual void accept(void) = 0;

    // for client using...
    inline asio_service& get_io_service(void)
    {
        return __ioservice;
    }

    //Func like: void handler(const error_code& error, int signal_number);
    template<typename Func>
    bool set_signal_handler(const char signum, Func handler)
    {
        try {
            __sigset.remove(signum);
            __sigset.add(signum);
            __sigset.async_wait(handler);
        }
        catch (const std::exception& e) {
            __loger.commit(__func__, e.what(), "ERROR");
            return false;
        }
        __loger.commit(__func__, "signal "+dtos((size_t)signum)+" registed!");
        return true;
    }

    // handler_for_server like void fun(streambuf_ptr, request_ptr)
    // which will be callbacked in asnyc routines
    // note streambuf_ptr should be wrapped into ostream in handler_for_server
    bool set_specific_logical(const string& sre, const string& method, const handler_for_server& rh)
    {
        auto flag = boost::regex::perl|boost::regex::optimize;
        try {
            __user_logical[sre][method] = rh;
            __sredict[sre] = boost::regex(sre, flag);
        }
        catch (const std::exception& e) {
            __loger.commit(__func__, e.what(), "ERROR");
            return false;
        }
        __loger.commit(__func__, method+" "+sre+" registed!");
        return true;
    }

    // same as set_specific_logical, but priority level is lower than set_specific_logical
    bool set_default_logical(const string& sre, const string& method, const handler_for_server& rh)
    {
        auto flag = boost::regex::perl|boost::regex::optimize;
        try {
            __default_logical[sre][method] = rh;
            __sredict[sre] = boost::regex(sre, flag);
        }
        catch (const std::exception& e) {
            __loger.commit(__func__, e.what(), "ERROR");
            return false;
        }
        __loger.commit(__func__, method+" "+sre+" registed!");
        return true;
    }

    // run http server forever!
    void start(void)
    {
        __logical_list.clear();

        for (auto it = __user_logical.begin(); it != __user_logical.end(); ++it) {
            __logical_list.push_back(it);
        }
        for (auto it = __default_logical.begin(); it != __default_logical.end(); ++it) {
            __logical_list.push_back(it);
        }

        this->accept(); //impl by sub class

        __threads.clear();
        for (size_t c = 1; c < __num_threads; ++c) {
            __threads.emplace_back( [this](){__ioservice.run();} ); //not push_back
        }

        __loger.commit(__func__, "server, number threads: "+dtos(__num_threads));
        __loger.flush();

        __ioservice.run();

        for (auto& t : __threads) {
            t.join();
        }
    }

    // just a tag...
    void stop(void)
    {
        __ioservice.stop();
        __loger.flush();
    }

    // internal using
    async_timer_ptr set_timeout_on_socket(socket_type_ptr socket, const size_t seconds)
    {
        assert(seconds);
        async_timer_ptr timer(new boost::asio::deadline_timer(__ioservice));
        timer->expires_from_now(boost::posix_time::seconds(seconds));
        timer->async_wait([this,socket](const error_code& ec) {
            if(not ec) {
                this->__loger.commit(__func__, "time out!");
                socket->lowest_layer().shutdown(asio_socket::shutdown_both);
                socket->lowest_layer().close();
            } else {
                //this->__loger.commit(__func__, ec.message(), "ERROR");
            }
        });
        return timer;
    }

    void read_request_and_content(socket_type_ptr socket)
    {
        request_ptr r(new _request());
        async_timer_ptr timer;

        if (__req_timeout > 0) {
            timer = set_timeout_on_socket(socket, __req_timeout);
        }

        auto read_header_callback = [this,timer,r,socket](const error_code& ec, size_t bytes_transferred) {
            if (__req_timeout > 0) {
                timer->cancel();
            }
            if (not ec) {
                // r->streambuf.size() is not necessarily the same as bytes_transferred, so calc it!
                auto num_additional_bytes = r->content_buffer.size() - bytes_transferred;

                this->parse_request(r, r->content);

                // if content, read that as well
                if (r->header.count("Content-Length") > 0) {
                    // set timeout on the following boost::asio::async-read or write function
                    async_timer_ptr next_timer;
                    if (__con_timeout > 0) {
                        next_timer = this->set_timeout_on_socket(socket, __con_timeout);
                    }

                    auto read_body_callback = [this,next_timer,r,socket](const error_code& ec, size_t bytes_transferred) {
                        if (__con_timeout > 0) {
                            next_timer->cancel();
                        }
                        if (not ec) {
                            this->write_response(socket, r);
                        }
                    };

                    auto ctsize = stod<size_t>(r->header["Content-Length"]);
                    auto ntrans = boost::asio::transfer_exactly(ctsize - num_additional_bytes);
                    boost::asio::async_read(*socket, r->content_buffer, ntrans, read_body_callback);
                }
                else {
                    this->write_response(socket, r);
                }
            }
        };

        boost::asio::async_read_until(*socket, r->content_buffer, "\r\n\r\n", read_header_callback);
    }


    // istream is very slow now..., better ideas?
    void parse_request(request_ptr r, istream& stream) const
    {
        //first parse r->method, path, and HTTP-version from the first line
        string line;
        boost::smatch match;
        getline(stream, line);
        boost::trim_right(line);

        if (boost::regex_match(line, match, __xprot)) {
            r->method  = match[1];
            r->path    = match[2];
            r->version = match[3];
            bool matched = false;

            do {
                getline(stream, line);
                boost::trim_right(line);
                matched = boost::regex_match(line, match, __xhead);
                if (matched) {
                    r->header[match[1]] = match[2];
                }
            } while (matched);
        }
    }

    // for ipv4
    inline string remote_addr(const asio_http& sock)
    {
        return sock.remote_endpoint().address().to_string();
    }

    // for ipv6
    inline string remote_addr(const asio_https& ssock)
    {
        return ssock.next_layer().remote_endpoint().address().to_string();
    }


    inline bool valid_request(const request_ptr& req, boost::smatch& matched, handler_for_server& handler)
    {
        auto iter = std::find_if(__logical_list.begin(), __logical_list.end(),
            [this, req, &matched, &handler](const logical_dict::iterator& _iter){
                auto sre  = _iter->first;
                auto mhd = _iter->second;
                if (boost::regex_match(req->path, matched, __sredict[sre])) {
                    if (mhd.count(req->method)) {
                        handler = mhd[req->method];
                        return true;
                    }
                }
                return false;
            }
        );

        if (iter != __logical_list.end()) {
            __loger.commit(__func__, "got valid request: "+req->path);
            return true;
        }
        return false;
    }

    //finally, write response to client, buf if http1.1...
    void write_response(socket_type_ptr socket, request_ptr r)
    {
        boost::smatch matched;
        handler_for_server user_handler;
        streambuf_ptr buffer(new boost::asio::streambuf);
        async_timer_ptr timer;

        if (__con_timeout > 0) {
            timer = set_timeout_on_socket(socket, __con_timeout);
        }

        //check path and method, get right handler and matched in logical_dict
        bool valid = valid_request(r, matched, user_handler);

        r->address = remote_addr(*socket);
        if (valid) {
            r->match1.assign(matched[1]);
            r->match2.assign(matched[2]);
            r->match3.assign(matched[3]);
            user_handler(buffer, r);
        } else {
            ostream response(buffer.get());
            response << templates::bad_request;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////
        // Be careful: must capture buffer in lambda so it is not destroyed before async_write is finished!
        ///////////////////////////////////////////////////////////////////////////////////////////////////
        auto write_callback = [this,timer,r,buffer,socket](const error_code& ec, size_t bytes_transferred) {
            if (__con_timeout > 0) {
                timer->cancel();
            }
            //if http 1.1 persistent connection
            if (not ec and r->version != "1.0") {
                this->read_request_and_content(socket); //more async operations
            }
        };

        boost::asio::async_write(*socket, *buffer, write_callback);
    }

    // see typedefs.hpp for more details
    logical_dict __user_logical;
    logical_dict __default_logical;

    asio_service  __ioservice;
    asio_endpoint __endpoint;
    asio_acceptor __acceptor;
    asio_signals  __sigset;

    size_t __num_threads;
    std::vector<boost::thread> __threads;

    size_t __req_timeout;
    size_t __con_timeout;

    std::vector<typename logical_dict::iterator> __logical_list;
    regex_dict __sredict;

    boost::regex __xprot; //("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    boost::regex __xhead; //("^([^:]*): ?(.*)$");

    buffered_logger<> __loger;

};


}//basiohttp



#endif  /* SERVER_BASE_HTTP_HPP */




