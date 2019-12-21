#include <protocol/rtmp_stack.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>
#include <common/buffer.hpp>
#include <common/log.hpp>

HandshakeBytes::HandshakeBytes() : c0c1(nullptr),
                                   s0s1s2(nullptr),
                                   c2(nullptr)
{
}

HandshakeBytes::~HandshakeBytes()
{
    rs_freepa(c0c1);
    rs_freepa(s0s1s2);
    rs_freepa(c2);
}

int32_t HandshakeBytes::ReadC0C1(IProtocolReaderWriter *rw)
{
    int32_t ret = ERROR_SUCCESS;

    if (c0c1)
    {
        return ret;
    }

    ssize_t nread;
    c0c1 = new char[1537];

    if ((ret = rw->ReadFully(c0c1, 1537, &nread)) != ERROR_SUCCESS)
    {
        rs_error("read c0c1 failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("read c0c1 success");
    return ret;
}

int32_t HandshakeBytes::ReadS0S1S2(IProtocolReaderWriter *rw)
{
    int32_t ret = ERROR_SUCCESS;

    if (s0s1s2)
    {
        return ret;
    }

    ssize_t nread;
    s0s1s2 = new char[3073];
    if ((ret = rw->ReadFully(s0s1s2, 3073, &nread)) != ERROR_SUCCESS)
    {
        rs_error("read s0s1s2 failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("read s0s1s2 success");
    return ret;
}

int32_t HandshakeBytes::ReadC2(IProtocolReaderWriter *rw)
{
    int32_t ret = ERROR_SUCCESS;

    if (c2)
    {
        return ret;
    }

    ssize_t nread;
    c2 = new char[1536];
    if ((ret = rw->ReadFully(c2, 1536, &nread)) != ERROR_SUCCESS)
    {
        rs_error("read c2 failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("read c2 success");
    return ret;
}

int32_t HandshakeBytes::CreateC0C1()
{
    int32_t ret = ERROR_SUCCESS;

    if (c0c1)
    {
        return ret;
    }

    c0c1 = new char[1537];
    Utils::RandomGenerate(c0c1, 1537);

    Buffer buffer;
    if ((ret = buffer.Initialize(c0c1, 9)) != ERROR_SUCCESS)
    {
        return ret;
    }

    //c0
    buffer.Write1Bytes(0x03);
    //c1
    buffer.Write4Bytes((int32_t)::time(nullptr));
    buffer.Write4Bytes(0x00);

    return ret;
}

int32_t HandshakeBytes::CreateS0S1S2(const char *c1)
{
    int ret = ERROR_SUCCESS;

    if (s0s1s2)
    {
        return ret;
    }

    s0s1s2 = new char[3073];
    Utils::RandomGenerate(s0s1s2, 3073);

    Buffer buffer;
    if ((ret = buffer.Initialize(s0s1s2, 9)) != ERROR_SUCCESS)
    {
        return ret;
    }
    //s0
    buffer.Write1Bytes(0x03);
    //s1
    buffer.Write4Bytes((int32_t)::time(NULL));
    // s1 time2 copy from c1
    if (c0c1)
    {
        buffer.WriteBytes(c0c1 + 1, 4);
    }

    //s2
    // if c1 specified, copy c1 to s2.
    if (c1)
    {
        memcpy(s0s1s2 + 1537, c1, 1536);
    }

    return ret;
}

int32_t HandshakeBytes::CreateC2()
{
    int ret = ERROR_SUCCESS;

    if (c2)
    {
        return ret;
    }

    c2 = new char[1536];
    Utils::RandomGenerate(c2, 1536);

    Buffer buffer;
    if ((ret = buffer.Initialize(c2, 8)) != ERROR_SUCCESS)
    {
        return ret;
    }

    buffer.Write4Bytes((int32_t)::time(nullptr));
    if (s0s1s2)
    {
        buffer.WriteBytes(s0s1s2 + 1, 4);
    }

    return ret;
}