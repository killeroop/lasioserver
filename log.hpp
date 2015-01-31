/**
 * file   : log.hpp
 * author : cypro666
 * date   : 2015.01.30
 * a simple buffered logger
 */
#pragma once
#ifndef SIMPLE_HTTP_LOGGER_HPP
#define SIMPLE_HTTP_LOGGER_HPP
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>
// #include <boost/interprocess/sync/file_lock.hpp>
// typedef boost::interprocess::file_lock file_lock;

#include "typedefs.hpp"


namespace basiohttp
{
using std::string;

const size_t MAX_LOGQ_SIZE = 512;
const size_t MIN_LOGQ_SIZE = 128;


template<typename StreamType = ofstream_ptr>
struct buffered_logger: public boost::noncopyable
{
    typedef boost::recursive_mutex::scoped_lock scoped_lock;

    buffered_logger(void) = delete;

    buffered_logger(const string logfile, const size_t qsize):
        /**
         * `qsize` is the max size of logqueue, when logqueue is full, logger will dump them to disk...
         */
        __squeue(min(max(MIN_LOGQ_SIZE, qsize), MAX_LOGQ_SIZE)),
        __curpos(0),
        logstream(make_ofstream())
    {
        for (size_t i = 0; i < qsize; ++i) {
            __squeue.push_back(string());
        }

        logstream->open(logfile, std::ios::out|std::ios::app);

        if (not logstream->is_open() or logstream->bad()) {
            throw(std::runtime_error("logging failed!\n"));
        }

        for (auto& s : __squeue) {
            s.reserve(1024); //every line in log should not longer than this value!
        }
    }

    ~buffered_logger(void) = default;

    inline void flush(void)
    {
        scoped_lock lock(__mutex);
        std::for_each(__squeue.begin(), __squeue.end(), [this](const string& s) {
            *logstream << s;
        });

        logstream->flush();
        __squeue.clear();
    }


    template<typename S1, typename S2, typename S3 = string>
    inline void commit(const S1& loc, const S2& str, const S3 level = "INFO")
    {
        __commit(getisotime() + " " + level + " " + loc + " " + str + "\n");
    }

    void __commit(const string& line)
    {
        scoped_lock lock(__mutex);
        if (__squeue.full()) {
            logstream->flush();

            std::for_each(__squeue.begin(), __squeue.end(),
                [this](const string& s){
                    *logstream << s;
                }
            );

            logstream->flush();
            __squeue.clear();
        }

        __squeue.push_back(line);
    }


    boost::circular_buffer<string> __squeue;
    boost::recursive_mutex __mutex;
    size_t __curpos;

    StreamType logstream;

};


}//basiohttp


#endif//SIMPLE_HTTP_LOGGER_HPP



