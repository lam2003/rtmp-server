#ifndef RS_SOCKET_HPP
#define RS_SOCKET_HPP

#include <common/core.hpp>

#include <st.h>
#include <common/io/io.hpp>

class StSocket : public IProtocolReaderWriter
{
public:
    StSocket(st_netfd_t client_stfd);
    virtual ~StSocket();

public:
    virtual bool IsNeverTimeout(int64_t timeout_us) override;
    virtual void SetRecvTimeout(int64_t timeout_us) override;
    virtual int64_t GetRecvTimeout() override;
    virtual void SetSendTimeout(int64_t timeout_us) override;
    virtual int64_t GetSendTimeout() override;
    virtual int64_t GetSendBytes() override;
    virtual int64_t GetRecvBytes() override;

public:
    virtual int Read(void *buf, size_t size, ssize_t *nread) override;
    virtual int ReadFully(void *buf, size_t size, ssize_t *nread) override;
    virtual int Write(void *buf, size_t size, ssize_t *nread) override;
    virtual int WriteEv(const iovec *iov, int iov_size, ssize_t *nwrite) override;
};

#endif