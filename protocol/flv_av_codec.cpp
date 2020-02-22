/*
 * @Author: linmin
 * @Date: 2020-02-21 16:28:32
 * @LastEditTime: 2020-02-21 18:29:36
 */
#include <protocol/flv_av_codec.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/config.hpp>
#include <common/utils.hpp>

namespace flv
{

static int avc_read_uev(BitBufferManager *manager, int32_t &v)
{
    int ret = ERROR_SUCCESS;

    int leading_zero_bits = -1;
    for (int8_t b = 0; !b && manager->Empty(); leading_zero_bits++)
    {
        b = manager->ReadBit();
    }

    if (leading_zero_bits >= 31)
    {
        return ERROR_AVC_NALU_UEV;
    }

    v = (1 << leading_zero_bits) - 1;

    for (int i = 0; i < leading_zero_bits; i++)
    {
        int32_t b = manager->ReadBit();
        v += b << (leading_zero_bits - i - 1);
    }

    return ret;
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
    flv_sample_rate = AudioSampleRate::UNKNOW;
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
    frame_type = VideoFrameType::UNKNOW;
    avc_packet_type = VideoPacketType::UNKNOW;
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

AVInfo::AVInfo()
{
}

AVInfo::~AVInfo()
{
}

int AVInfo::avc_demux_sequence_header(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    avc_extra_size = manager->Size() - manager->Pos();

    if (avc_extra_size > 0)
    {
        rs_freepa(avc_extra_data);
        avc_extra_data = new char[avc_extra_size];
        memcpy(avc_extra_data, manager->Data() + manager->Pos(), avc_extra_size);
    }

    if (!manager->Require(6))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    manager->Read1Bytes();
    avc_profile = (AVCProfile)manager->Read1Bytes();
    manager->Read1Bytes();
    avc_level = (AVCLevel)manager->Read1Bytes();

    int8_t length_size_minus_one = manager->Read1Bytes();
    length_size_minus_one &= 0x03;

    nalu_unit_length = length_size_minus_one;
    if (nalu_unit_length == 2)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("seqence should never be 2. ret=%d", ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    int8_t num_of_sps = manager->Read1Bytes();
    num_of_sps &= 0x1f;

    if (num_of_sps != 1)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    if (!manager->Require(2))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    sps_length = manager->Read2Bytes();
    if (!manager->Require(sps_length))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    if (sps_length > 0)
    {
        rs_freepa(sps);
        sps = new char[sps_length];
        manager->ReadBytes(sps, sps_length);
    }

    if (!manager->Require(1))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    int8_t num_of_pps = manager->Read1Bytes();
    num_of_pps &= 0x1f;

    if (num_of_pps != 1)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    pps_length = manager->Read2Bytes();
    if (!manager->Require(pps_length))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    if (pps_length > 0)
    {
        rs_freepa(pps);
        pps = new char[pps_length];
        manager->ReadBytes(pps, pps_length);
    }

    return ret;
}

int AVInfo::avc_demux_sps()
{
    int ret = ERROR_SUCCESS;

    if (!sps_length)
    {
        return ret;
    }

    return ret;
}

int AVInfo::AVCDemux(char *data, int size, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    sample->is_video = true;

    if (!data || size <= 0)
    {
        return ret;
    }

    BufferManager manager;
    if ((ret = manager.Initialize(data, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(1))
    {
        ret = ERROR_DECODE_FLV_FAILED;
        rs_error("decode frame_type failed. ret=%d", ret);
        return ret;
    }

    int8_t frame_type = manager.Read1Bytes();
    int8_t codec_id = frame_type & 0x0f;
    frame_type = (frame_type >> 4) & 0x0f;

    sample->frame_type = (VideoFrameType)frame_type;

    if (sample->frame_type != VideoFrameType::VIDEO_INFO_FRAME)
    {
        rs_warn("ignore the info frame");
        return ret;
    }

    if (codec_id != (int8_t)VideoCodecType::AVC)
    {
        ret = ERROR_DECODE_FLV_FAILED;
        rs_error("only support h264/avc codec. actual=%d, ret=%d", codec_id, ERROR_DECODE_FLV_FAILED);
        return ret;
    }

    video_codec_id = codec_id;
    if (!manager.Require(4))
    {
        ret = ERROR_DECODE_FLV_FAILED;
        rs_error("decode avc_packet_type failed. ret=%d", ret);
        return ret;
    }

    int8_t avc_packet_type = manager.Read1Bytes();
    int32_t composition_time = manager.Read3Bytes();

    sample->cts = composition_time;
    sample->avc_packet_type = (VideoPacketType)avc_packet_type;

    if (avc_packet_type == (int8_t)VideoPacketType::SEQUENCE_HEADER)
    {
    }
    else if (avc_packet_type == (int8_t)VideoPacketType::NALU)
    {
    }
    else
    {
        //ignore
    }

    return ret;
}

} // namespace flv