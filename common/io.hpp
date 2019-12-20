#ifndef RS_IO_HPP
#define RS_IO_HPP

#include <common/core.hpp>

#include <sys/uio.h>

class IBufferReader
{
public:
    IBufferReader();
    virtual ~IBufferReader();

public:
    virtual int Read(void *buf, size_t size, ssize_t *nread) = 0;
};

class IBufferWriter
{
public:
    IBufferWriter();
    virtual ~IBufferWriter();

public:
    virtual int Write(void *buf, size_t size, ssize_t *nwrite) = 0;
    virtual int WriteEv(const iovec *iov, int iov_size, ssize_t *nwrite) = 0;
};

class IStatistic
{
public:
    IStatistic();
    virtual ~IStatistic();

public:
    virtual int64_t GetRecvBytes() = 0;
    virtual int64_t GetSendBytes() = 0;
};

class IProtocolReader : public virtual IBufferReader, public virtual IStatistic
{
public:
    IProtocolReader();
    virtual ~IProtocolReader();

public:
    virtual void SetRecvTimeout(int64_t timeout_us) = 0;
    virtual int64_t GetRecvTimeout() = 0;

public:
    virtual int ReadFully(void *buf, size_t size, ssize_t *nread) = 0;
};

class IProtocolWriter : public virtual IBufferWriter, public virtual IStatistic
{
public:
    IProtocolWriter();
    virtual ~IProtocolWriter();

public:
    virtual void SetSendTimeout(int64_t timeout_us) = 0;
    virtual int64_t GetSendTimeout() = 0;
};

class IProtocolReaderWriter : public virtual IProtocolReader, public virtual IProtocolWriter
{
public:
    IProtocolReaderWriter();
    virtual ~IProtocolReaderWriter();

public:
    virtual bool IsNeverTimeout(int64_t timeout_us) = 0;
};
#endif