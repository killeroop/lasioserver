/*
 * File IO
 */
#ifndef __READER_H_
#define __READER_H_
#include <cerrno>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

namespace basiohttp
{
using std::string;
typedef char byte;

template<typename T>
inline bool file_check(const T& filename)
{
    bool exist = boost::filesystem::exists(filename);
    bool isdir = boost::filesystem::is_directory(filename);
    return exist and not isdir;
}

typedef boost::shared_ptr<std::ofstream> ofstream_ptr;

ofstream_ptr make_ofstream(void)
{
    return ofstream_ptr(new std::ofstream, [](std::ofstream* p){ p->flush(); p->close(); });
}


struct mmap_reader
{
    string __fn;
    int    __fd;
    byte*  __mapped;
    byte   __empty[2];
    size_t __fsize;

    mmap_reader(void) = delete;
    mmap_reader(const string& filename)
    {
        __fn = filename;
        __fd = -1;
        __mapped = nullptr;
        bzero(&__empty, sizeof(__empty));
        if (file_check(filename)) {
            __fsize = boost::filesystem::file_size(filename);
        }
    }

    ~mmap_reader(void)
    {
        if (__fd > 0) {
            ::close(__fd);
        }
        if (__mapped) {
            ::munmap(__mapped, __fsize);
        }
    }

    const size_t size(void)
    {
        return __fsize;
    }

    const byte* read(void)
    {
        if (__mapped) {
            return __mapped;
        }
        if (not __fsize) {
            return __empty;
        }

        __fd = ::open64(__fn.c_str(), O_RDONLY);
        if (__fd <= 0) {
            //std::cerr << "IO Error" << __fn << "\n";
            return nullptr;
        }

        __mapped = (byte*)::mmap64(0, __fsize, PROT_READ, MAP_PRIVATE, __fd, 0);
        if (__mapped == (byte*)(-1)) {
            return nullptr;
        }
        assert(__mapped);
        return __mapped;
    }
};

}

#endif//__READER_H_
