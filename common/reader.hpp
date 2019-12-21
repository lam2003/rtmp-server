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

public:
    virtual bool IsOpen();
    virtual int64_t Tellg();
    virtual void Skip(int64_t size);
    virtual int64_t Lseek(int64_t offset);
    virtual int64_t FileSize();
    virtual int32_t Read(void *buf, size_t size, ssize_t *nread);
};
#endif