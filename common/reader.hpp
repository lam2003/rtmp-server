#ifndef RS_READER_H
#define RS_READER_H

#include <common/core.hpp>

#include <string>

class Reader
{
public:
    Reader();
    virtual ~Reader();

public:
    virtual int32_t Open(const std::string &p) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() = 0;
    virtual int64_t Tellg() = 0;
    virtual void Skip(int64_t size) = 0;
    virtual int64_t Lseek(int64_t offset) = 0;
    virtual int64_t FileSize() = 0;
    virtual int32_t Read(void *buf, size_t size, ssize_t *nread) = 0;
};
#endif