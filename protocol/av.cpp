#include <protocol/av.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/buffer.hpp>
#include <common/utils.hpp>

namespace av
{

std::string ACodec2Str(AudioCodecType codec_type)
{
    switch (codec_type)
    {
    case AudioCodecType::AAC:
        return "AAC";
    case AudioCodecType::MP3:
        return "MP3";
    default:
        return "OTHER";
    }
}

std::string AACProfile2Str(AACObjectType object_type)
{
    switch (object_type)
    {
    case AACObjectType::MAIN:
        return "Main";
    case AACObjectType::LC:
        return "LC";
    case AACObjectType::SSR:
        return "SSR";
    default:
        return "Other";
    }
}

CodecSampleUnit::CodecSampleUnit()
{
    size = 0;
    bytes = nullptr;
}

CodecSampleUnit::~CodecSampleUnit()
{
}

CodecSample::CodecSample()
{
    Clear();
}

CodecSample::~CodecSample()
{
}

void CodecSample::Clear()
{
    is_video = false;
    acodec = AudioCodecType::UNKNOW;
    flv_sample_rate = FLVSampleRate::UNKNOW;
    aac_sample_rate = 0;
    sound_size = AudioSampleSize::UNKNOW;
    sound_type = AudioSoundType::UNKNOW;
    aac_packet_type = AudioPacketType::UNKNOW;
    nb_sample_units = 0;
    cts = 0;
    has_idr = false;
    has_sps_pps = false;
    has_aud = false;
    first_nalu_type = AVCNaluType::UNKNOW;
}

int CodecSample::AddSampleUnit(char *bytes, int size)
{
    int ret = ERROR_SUCCESS;

    if (nb_sample_units >= MAX_CODEC_SAMPLE)
    {
        ret = ERROR_SAMPLE_EXCEED;
        rs_error("codec sample exceed the max count: %d. ret=%d", MAX_CODEC_SAMPLE, ret);
        return ret;
    }

    CodecSampleUnit *sample_unit = &sample_units[nb_sample_units++];
    sample_unit->bytes = bytes;
    sample_unit->size = size;

    if (is_video)
    {
        AVCNaluType nalu_type = (AVCNaluType)(bytes[0] & 0x1f);
        if (nalu_type == AVCNaluType::IDR)
        {
            has_idr = true;
        }
        else if (nalu_type == AVCNaluType::SPS || nalu_type == AVCNaluType::PPS)
        {
            has_sps_pps = true;
        }
        else if (nalu_type == AVCNaluType::ACCESS_UNIT_DELIMITER)
        {
            has_aud = true;
        }
        else
        {
            //ignore
        }

        if (first_nalu_type == AVCNaluType::UNKNOW)
        {
            first_nalu_type = nalu_type;
        }
    }

    return ret;
}

Codec::Codec()
{
    audio_codec_id = 0;
    aac_extra_size = 0;
    aac_extra_data = nullptr;
    channels = 0;
    sample_rate = AAC_SAMPLE_RATE_UNSET;
    aac_object_type = AACObjectType::UNKNOW;
}

Codec::~Codec()
{
}

bool Codec::is_h264(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0];

    return codec_id == (char)VideoCodecType::AVC;
}

bool Codec::is_aac(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0];
    codec_id = (codec_id >> 4) & 0x0f;

    return codec_id == (char)AudioCodecType::AAC;
}

bool Codec::IsVideoSeqenceHeader(char *data, int size)
{
    if (!is_h264(data, size))
    {
        return false;
    }

    if (size < 2)
    {
        return false;
    }

    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0f;

    char packet_type = data[1];

    return frame_type == (char)VideoFrameType::KEY_FRAME &&
           packet_type == (char)VideoPacketType::SEQUENCE_HEADER;
}

bool Codec::IsAudioSeqenceHeader(char *data, int size)
{
    if (!is_aac(data, size))
    {
        return false;
    }

    if (size < 2)
    {
        return false;
    }

    char packet_type = data[1];

    return packet_type == (char)AudioPacketType::SEQUENCE_HEADER;
}

