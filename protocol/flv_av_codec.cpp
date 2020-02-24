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

    if (manager->Empty())
    {
        return ERROR_BIT_BUFFER_MANAGER_EMPTY;
    }

    int leading_zero_bits = -1;
    for (int8_t b = 0; !b && !manager->Empty(); leading_zero_bits++)
    {
        b = manager->ReadBit();
    }

    if (leading_zero_bits >= 31)
    {
        return ERROR_BIT_BUFFER_MANAGER_EMPTY;
    }

    v = (1 << leading_zero_bits) - 1;

    for (int i = 0; i < leading_zero_bits; i++)
    {
        int32_t b = manager->ReadBit();
        v += b << (leading_zero_bits - i - 1);
    }

    return ret;
}

static int avc_read_bit(BitBufferManager *manager, int8_t &v)
{
    int ret = ERROR_SUCCESS;

    if (manager->Empty())
    {
        return ERROR_BIT_BUFFER_MANAGER_EMPTY;
    }

    v = manager->ReadBit();

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
    duration = 0;
    width = 0;
    height = 0;
    frame_rate = 0;
    video_codec_id = 0;
    video_data_rate = 0;
    audio_codec_id = 0;
    audio_data_rate = 0;
    avc_profile = AVCProfile::UNKNOW;
    avc_level = AVCLevel::UNKNOW;
    nalu_unit_length = 0;
    sps_length = 0;
    sps = nullptr;
    pps_length = 0;
    pps = nullptr;
    payload_format = AVCPayloadFormat::GUESS;
    aac_obj_type = AACObjectType::UNKNOW;
    aac_sample_rate = AAC_SAMPLE_RATE_UNSET;
    aac_channels = 0;
    avc_extra_size = 0;
    avc_extra_data = nullptr;
    aac_extra_size = 0;
    aac_extra_data = nullptr;
    avc_parse_sps = true;
}

AVInfo::~AVInfo()
{
}

