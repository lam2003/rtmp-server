/*
 * @Author: linmin
 * @Date: 2020-02-06 19:12:24
 * @LastEditTime: 2020-02-21 16:40:45
 */

#ifndef RS_FLV_HPP
#define RS_FLV_HPP

#include <common/core.hpp>
#include <common/file.hpp>
#include <protocol/rtmp_stack.hpp>
#include <protocol/flv_av_codec.hpp>

#define FLV_TAG_HEADER_SIZE 11
#define FLV_PREVIOUS_TAG_SIZE 4
#define AAC_SAMPLE_RATE_UNSET 15

namespace flv
{


extern std::string ACodec2Str(AudioCodecType codec_type);
extern std::string AACProfile2Str(AACObjectType object_type);


class Encoder
{
public:
    Encoder();
    virtual ~Encoder();

public:
    static int SizeTag(int data_size);

    virtual int Initialize(FileWriter *writer);
    virtual int WriteFlvHeader();
    virtual int WriteFlvHeader(char flv_header[9]);
    virtual int WriteMetadata(char *data, int size);
    virtual int WriteAudio(int64_t timestamp, char *data, int size);
    virtual int WriteVideo(int64_t timestamp, char *data, int size);
    virtual int WriteTags(rtmp::SharedPtrMessage **msgs, int count);

private:
    int write_tag_header_to_cache(char type, int size, int timestamp, char *cache);
    int write_previous_tag_size_to_cache(int size, char *cache);
    int write_tag(char *header, int header_size, char *tag, int tag_size);

private:
    FileWriter *writer_;
    int nb_tag_headers_;
    char *tag_headers_;
    int nb_ppts_;
    char *ppts_;
    int nb_iovss_cache_;
    iovec *iovss_cache_;
};

class Codec
{
public:
    Codec();
    virtual ~Codec();

public:
    static bool IsVideoSeqenceHeader(char *data, int size);
    static bool IsAudioSeqenceHeader(char *data, int size);
    static bool IsH264(char *data, int size);
    static bool IsAAC(char *data, int size);
    static bool IsKeyFrame(char *data, int size);
    int DemuxAudio(char *data, int size, CodecSample *sample);

private:
    int aac_sequence_header_demux(char *data, int size);
    bool is_aac_codec_ok();

public:
    int audio_codec_id;
    int aac_extra_size;
    char *aac_extra_data;
    uint8_t channels;
    uint8_t sample_rate;
    AACObjectType aac_object_type;
};

}; // namespace flv

#endif