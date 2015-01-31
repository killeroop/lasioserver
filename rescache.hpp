/**
 * file   : rescache.hpp
 * author : cypro666
 * date   : 2015.01.30
 * multi-thread safe cache for server
 */
#pragma once
#include <string>
#include <map>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include "typedefs.hpp"

namespace basiohttp
{
using std::string;

struct atom_lock
{
    volatile boost::atomic_flag __flag;

    atom_lock(void) = default;

    inline void lock(void)
    {
        while (__flag.test_and_set(boost::memory_order_acquire)) {
            ;
        }
    }

    inline void unlock(void)
    {
        __flag.clear(boost::memory_order_release);
    }

};


template<typename LockType = boost::shared_mutex>
struct response_cache
{
    typedef boost::shared_lock<LockType> reads_lock;
    typedef boost::unique_lock<LockType> write_lock;
    typedef boost::unordered_map<string, response_ptr> cache_type;

    response_cache(void)
    {
    }

    bool set(const string& key, response_ptr res)
    {
        write_lock lock(__mutex);
        auto iter = __cache.find(key);

        if (iter != __cache.end()) {
            iter->second = res;
            return true;
        }
        else {
            __cache.insert({key, res});
        }
        return false;
    }

    response_ptr get(const string& key)
    {
        reads_lock lock(__mutex);
        response_ptr ret;

        auto iter = __cache.find(key);
        if (iter != __cache.end()) {
            ret = iter->second;
        }

        return ret;
    }

    LockType __mutex;
    cache_type __cache;
};


}



