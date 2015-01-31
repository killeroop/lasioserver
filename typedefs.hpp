/**
 * this head should be included by all hpp file in basiohttp namespace
 */
#pragma once
#ifndef _TYPEDEFS_HTTP_HPP_
#define _TYPEDEFS_HTTP_HPP_
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/function.hpp>


namespace basiohttp
{
using std::string;
using std::istream;
using std::ostream;

typedef boost::system::error_code  error_code;

typedef boost::asio::ip::tcp::socket  asio_http;
typedef boost::asio::ip::tcp::socket  asio_socket;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>  asio_https;

typedef boost::asio::ip::address     ip_address;
typedef boost::asio::ip::address_v4  ipv4_address;
typedef boost::asio::ip::address_v6  ipv6_address;

typedef boost::asio::ip::tcp::endpoint  asio_endpoint;
typedef boost::asio::ip::tcp::acceptor  asio_acceptor;
typedef boost::asio::ip::tcp::resolver  asio_resolver;

typedef boost::asio::io_service  asio_service;

typedef boost::asio::signal_set  asio_signals;

typedef boost::shared_ptr<asio_service> asio_service_ptr;
typedef boost::shared_ptr<boost::asio::deadline_timer>  async_timer_ptr;
typedef boost::shared_ptr<boost::asio::streambuf>  streambuf_ptr;

typedef boost::unordered_map<string, boost::regex>  regex_dict;

struct _request //for server
{
    string  path;
    string  method;
    string  version;
    string  match1;
    string  match2;
    string  match3;
    string  address;
    istream content;
    boost::asio::streambuf content_buffer;
    boost::unordered_map<string, string> header;

    _request(void):content(&content_buffer)
    {
    }

};

typedef boost::shared_ptr<_request>  request_ptr;

typedef boost::function<void(streambuf_ptr, request_ptr)>  handler_for_server; //for server
typedef boost::unordered_map<string, std::map<string, handler_for_server>> logical_dict;  //for server

struct _response
{
    string head;    //http head
    string content; //http body
    uint32_t crc_content;
    _response(void):crc_content(0)
    {
    }
};

typedef boost::shared_ptr<_response>  response_ptr;

typedef boost::function<void(string, response_ptr)> handler_for_client; //for client



}//basiohttp


#endif//_TYPEDEFS_HTTP_HPP_
