#pragma once
#ifndef HTTP_REPLY_HPP
#define HTTP_REPLY_HPP
#include <string>
#include <vector>
#include <boost/asio.hpp>

namespace basiohttp
{

using std::string;
// Convert a file extension into a MIME type.
struct mimetypes
{
  const char* extension;
  const char* mime_type;
};

static const mimetypes mappings[] =
{
    { "gif",  "image/gif" },
    { "htm",  "text/html" },
    { "html", "text/html" },
    { "jpg",  "image/jpeg" },
    { "png",  "image/png" }
};


inline string extension_to_type(const string& extension)
{
    for (auto& m : mappings) {
        if (m.extension == extension) {
            return m.mime_type;
        }
    }
    return "text/plain";
}



enum STATUS_TYPE
{
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
};


namespace templates
{
const string ok = "HTTP/1.1 200 OK\r\n"
                  "Connection: Keep-Alive\r\n"
                  "Content-Type: text/html; charset=utf-8\r\n"
                  "Content-Length: $length"
                  "\r\n\r\n"
                  "$content";

const string not_found = "HTTP/1.0 404 Not Found\r\n"
                         "Connection: Close\r\n"
                         "Content-Length: $length"
                         "\r\n\r\n"
                         "$content";

const string get  = "GET $path HTTP/1.0\r\n"
                    "Host: $host \r\n"
                    "Accept: text/html,application/xhtml+xml\r\n"
                    "User-Agent: Mozilla/5.0 (Linux x86_64) Gecko Firefox\r\n"
                    "Connection: close\r\n\r\n";

const string head = "HEAD $path HTTP/1.0\r\n"
                    "Host: $host \r\n"
                    "User-Agent: Mozilla/5.0 (Linux x86_64) Gecko Firefox\r\n"
                    "Connection: close\r\n\r\n";

const string bad_request = "HTTP/1.0 400 Bad Request\r\n"
                           "Connection: Close\r\n"
                           "Content-Length: 24"
                           "\r\n\r\n"
                           "<html>Bad Request</html>";





} //templates


} //basiohttp


#endif//HTTP_REPLY_HPP


