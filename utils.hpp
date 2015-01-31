/**
 * file   : utils.hpp
 * author : cypro666
 * date   : 2015.01.30
 * utility functions and auxes
 */
#pragma once
#ifndef SIMPLE_HTTP_UTIL_HPP
#define SIMPLE_HTTP_UTIL_HPP
#include <string>
#include <ctime>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread.hpp>
#include "typedefs.hpp"

namespace basiohttp
{

using std::string;
using boost::replace_all_copy;

typedef std::pair<string, string> placepair;

//// template and string algos ///////////////////////////////////////////////////////////////////
inline string sreplace(const string& tpl, placepair a)
{
    return replace_all_copy(tpl, a.first, a.second);
}

inline string sreplace(const string& tpl, placepair a1, placepair a2)
{
    return sreplace(sreplace(tpl, a1), a2);
}

inline string sreplace(const string& tpl, placepair a1, placepair a2, placepair a3)
{
    return sreplace(sreplace(tpl, a1, a2), a3);
}

inline string sreplace(const string& tpl, placepair a1, placepair a2, placepair a3, placepair a4)
{
    return sreplace(sreplace(tpl, a1, a2, a3), a4);
}

template<typename D>
inline string dtos(const D& d)
{
    //instead of sprintf or itoa
    return boost::lexical_cast<string, D>(d);
}

template<typename D, typename S = string>
inline D stod(const S& s)
{
    //instead of atoi and others
    return boost::lexical_cast<D, S>(s);
}


//// safe max and min ////////////////////////////////////////////////////////////////////////////
template<typename T>
inline T max(const T& x, const T& y)
{
    return (x > y) ? x : y;
}

template<typename T>
inline T min(const T& x, const T& y)
{
    return (x < y) ? x : y;
}


//// asio buffer aux /////////////////////////////////////////////////////////////////////////////
inline void streambuf2cstr(std::string& dst, boost::asio::streambuf& src, const size_t& nbytes)
{
    //let's fuck c++ again...
    dst.assign(nbytes, 0);
    auto _cstr = const_cast<char*>(dst.c_str());
    src.sgetn(_cstr, nbytes);
}


//// debug helper ////////////////////////////////////////////////////////////////////////////////
//#include <boost/date_time/posix_time/posix_time.hpp>
//inline string getisotime(void)
//{
//    using boost::posix_time::to_iso_extended_string;
//    using boost::posix_time::second_clock;
//    return to_iso_extended_string(second_clock::local_time());
//}

inline string getisotime(void)
{
    char buf[32];
    struct tm* ptm;
    time_t t = time(0);
    ptm = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ptm);
    return string(buf);
}


}//basiohttp









#endif//SIMPLE_HTTP_UTIL_HPP