int AVInfo::avc_demux_sps_rbsp(char *rbsp, int nb_rbsp)
{
    // for SPS, 7.3.2.1.1 Sequence parameter set data syntax
    // H.264-AVC-ISO_IEC_14496-10-2012.pdf, page 62.

    int ret = ERROR_SUCCESS;

    if (!avc_parse_sps)
    {
        return ret;
    }

    BufferManager manager;
    if (manager.Initialize(rbsp, nb_rbsp) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(3))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t profile_idc = manager.Read1Bytes();
    if (!profile_idc)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t flags = manager.Read1Bytes();
    if (flags & 0x03)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t level_idc = manager.Read1Bytes();
    if (!level_idc)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    BitBufferManager bbm;
    if ((ret = bbm.Initialize(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t seq_parameter_set_id = -1;
    if ((ret = avc_read_uev(&bbm, seq_parameter_set_id)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (seq_parameter_set_id < 0)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    int32_t chroma_format_idc = -1;
    int8_t separate_colour_plane_flag = 0;

    if (profile_idc == 100 ||
        profile_idc == 110 ||
        profile_idc == 122 ||
        profile_idc == 244 ||
        profile_idc == 44 ||
        profile_idc == 83 ||
        profile_idc == 86 ||
        profile_idc == 118 ||
        profile_idc == 128)
    {
        if ((ret = avc_read_uev(&bbm, chroma_format_idc)) != ERROR_SUCCESS)
        {
            return ret;
        }

        if (chroma_format_idc == 3)
        {
            if ((ret = avc_read_bit(&bbm, separate_colour_plane_flag)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }

        int32_t bit_depth_luma_minus8 = -1;
        if ((ret = avc_read_uev(&bbm, bit_depth_luma_minus8)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t bit_depth_chroma_minus8 = -1;
        if ((ret = avc_read_uev(&bbm, bit_depth_chroma_minus8)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int8_t qpprime_y_zero_transform_bypass_flag = -1;
        if ((ret = avc_read_bit(&bbm, qpprime_y_zero_transform_bypass_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int8_t seq_scaling_matrix_present_flag = -1;
        if ((ret = avc_read_bit(&bbm, seq_scaling_matrix_present_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if (seq_scaling_matrix_present_flag)
        {
            int nb_scmpfs = ((chroma_format_idc != 3) ? 8 : 12);
            for (int i = 0; i < nb_scmpfs; i++)
            {
                int8_t seq_scaling_matrix_present_flag_i = -1;
                if ((ret = avc_read_bit(&bbm, seq_scaling_matrix_present_flag_i)) != ERROR_SUCCESS)
                {
                    return ret;
                }
            }
        }
    }

    int32_t log2_max_frame_num_minus4 = -1;
    if ((ret = avc_read_uev(&bbm, log2_max_frame_num_minus4)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_order_cnt_type = -1;
    if ((ret = avc_read_uev(&bbm, pic_order_cnt_type)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (pic_order_cnt_type == 0)
    {
        int32_t log2_max_pic_order_cnt_lsb_minus4 = -1;
        if ((ret = avc_read_uev(&bbm, log2_max_pic_order_cnt_lsb_minus4)) != ERROR_SUCCESS)
        {
            return ret;
        }
    }
    else if (pic_order_cnt_type == 1)
    {
        int8_t delta_pic_order_always_zero_flag = -1;
        if ((ret = avc_read_bit(&bbm, delta_pic_order_always_zero_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t offset_for_non_ref_pic = -1;
        if ((ret = avc_read_uev(&bbm, offset_for_non_ref_pic)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t offset_for_top_to_bottom_field = -1;
        if ((ret = avc_read_uev(&bbm, offset_for_top_to_bottom_field)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t num_ref_frames_in_pic_order_cnt_cycle = -1;
        if ((ret = avc_read_uev(&bbm, num_ref_frames_in_pic_order_cnt_cycle)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if (num_ref_frames_in_pic_order_cnt_cycle < 0)
        {
            ret = ERROR_DECODE_H264_FAILED;
            rs_error("decode sps_rbsp failed. ret=%d", ret);
            return ret;
        }
        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            int32_t offset_for_ref_frame_i = -1;
            if ((ret = avc_read_uev(&bbm, offset_for_ref_frame_i)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }
    }

    int32_t max_num_ref_frames = -1;
    if ((ret = avc_read_uev(&bbm, max_num_ref_frames)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int8_t gaps_in_frame_num_value_allowed_flag = -1;
    if ((ret = avc_read_bit(&bbm, gaps_in_frame_num_value_allowed_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_width_in_mbs_minus1 = -1;
    if ((ret = avc_read_uev(&bbm, pic_width_in_mbs_minus1)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_height_in_map_units_minus1 = -1;
    if ((ret = avc_read_uev(&bbm, pic_height_in_map_units_minus1)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int8_t frame_mbs_only_flag = -1;
    int8_t mb_adaptive_frame_field_flag = -1;
    int8_t direct_8x8_inference_flag = -1;
    if ((ret = avc_read_bit(&bbm, frame_mbs_only_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if (!frame_mbs_only_flag &&
        (ret = avc_read_bit(&bbm, mb_adaptive_frame_field_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if ((ret = avc_read_bit(&bbm, direct_8x8_inference_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    int8_t frame_cropping_flag;
    int32_t frame_crop_left_offset = 0;
    int32_t frame_crop_right_offset = 0;
    int32_t frame_crop_top_offset = 0;
    int32_t frame_crop_bottom_offset = 0;
    if ((ret = avc_read_bit(&bbm, frame_cropping_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if (frame_cropping_flag)
    {
        if ((ret = avc_read_uev(&bbm, frame_crop_left_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = avc_read_uev(&bbm, frame_crop_right_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = avc_read_uev(&bbm, frame_crop_top_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = avc_read_uev(&bbm, frame_crop_bottom_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
    }

    width = 16 * (pic_width_in_mbs_minus1 + 1);
    height = 16 * (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1);

    if (separate_colour_plane_flag || chroma_format_idc == 0)
    {
        frame_crop_bottom_offset *= (2 - frame_mbs_only_flag);
        frame_crop_top_offset *= (2 - frame_mbs_only_flag);
    }
    else if (!separate_colour_plane_flag && chroma_format_idc > 0)
    {
        // Width multipliers for formats 1 (4:2:0) and 2 (4:2:2).
        if (chroma_format_idc == 1 || chroma_format_idc == 2)
        {
            frame_crop_left_offset *= 2;
            frame_crop_right_offset *= 2;
        }
        // Height multipliers for format 1 (4:2:0).
        if (chroma_format_idc == 1)
        {
            frame_crop_top_offset *= 2;
            frame_crop_bottom_offset *= 2;
        }
    }

    // Subtract the crop for each dimension.
    width -= (frame_crop_left_offset + frame_crop_right_offset);
    height -= (frame_crop_top_offset + frame_crop_bottom_offset);

    rs_trace("sps parsed, width=%d, height=%d, profile=%d, level=%d, sps_id=%d",
             width,
             height,
             profile_idc,
             level_idc,
             seq_parameter_set_id);

    return ret;
}

int AVInfo::avc_demux_sps()
{
    int ret = ERROR_SUCCESS;

    if (!sps_length)
    {
        return ret;
    }

    BufferManager manager;
    if (manager.Initialize(sps, sps_length) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(1))
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps failed. ret=%d", ret);
        return ret;
    }

    int8_t nutv = manager.Read1Bytes();

    AVCNaluType nal_unit_type = AVCNaluType(nutv & 0x1f);
    if (nal_unit_type != AVCNaluType::SPS)
    {
        ret = ERROR_DECODE_H264_FAILED;
        rs_error("decode sps failed. ret=%d", ret);
        return ret;
    }

    int8_t *rbsp = new int8_t[sps_length];
    rs_auto_freea(int8_t, rbsp);

    int nb_rbsp = 0;
    while (!manager.Empty())
    {
        rbsp[nb_rbsp] = manager.Read1Bytes();
        if (nb_rbsp > 2 &&
            rbsp[nb_rbsp - 2] == 0x00 &&
            rbsp[nb_rbsp - 1] == 0x00 &&
            rbsp[nb_rbsp] == 0x03)
        {
            if (!manager.Empty())
            {
                rbsp[nb_rbsp] = manager.Read1Bytes();
                nb_rbsp++;
                continue;
            }
        }

        nb_rbsp++;
    }

    return avc_demux_sps_rbsp((char *)rbsp, nb_rbsp);
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

    return avc_demux_sps();
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

    if (sample->frame_type == VideoFrameType::VIDEO_INFO_FRAME)
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
        if ((ret = avc_demux_sequence_header(&manager)) != ERROR_SUCCESS)
        {
            return ret;
        }
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