/**
 * file   : server.hpp
 * author : cypro666
 * date   : 2015.01.30
 * asynchronous http server implement
 */
#pragma once
#ifndef SERVER_HTTP_HPP
#define SERVER_HTTP_HPP
#include <cstdlib>
#include "serverbase.hpp"
#include "rescache.hpp"
#include <boost/asio/ssl.hpp>
#include <boost/noncopyable.hpp>

namespace basiohttp
{

template<class socket_type>
struct server: public server_base<socket_type>
{
    //nothing to do...
};


template<>
struct server<asio_http>: public server_base<asio_http>
{
    server(const ipv4_address addr,
           const size_t port,
           const size_t num_threads = 1,
           const size_t request_timeout = 5,
           const size_t connect_timeout = 300):
        server_base<asio_http>::server_base(addr, port, num_threads, request_timeout, connect_timeout)
    {
    }

    response_cache<>& cache(void)
    {
        return __rescache;
    }

protected:
    response_cache<> __rescache;

    void accept(void)
    {
        //create new socket for this connection
        //shared_ptr is used to pass temporary objects to the asynchronous functions
        boost::shared_ptr<asio_http> asocket(new asio_http(__ioservice));

        __acceptor.async_accept(*asocket,
            [this, asocket](const error_code& ec) {
                accept();   //immediately start accepting a new connection
                if(!ec) {
                    read_request_and_content(asocket);
                }
            }
        );

    }
};



template<>
struct server<asio_https>: public server_base<asio_https>
{
    server(const ipv4_address addr,
           const size_t port,
           const size_t num_threads,
           const std::string& cert_file,
           const std::string& private_key_file,
           size_t timeout_request = 5,
           size_t timeout_content = 300,
           const std::string& verify_file = std::string()):
       /* different from server<asio_http> */
       server_base<asio_https>::server_base(addr, port, num_threads, timeout_request, timeout_content),
       context(boost::asio::ssl::context::sslv23)
    {
        context.use_certificate_chain_file(cert_file);
        context.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
        if (verify_file.size() > 0) {
            context.load_verify_file(verify_file);
        }
    }

    boost::asio::ssl::context context;

    void accept(void)
    {
        //Create new socket for this connection
        //Shared_ptr is used to pass temporary objects to the asynchronous functions
        boost::shared_ptr<asio_https> socket(new asio_https(__ioservice, context));

        __acceptor.async_accept((*socket).lowest_layer(),
            [this, socket](const error_code& ec) {
                accept();
                if (!ec) {
                    //Set timeout on the following boost::asio::ssl::stream::async_handshake
                    async_timer_ptr timer;
                    if (__req_timeout > 0) {
                        timer = set_timeout_on_socket(socket, __req_timeout);
                    }
                    (*socket).async_handshake(boost::asio::ssl::stream_base::server,
                        [this, socket, timer] (const error_code& ec) {
                            if (__req_timeout > 0) {
                                timer->cancel();
                                }
                            if(!ec) {
                                read_request_and_content(socket);
                            }
                        }
                    );
                }
            }
        );
    }

};

}


#endif  /* SERVER_HTTP_HPP */
