/*
 * @Author: linmin
 * @Date: 2020-02-21 13:20:04
 * @LastEditTime: 2020-02-21 17:52:58
 */
#ifndef RS_FLV_AV_HPP
#define RS_FLV_AV_HPP

#include <common/core.hpp>
#include <common/buffer.hpp>

#define MAX_CODEC_SAMPLE 128

namespace flv
{
//### AVC defines begin
enum class AVCProfile
{
    UNKNOW = 0,
    BASELINE = 66,
    CONSTRAINED_BASELINE = 578,
    MAIN = 77,
    EXTENDED = 88,
    HIGH = 100,
    HIGH_10 = 110,
    HIGH_10_INTRA = 2158,
    HIGH_422 = 122,
    HIGH_422_INTRA = 2170,
    HIGH_444 = 144,
    HIGH_444_PREDICTIVE = 244,
    HIGH_444_INTRA = 2192
};

enum class AVCLevel
{
    UNKNOW = 0,
    LEVEL_1 = 10,
    LEVEL_11 = 11,
    LEVEL_12 = 12,
    LEVEL_13 = 13,
    LEVEL_2 = 20,
    LEVEL_21 = 21,
    LEVEL_22 = 22,
    LEVEL_3 = 30,
    LEVEL_31 = 31,
    LEVEL_32 = 32,
    LEVEL_4 = 40,
    LEVEL_41 = 41,
    LEVEL_5 = 50,
    LEVEL_51 = 51
};

enum class AVCPayloadFormat
{
    GUESS = 0,
    ANNEXB = 1,
    IBMF = 2
};

enum class AVCNaluType
{
    UNKNOW = 0,
    NON_IDR = 1,
    DATA_PARTITION_A = 2,
    DATA_PARTITION_B = 3,
    DATA_PARTITION_C = 4,
    IDR = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8,
    ACCESS_UNIT_DELIMITER = 9,
    EOSEQUENCE = 10,
    EOSTREAM = 11,
    FILTER_DATA = 12,
    SPS_EXT = 13,
    PERFIX_NALU = 14,
    SUBSET_SPS = 15,
    LAYER_WITHOUT_PARTITION = 19,
    CODECD_SLICE_EXT = 20
};
//### AVC defines end

//### AAC defines begin
enum class AACObjectType
{
    UNKNOW = 0,
    MAIN = 1,
    LC = 2,
    SSR = 3,
    //LC + SBR
    HE = 4,
    //LC + SBR + PS
    HEV2 = 5
};
//### AAC defines end

//### FLV defines begin
enum class VideoCodecType
{
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

enum class AudioSampleSize
{
    SAMPLE_SIZE_8BIT = 0,
    SAMPLE_SIZE_16BIT = 1,
    UNKNOW = 2
};

enum class AudioSoundType
{
    MONO = 0,
    STEREO = 1,
    UNKNOW = 2
};

enum class AudioPacketType
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
//### flv defines end

class CodecSampleUnit
{
public:
    CodecSampleUnit();
    virtual ~CodecSampleUnit();

public:
    int size;
    char *bytes;
};

class CodecSample
{
public:
    CodecSample();
    virtual ~CodecSample();

public:
    virtual void Clear();
    virtual int AddSampleUnit(char *bytes, int size);

public:
    bool is_video;
    AudioCodecType acodec;
    AudioSampleRate flv_sample_rate;
    int aac_sample_rate;
    AudioSampleSize sound_size;
    AudioSoundType sound_type;
    AudioPacketType aac_packet_type;
    int nb_sample_units;
    CodecSampleUnit sample_units[MAX_CODEC_SAMPLE];
    int32_t cts;
    bool has_idr;
    bool has_sps_pps;
    bool has_aud;
    AVCNaluType first_nalu_type;
    VideoFrameType frame_type;
    VideoPacketType avc_packet_type;
};

class AVInfo
{
public:
    AVInfo();
    virtual ~AVInfo();

public:
    int AVCDemux(char *data, int size, CodecSample *sample);

private:
    int avc_demux_sequence_header(BufferManager *manager);
    int avc_demux_sps();

public:
    int duration;
    int width;
    int height;
    int frame_rate;
    int video_codec_id;
    int video_data_rate;
    int audio_codec_id;
    int audio_data_rate;
    //profile_idc, H.264-AVC-ISO_IEC_14496-10.pdf, page 45.
    AVCProfile avc_profile;
    AVCLevel avc_level;
    int8_t nalu_unit_length;
    uint16_t sps_length;
    char *sps;
    uint16_t pps_length;
    char *pps;
    AVCPayloadFormat payload_format;
    AACObjectType aac_obj_type;
    //samplingFrequencyIndex
    uint8_t aac_sample_rate;
    uint8_t aac_channels;
    int avc_extra_size;
    char *avc_extra_data;
    int aac_extra_size;
    char *aac_extra_data;
    bool avc_parse_sps;
};
} // namespace flv

#endif