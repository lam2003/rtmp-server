#ifndef RS_SAMPLE_HPP
#define RS_SAMPLE_HPP

#include <common/core.hpp>

#define MAX_CODEC_SAMPLE 128

class CodecSampleUnit
{
public:
    CodecSampleUnit();
    virtual ~CodecSampleUnit();

public:
    int size;
    char *bytes;
};

class ICodecSample
{
public:
    ICodecSample();
    virtual ~ICodecSample();

public:
    virtual void Clear();
    virtual int AddSampleUnit(char *bytes, int size);

public:
    int nb_sample_units;
    CodecSampleUnit sample_units[MAX_CODEC_SAMPLE];
};

#endif