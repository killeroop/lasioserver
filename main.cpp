/**
 * file   : main.cpp
 * author : cypro666
 * date   : 2015.01.30
 * driver of basioserver
 */
#include <csignal>
#include <fstream>
#include <vector>
#include <iterator>
#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>
#include "fileio.hpp"
#include "reply.hpp"
#include "client.hpp"
#include "server.hpp"
#include "crc.hpp"



int main(int argc, char* argv[])
{
    using namespace std;
    using namespace boost;
    using namespace basiohttp;
    std::ios::sync_with_stdio(false);

    ipv4_address addr;
    addr.from_string("0.0.0.0");
    server<asio_http> webserver1(addr, 8888, 16, 30, 300);

    test_client("www.baidu.com");

    auto sighandler = [](const error_code& err, const int& sig) {
        std::cerr << " catch signal " << sig << "\n";
        exit(sig);
    };

    webserver1.set_signal_handler(SIGINT, sighandler);
    webserver1.set_signal_handler(SIGQUIT, sighandler);

    auto& cache1 = webserver1.cache();

    auto post_specific = [](streambuf_ptr resbuf, request_ptr r) {
        ostream response(resbuf.get());
        stringstream ss;
        r->content >> ss.rdbuf();
        string content = ss.str();
        response << sreplace(templates::ok, {"$length", dtos(content.size())}, {"$content", content});
    };

    auto get_default1 = [&cache1](streambuf_ptr resbuf, request_ptr r) {
        std::cerr << r->address << " : " << r->path << "\n";
        ostream response(resbuf.get());
        string filename = "web/";
        string path = r->match1;
        size_t pos = 0;
        while((pos = path.find("..")) != string::npos) {
            path.erase(pos, 1);
        }
        filename += path;
        if (filename.find('.') == string::npos) {
            if(*filename.rbegin() != '/') {
                filename += '/';
            }
            filename += "index.html";
        }
        if (not file_check(filename)) {
            const string ct = "<html><h1>404 Not Found</h1>\n<h3>Your IP: " + r->address + "</h3></html>";
            response << sreplace(templates::not_found, {"$content", ct}, {"$length", dtos(ct.size())});
            return;
        }
        auto pres = cache1.get(filename);
        if (pres) {
            response << pres->head << pres->content;
        }
        else {
            mmap_reader mr(filename.c_str());
            auto buf = mr.read();
            string ct = sreplace(templates::ok, {"$length", dtos(mr.size())}, {"$content", buf});
            response << ct;
            response_ptr pres(new _response);
            auto range = boost::find_first(ct, "\r\n\r\n");
            pres->head.assign(ct.begin(), range.end());
            pres->content.assign(range.end(), ct.end());
            cache1.set(filename, pres);
        }
    };

    webserver1.set_specific_logical("^/?(.*)$", "POST", post_specific);
    webserver1.set_default_logical("^/?123(.*)$", "GET", get_default1);

    boost::thread server_thread1( [&webserver1](){webserver1.start();} );

    boost::this_thread::sleep_for(boost::chrono::seconds(10000000));

    server_thread1.join();

    return 0;
}








