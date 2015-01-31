/**
 * file   : client.hpp
 * author : cypro666
 * date   : 2015.01.30
 * asynchronous http client implement
 */
#pragma once
#ifndef SIMPLE_CLIENT_HTTP_HPP
#define SIMPLE_CLIENT_HTTP_HPP
#include <string>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/atomic.hpp>
#include "utils.hpp"
#include "typedefs.hpp"

namespace basiohttp
{

struct client
{
    client(void) = delete; //no default constructor

    client(asio_service& io_service,
           handler_for_client& response_handler,
           const string& host,
           const string& path,
           const string method = "GET") //only support GET and HEAD
    try :
        __url("http://" + host + path),
        __socket(io_service),
        __resolver(io_service),
        __data(new _response),
        __timer(new boost::asio::deadline_timer(io_service)),
        __user_handler(response_handler),
        __finished(false)
    {
        using boost::replace_first_copy;
        // connection callback to each endpoint here until we successfully establish a connection.
        auto resolve_cb = [this](const error_code& err, asio_resolver::iterator endpoint_iterator){
            if (not err) {
               boost::asio::async_connect(this->__socket, endpoint_iterator,
                   [this](const error_code& _err, asio_resolver::iterator endpoint_iterator){
                       this->handle_connect(_err);
                   }
               );
           }
           else {
               std::cerr << "Error: " << err.message() << "\n";
               this->__socket.lowest_layer().shutdown(asio_socket::shutdown_both);
               this->__socket.lowest_layer().close();
           }
        };

        asio_resolver::query query(host, "http");
        ostream reqos(&__reqbuf);

        if (method == "GET") {
            reqos << sreplace(templates::get, {"$path", path}, {"$host", host});
        }
        else if (method == "HEAD") {
            reqos << sreplace(templates::head, {"$path", path}, {"$host", host});
        }
        else if (method == "POST") {
            throw std::runtime_error("POST not support now!");
        }
        else  {
            throw std::runtime_error("method not correct!");
        }
        __resolver.async_resolve(query, resolve_cb);
    }
    catch (const std::exception& e) {
        std::cout << __func__ << "Exception: " << e.what() << "\n";
    }

    ~client(void)
    {
        std::cerr << "client released: " << __url << "\n";
    }

    void fetch(void)
    {
    }

    void handle_connect(const error_code& err)
    {
        if (not err) {
            // the connection was successful, send the request.
            boost::asio::async_write(__socket, __reqbuf,
                [this](const error_code& _err, const size_t& _nbytes) {
                    this->handle_write_request(_err, _nbytes, this->__timer);
                }
            );
        }
        else {
            std::cerr << __func__ << " Error : " << err.message() << "\n";
        }
    }


    void handle_write_request(const error_code& err, const size_t& nbytes, async_timer_ptr timer)
    {
        if (not err) {
            // read the response status line. The __resbuf will automatically grow to accommodate the entire line.
            // the growth may be limited by passing a maximum size to the streambuf constructor.
            boost::asio::async_read_until(__socket, __resbuf, "\r\n",
                [this](const error_code& _err, const size_t& _nbytes){
                    this->handle_read_status_line(_err, _nbytes);
                }
            );
        }
        else {
            std::cerr << __func__ << " Error: " << err.message() << "\n";
        }
    }


    void handle_read_status_line(const error_code& err, const size_t& nbytes)
    {
        if (not err) { //get http head means read until "\r\n\r\n"
            istream response_stream(&__resbuf);
            string  http_version;
            size_t  status_code;
            string  status_message;
            response_stream >> http_version;
            response_stream >> status_code;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                std::cout << "Invalid response\n";
                return; // check that response is OK.
            }
            if (status_code != 200) {
                std::cout << "Response returned with status code ";
                std::cout << status_code << "\n";
                return;
            }
            // read the response headers, which are terminated by a blank line.
            boost::asio::async_read_until(__socket, __resbuf, "\r\n\r\n",
                [this](const error_code& _err, const size_t& _nbytes){
                    this->handle_read_headers(_err, _nbytes);
                }
            );
        }
        else {
            std::cerr << __func__ << " Error: " << err << "\n";
        }
    }

    inline boost::asio::detail::transfer_at_least_t __transctl(const size_t n = 1)
    {
        //same as "boost::asio::transfer_at_least"
        return boost::asio::detail::transfer_at_least_t(n);
    }

    void handle_read_headers(const error_code& err, const size_t& nbytes)
    {
        // start reading remaining data until eof.
        if (not err) {
            streambuf2cstr(__data->head, __resbuf, nbytes);
            boost::erase_last(__data->head, "\r\n");
            boost::asio::async_read(__socket, __resbuf, __transctl(),
                [this](const error_code& _err, const size_t& _nbytes){
                    this->handle_read_content(_err, _nbytes);
                }
            );
        }
        else {
            std::cerr << __func__ << " Error: " << err << "\n";
        }
    }


    void handle_read_content(const error_code& err, const size_t& nbytes)
    {
        if (not err) {
            // write all of the data that has been read so far, continue reading remaining data until eof
            boost::asio::async_read(__socket, __resbuf, __transctl(), boost::bind(&client::handle_read_content,
                                                                                  this,
                                                                                  boost::asio::placeholders::error,
                                                                                  _2));
            // note here should NOT use lambda expr!!!
        }
        else if (err == boost::asio::error::eof) {
            streambuf2cstr(__data->content, __resbuf, __resbuf.size());
            __user_handler(__url, __data);
            __finished.store(true, boost::memory_order_seq_cst);
        }
        else {
            std::cerr << __func__ << " Error: " << err << "\n";
        }

    }

    // see typedefs.hpp for more details
    const string  __url;
    asio_socket   __socket;
    asio_resolver __resolver;

    boost::asio::streambuf __reqbuf;
    boost::asio::streambuf __resbuf;

    response_ptr    __data;
    async_timer_ptr __timer;

    handler_for_client __user_handler;
    boost::atomics::atomic_bool __finished;

};


typedef boost::shared_ptr<client> client_ptr;

inline client_ptr create_client(asio_service& io_service,
                                const string& url,
                                const string& method,
                                handler_for_client handler)
{
    static boost::regex x("^(http://)?(\\w+\\.\\w.*?)(/.*)?$");
    boost::smatch match;
    boost::regex_match(url, match, x);
    string prot(match[1]);
    string host(match[2]);
    string path(match[3]);
    if (path.empty()) {
        path = "/";
    }
    return boost::make_shared<client>(io_service, handler, host, path, method);
}


inline int test_client(string url)
{
    try {
        asio_service io_service;
        auto ptr = create_client(io_service, url, "GET", [](const string& url, response_ptr data){
            boost::erase_all(data->head, "\r");
            std::cout << url << "\n" << data->head << "\ncontent size: " << data->content.size() << "\n";
            std::cout.flush();
        });
        io_service.run();
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

}//basiohttp


#endif//SIMPLE_CLIENT_HTTP_HPP

