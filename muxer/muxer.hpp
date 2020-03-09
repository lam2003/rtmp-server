#ifndef RS_MUXER_HPP
#define RS_MUXER_HPP

#include <common/core.hpp>
#include <common/file.hpp>
#include <common/sample.hpp>

class Muxer
{
public:
    Muxer();
    virtual ~Muxer();

public:
    virtual int Initialize(FileWriter *writer) = 0;
    virtual int WriteMetadata(char *data, int size) = 0;
    virtual int WriteAudio(int64_t timestamp, char *data, int size) = 0;
    virtual int WriteVideo(int64_t timestamp, char *data, int size) = 0;
    virtual int WriteMuxerHeader() = 0;
};

class Demuxer
{
public:
    Demuxer();
    virtual ~Demuxer();

public:
    virtual int DemuxAudio(char *data, int size, CodecSample *sample) = 0;
    virtual int DemuxVideo(char *data, int size, CodecSample *sample) = 0;
};

#endif