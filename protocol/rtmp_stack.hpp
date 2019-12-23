#ifndef RS_RTMP_STACK_HPP
#define RS_RTMP_STACK_HPP

#include <common/core.hpp>
#include <common/io.hpp>
#include <common/buffer.hpp>
#include <protocol/rtmp_message.hpp>

class HandshakeBytes
{
public:
    HandshakeBytes();
    virtual ~HandshakeBytes();

    virtual int32_t ReadC0C1(IProtocolReaderWriter *rw);
    virtual int32_t ReadS0S1S2(IProtocolReaderWriter *rw);
    virtual int32_t ReadC2(IProtocolReaderWriter *rw);
    virtual int32_t CreateC0C1();
    virtual int32_t CreateS0S1S2(const char *c1 = NULL);
    virtual int32_t CreateC2();

public:
    //1+1536
    char *c0c1;
    //1+1536+1536
    char *s0s1s2;
    //1536
    char *c2;
};

class SimpleHandshake
{
public:
    SimpleHandshake();
    virtual ~SimpleHandshake();

public:
    virtual int32_t HandshakeWithClient(HandshakeBytes *handshake_bytes, IProtocolReaderWriter *rw);
};

class RTMPProtocol
{
public:
    RTMPProtocol(IProtocolReaderWriter *rw);
    virtual ~RTMPProtocol();

public:
    virtual void SetSendTimeout(int64_t timeout_us);
    virtual void SetRecvTimeout(int64_t timeout_us);
    virtual int ReadInterlacedMessage(RTMPCommonMessage **pmsg);

protected:
    virtual int ReadBasicHeader(char &fmt, int &cid);

private:
    IProtocolReaderWriter *rw_;
    FastBuffer *in_buffer_;
};

class RTMPServer
{
public:
    RTMPServer(IProtocolReaderWriter *rw);
    virtual ~RTMPServer();

public:
    virtual int32_t Handshake();
    virtual void SetSendTimeout(int64_t timeout_us);
    virtual void SetRecvTimeout(int64_t timeout_us);
    virtual int32_t RecvMessage(RTMPCommonMessage **pmsg);

private:
    IProtocolReaderWriter *rw_;
    HandshakeBytes *handshake_bytes_;
    RTMPProtocol *protocol_;
};

#endif