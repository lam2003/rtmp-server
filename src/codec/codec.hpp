#ifndef RS_CODEC_HPP
#define RS_CODEC_HPP

#include <common/core.hpp>
#include <common/buffer.hpp>
#include <common/sample.hpp>

class IVCodec
{
public:
    IVCodec();
    virtual ~IVCodec();

public:
    virtual bool HasSequenceHeader() = 0;
    virtual int DecodeSequenceHeader(BufferManager *manager) = 0;
    virtual int DecodecNalu(BufferManager *manager, ICodecSample *sample) = 0;
};

class IACodec
{
public:
    IACodec();
    virtual ~IACodec();

public:
    virtual bool HasSequenceHeader() = 0;
    virtual int DecodeSequenceHeader(BufferManager *manager) = 0;
    virtual int DecodeRawData(BufferManager *manager, ICodecSample *sample) = 0;
};

#endif