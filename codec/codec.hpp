#ifndef RS_CODEC_HPP
#define RS_CODEC_HPP

#include <common/core.hpp>
#include <common/buffer.hpp>
#include <common/sample.hpp>

class VCodec
{
public:
    VCodec();
    virtual ~VCodec();

public:
    virtual int DecodeSequenceHeader(BufferManager *manager) = 0;
    virtual int DecodecNalu(BufferManager *manager, CodecSample *sample) = 0;
};

class ACodec
{
public:
    ACodec();
    virtual ~ACodec();

public:
    virtual bool HasSequenceHeader() = 0;
    virtual int DecodeSequenceHeader() = 0;
    virtual int DecodeRawData(BufferManager *manager) = 0;
};

#endif