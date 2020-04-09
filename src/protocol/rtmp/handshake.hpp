/*
 * @Author: linmin
 * @Date: 2020-02-18 12:56:05
 * @LastEditTime: 2020-02-18 12:57:24
 */

#ifndef RS_RTMP_HANDSHAKE_HPP
#define RS_RTMP_HANDSHAKE_HPP

#include <common/core.hpp>
#include <common/io.hpp>

namespace rtmp
{

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
} // namespace rtmp

#endif