int Codec::aac_sequence_header_demux(char *data, int size)
{
    int ret = ERROR_SUCCESS;

    BufferManager manager;
    if (manager.Initialize(data, size) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(2))
    {
        ret = ERROR_DECODE_AAC_EXTRA_DATA_FAILED;
        rs_error("decode aac sequence header failed. ret=%d", ret);
        return ret;
    }

    uint8_t object_type = manager.Read1Bytes();
    uint8_t sampling_frequency_index = manager.Read1Bytes();

    channels = (sampling_frequency_index >> 3) & 0x0f;
    sampling_frequency_index = ((object_type & 0x07) << 1) | ((sampling_frequency_index >> 7) & 0x01);
    object_type = (object_type >> 3) & 0x1f;

    sample_rate = sampling_frequency_index;

    aac_object_type = (AACObjectType)object_type;

    if (aac_object_type == AACObjectType::HE || aac_object_type == AACObjectType::HEV2)
    {
        ret = ERROR_ACODEC_NOT_SUPPORT;
        rs_error("not support HE/HE2 yet. ret=%d", ret);
        return ret;
    }
    else if (aac_object_type == AACObjectType::UNKNOW)
    {
        ret = ERROR_DECODE_AAC_EXTRA_DATA_FAILED;
        rs_error("decode aac sequence header failed, adts object=%d invalid. ret=%d", object_type, ret);
        return ret;
    }
    else
    {
        //TODO: support HE/HE2. see: ngx_rtmp_codec_parse_aac_header
    }

    return ret;
}

bool Codec::is_aac_codec_ok()
{
    return aac_extra_size > 0 && aac_extra_data;
}

int Codec::DemuxAudio(char *data, int size, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    sample->is_video = false;
    if (!data || size <= 0)
    {
        rs_warn("no audio persent, ignore it.");
        return ret;
    }

    BufferManager manager;
    if ((ret = manager.Initialize(data, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(1))
    {
        ret = ERROR_DECODE_AAC_FAILED;
        rs_error("aac decode sound_format failed. ret=%d", ret);
        return ret;
    }

    int sound_format = manager.Read1Bytes();

    int sound_type = sound_format & 0x01;
    int sound_size = (sound_format >> 1) & 0x01;
    int sound_rate = (sound_format >> 2) & 0x03;
    sound_format = (sound_format >> 4) & 0x0f;

    audio_codec_id = sound_format;
    sample->acodec = (AudioCodecType)audio_codec_id;
    sample->sound_type = (AudioSoundType)sound_type;
    sample->sound_size = (AudioSampleSize)sound_size;
    sample->flv_sample_rate = (FLVSampleRate)sound_rate;

    if (audio_codec_id == (int)AudioCodecType::MP3)
    {
        return ERROR_ACODEC_TRY_MP3;
    }

    if (audio_codec_id != (int)AudioCodecType::AAC)
    {
        ret = ERROR_DECODE_AAC_FAILED;
        rs_error("only support mp3/aac codec. actual=%d, ret=%d", audio_codec_id, ret);
        return ret;
    }

    if (!manager.Require(1))
    {
        ret = ERROR_DECODE_AAC_FAILED;
        rs_error("aac decode aac_packet_type failed. ret=%d", ret);
        return ret;
    }

    int8_t aac_packet_type = manager.Read1Bytes();
    sample->aac_packet_type = (AudioPacketType)aac_packet_type;

    if (aac_packet_type == (int8_t)AudioPacketType::SEQUENCE_HEADER)
    {
        aac_extra_size = manager.Size() - manager.Pos();
        if (aac_extra_size > 0)
        {
            rs_freepa(aac_extra_data);
            aac_extra_data = new char[aac_extra_size];
            memcpy(aac_extra_data, manager.Data() + manager.Pos(), aac_extra_size);

            if ((ret = aac_sequence_header_demux(aac_extra_data, aac_extra_size)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }
    }
    else if (aac_packet_type == (int8_t)AudioPacketType::RAW_DATA)
    {

        if (!is_aac_codec_ok())
        {
            rs_warn("aac ignore type=%d for no sequence header. ret=%d", aac_packet_type, ret);
            return ret;
        }

        if ((ret = sample->AddSampleUnit(manager.Data() + manager.Pos(), manager.Size() - manager.Pos())) != ERROR_SUCCESS)
        {
            rs_error("add aac sample failed. ret=%d", ret);
            return ret;
        }
    }
    else
    {
        //ignore
    }

    if (sample_rate != AAC_SAMPLE_RATE_UNSET)
    {
        static int aac_sample_rates[] = {
            96000, 88200, 64000,
            48000, 44100, 32000,
            24000, 22050, 16000,
            12000, 11025, 8000,
            0, 0, 0, 0};
        switch (aac_sample_rates[sample_rate])
        {
        case 11025:
            sample->flv_sample_rate = FLVSampleRate::SAMPLE_RATE_11025;
            break;
        case 22050:
            sample->flv_sample_rate = FLVSampleRate::SAMPLE_RATE_22050;
            break;
        case 44100:
            sample->flv_sample_rate = FLVSampleRate::SAMPLE_RATE_44100;
            break;
        default:
            break;
        }
        sample->aac_sample_rate = aac_sample_rates[sample_rate];
    }

    rs_info("aac decoded, type=%d, codec=%d, asize=%d, rate=%d, format=%d, size=%d",
            sound_type, audio_codec_id, sound_size, sound_rate, sound_format, size);

    return ret;
}
} // namespace av