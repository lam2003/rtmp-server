#ifndef RS_FLV_HPP
#define RS_FLV_HPP

#include <common/core.hpp>
#include <common/file.hpp>
#include <common/sample.hpp>
#include <protocol/rtmp_stack.hpp>
#include <muxer/muxer.hpp>
#include <codec/codec.hpp>

#define FLV_TAG_HEADER_SIZE 11
#define FLV_PREVIOUS_TAG_SIZE 4
#define AAC_SAMPLE_RATE_UNSET 15

namespace flv
{
enum class VideoCodecType
{
    UNKNOW = 0,
    SORENSON_H263 = 2,
    SCREEN_VIDEO = 3,
    ON2_VP6 = 4,
    ON3_VP6_WITH_ALPHA_CHANNEL = 5,
    SCREEN_VIDEO_VERSION2 = 6,
    AVC = 7
};

enum class VideoFrameType
{
    UNKNOW = 0,
    KEY_FRAME = 1,
    INTER_FRAME = 2,
    DISPOSABLE_INTER_FRAME = 3,
    GENERATED_KEY_FRAME = 4,
    VIDEO_INFO_FRAME = 5
};

enum class VideoPacketType
{
    SEQUENCE_HEADER = 0,
    NALU = 1,
    SEQUENCE_HEADER_EOF = 2,
    UNKNOW = 4
};

enum class AudioCodecType
{
    LINEAR_PCM_PLATFORM_ENDIAN = 0,
    ADPCM = 1,
    MP3 = 2,
    LINEAR_PCM_LITTLE_ENDIAN = 3,
    NELLY_MOSER_16KHZ_MONO = 4,
    NELLY_MOSER_8KHZ_MONO = 5,
    NELLY_MOSER = 6,
    AAC = 10,
    SPEEX = 11,
    MP3_8KHZ = 14,
    DEVICE_SPECIFIC_SOUND = 15,
    UNKNOW = 16
};

enum class AudioSampleRate
{
    SAMPLE_RATE_5512 = 0,
    SAMPLE_RATE_11025 = 1,
    SAMPLE_RATE_22050 = 2,
    SAMPLE_RATE_44100 = 3,
    UNKNOW = 4
};

enum class AudioSoundSize
{
    BIT_DEPTH_8BIT = 0,
    BIT_DEPTH_16BIT = 1,
    UNKNOW = 2
};

enum class AudioSoundType
{
    MONO = 0,
    STEREO = 1,
    UNKNOW = 2
};

enum class AACPacketType
{
    SEQUENCE_HEADER = 0,
    RAW_DATA = 1,
    UNKNOW = 2
};

enum class TagType
{
    AUDIO = 8,
    VIDEO = 9,
    SCRIPT = 18,
    UNKNOW = 0
};

extern std::string sample_rate_to_str(AudioSampleRate sample_rate);
extern std::string audio_codec_type_to_str(AudioCodecType code_type);
extern std::string sound_type_to_str(AudioSoundType sound_type);
extern std::string sound_size_to_str(AudioSoundSize sound_size);
} // namespace flv

class FlvCodecSample : public CodecSample
{
public:
    FlvCodecSample();
    virtual ~FlvCodecSample();

public:
    bool is_video;
    flv::AudioCodecType acodec_type;
    flv::AudioSoundType sound_type;
    flv::AudioSoundSize sound_size;
    flv::AudioSampleRate sample_rate;
    flv::AACPacketType aac_pkt_type;
    bool has_print;
};

class FlvMuxer : public Muxer
{
public:
    FlvMuxer();
    virtual ~FlvMuxer();

public:
    static int SizeTag(int data_size);
    virtual int Initialize(FileWriter *writer) override;
    virtual int WriteMetadata(char *data, int size) override;
    virtual int WriteAudio(int64_t timestamp, char *data, int size) override;
    virtual int WriteVideo(int64_t timestamp, char *data, int size) override;
    virtual int WriteMuxerHeader() override;

private:
    int write_flv_header(char flv_header[9]);
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

class FlvDemuxer : public Demuxer
{
public:
    FlvDemuxer();
    virtual ~FlvDemuxer();

public:
    int DemuxAudio(char *data, int size, CodecSample *s);
    int DemuxVideo(char *data, int size, CodecSample *s);

    static bool IsAVC(char *data, int size);
    static bool IsAAC(char *data, int size);
    static bool IsAACSequenceHeader(char *data, int size);
    static bool IsAVCSequenceHeader(char *data, int size);
    static bool IsKeyFrame(char *data, int size);

private:
    int demux_aac(BufferManager *manager, FlvCodecSample *sample);
    int demux_avc(BufferManager *manager, FlvCodecSample *sample);

public:
    flv::VideoCodecType vcodec_type;
    VCodec *vcodec;
    flv::AudioCodecType acodec_type;
    ACodec *acodec;
};

#endif