#include <codec/avc.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/buffer.hpp>
#include <common/utils.hpp>

namespace avc
{
    
std::string profile_to_str(Profile profile)
{
    switch (profile)
    {
    case Profile::BASELINE:
        return "Baseline";
    case Profile::CONSTRAINED_BASELINE:
        return "Constrained Baseline";
    case Profile::MAIN:
        return "Main";
    case Profile::EXTENDED:
        return "Extended";
    case Profile::HIGH:
        return "High";
    case Profile::HIGH_10:
        return "High10";
    case Profile::HIGH_10_INTRA:
        return "High10 Intra";
    case Profile::HIGH_422:
        return "High422";
    case Profile::HIGH_422_INTRA:
        return "High422 Intra";
    case Profile::HIGH_444:
        return "High444";
    case Profile::HIGH_444_PREDICTIVE:
        return "High444 Predictive";
    case Profile::HIGH_444_INTRA:
        return "High444 Intra";
    default:
        return "Unknow";
    }
}

std::string level_to_str(Level level)
{
    switch (level)
    {
    case Level::LEVEL_1:
        return "Level 1";
    case Level::LEVEL_11:
        return "Level 1.1";
    case Level::LEVEL_12:
        return "Level 1.2";
    case Level::LEVEL_13:
        return "Level 1.3";
    case Level::LEVEL_2:
        return "Level 2";
    case Level::LEVEL_21:
        return "Level 2.1";
    case Level::LEVEL_22:
        return "Level 2.2";
    case Level::LEVEL_3:
        return "Level 2.3";
    case Level::LEVEL_31:
        return "Level 3.1";
    case Level::LEVEL_32:
        return "Level 3.2";
    case Level::LEVEL_4:
        return "Level 4";
    case Level::LEVEL_41:
        return "Level 4.1";
    case Level::LEVEL_42:
        return "Level 4.2";
    case Level::LEVEL_5:
        return "Level 5";
    case Level::LEVEL_51:
        return "Level 5.1";
    case Level::LEVEL_52:
        return "Level 5.2";
    default:
        return "Unknow";
    }
}

} // namespace avc

// for SPS, 7.3.2.1.1 Sequence parameter set data syntax
// H.264-AVC-ISO_IEC_14496-10-2012.pdf, page 62.
int AVCCodec::demux_sps_rbsp(char *rbsp, int nb_rbsp)
{
    int ret = ERROR_SUCCESS;

    BufferManager manager;
    if (manager.Initialize(rbsp, nb_rbsp) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (!manager.Require(3))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t profile_idc = manager.Read1Bytes();
    if (!profile_idc)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t flags = manager.Read1Bytes();
    if (flags & 0x03)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    uint8_t level_idc = manager.Read1Bytes();
    if (!level_idc)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sps_rbsp failed. ret=%d", ret);
        return ret;
    }

    BitBufferManager bbm;
    if ((ret = bbm.Initialize(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t seq_parameter_set_id = -1;
    if ((ret = Utils::ReadUEV(&bbm, seq_parameter_set_id)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (seq_parameter_set_id < 0)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
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
        if ((ret = Utils::ReadUEV(&bbm, chroma_format_idc)) != ERROR_SUCCESS)
        {
            return ret;
        }

        if (chroma_format_idc == 3)
        {
            if ((ret = Utils::ReadBit(&bbm, separate_colour_plane_flag)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }

        int32_t bit_depth_luma_minus8 = -1;
        if ((ret = Utils::ReadUEV(&bbm, bit_depth_luma_minus8)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t bit_depth_chroma_minus8 = -1;
        if ((ret = Utils::ReadUEV(&bbm, bit_depth_chroma_minus8)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int8_t qpprime_y_zero_transform_bypass_flag = -1;
        if ((ret = Utils::ReadBit(&bbm, qpprime_y_zero_transform_bypass_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int8_t seq_scaling_matrix_present_flag = -1;
        if ((ret = Utils::ReadBit(&bbm, seq_scaling_matrix_present_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if (seq_scaling_matrix_present_flag)
        {
            int nb_scmpfs = ((chroma_format_idc != 3) ? 8 : 12);
            for (int i = 0; i < nb_scmpfs; i++)
            {
                int8_t seq_scaling_matrix_present_flag_i = -1;
                if ((ret = Utils::ReadBit(&bbm, seq_scaling_matrix_present_flag_i)) != ERROR_SUCCESS)
                {
                    return ret;
                }
            }
        }
    }

    int32_t log2_max_frame_num_minus4 = -1;
    if ((ret = Utils::ReadUEV(&bbm, log2_max_frame_num_minus4)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_order_cnt_type = -1;
    if ((ret = Utils::ReadUEV(&bbm, pic_order_cnt_type)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (pic_order_cnt_type == 0)
    {
        int32_t log2_max_pic_order_cnt_lsb_minus4 = -1;
        if ((ret = Utils::ReadUEV(&bbm, log2_max_pic_order_cnt_lsb_minus4)) != ERROR_SUCCESS)
        {
            return ret;
        }
    }
    else if (pic_order_cnt_type == 1)
    {
        int8_t delta_pic_order_always_zero_flag = -1;
        if ((ret = Utils::ReadBit(&bbm, delta_pic_order_always_zero_flag)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t offset_for_non_ref_pic = -1;
        if ((ret = Utils::ReadUEV(&bbm, offset_for_non_ref_pic)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t offset_for_top_to_bottom_field = -1;
        if ((ret = Utils::ReadUEV(&bbm, offset_for_top_to_bottom_field)) != ERROR_SUCCESS)
        {
            return ret;
        }

        int32_t num_ref_frames_in_pic_order_cnt_cycle = -1;
        if ((ret = Utils::ReadUEV(&bbm, num_ref_frames_in_pic_order_cnt_cycle)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if (num_ref_frames_in_pic_order_cnt_cycle < 0)
        {
            ret = ERROR_CODEC_DECODE_AVC_FAILED;
            rs_error("decode sps_rbsp failed. ret=%d", ret);
            return ret;
        }
        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            int32_t offset_for_ref_frame_i = -1;
            if ((ret = Utils::ReadUEV(&bbm, offset_for_ref_frame_i)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }
    }

    int32_t max_num_ref_frames = -1;
    if ((ret = Utils::ReadUEV(&bbm, max_num_ref_frames)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int8_t gaps_in_frame_num_value_allowed_flag = -1;
    if ((ret = Utils::ReadBit(&bbm, gaps_in_frame_num_value_allowed_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_width_in_mbs_minus1 = -1;
    if ((ret = Utils::ReadUEV(&bbm, pic_width_in_mbs_minus1)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int32_t pic_height_in_map_units_minus1 = -1;
    if ((ret = Utils::ReadUEV(&bbm, pic_height_in_map_units_minus1)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int8_t frame_mbs_only_flag = -1;
    int8_t mb_adaptive_frame_field_flag = -1;
    int8_t direct_8x8_inference_flag = -1;
    if ((ret = Utils::ReadBit(&bbm, frame_mbs_only_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if (!frame_mbs_only_flag &&
        (ret = Utils::ReadBit(&bbm, mb_adaptive_frame_field_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if ((ret = Utils::ReadBit(&bbm, direct_8x8_inference_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    int8_t frame_cropping_flag;
    int32_t frame_crop_left_offset = 0;
    int32_t frame_crop_right_offset = 0;
    int32_t frame_crop_top_offset = 0;
    int32_t frame_crop_bottom_offset = 0;
    if ((ret = Utils::ReadBit(&bbm, frame_cropping_flag)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if (frame_cropping_flag)
    {
        if ((ret = Utils::ReadUEV(&bbm, frame_crop_left_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = Utils::ReadUEV(&bbm, frame_crop_right_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = Utils::ReadUEV(&bbm, frame_crop_top_offset)) != ERROR_SUCCESS)
        {
            return ret;
        }
        if ((ret = Utils::ReadUEV(&bbm, frame_crop_bottom_offset)) != ERROR_SUCCESS)
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

int AVCCodec::demux_sps()
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
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sps failed. ret=%d", ret);
        return ret;
    }

    int8_t nutv = manager.Read1Bytes();

    avc::NaluType nal_unit_type = avc::NaluType(nutv & 0x1f);
    if (nal_unit_type != avc::NaluType::SPS)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
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

    return demux_sps_rbsp((char *)rbsp, nb_rbsp);
}

bool AVCCodec::HasSequenceHeader()
{
    return extradata_size > 0 && extradata;
}

int AVCCodec::DecodeSequenceHeader(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    extradata_size = manager->Size() - manager->Pos();

    if (extradata_size > 0)
    {
        rs_freepa(extradata);
        extradata = new char[extradata_size];
        memcpy(extradata, manager->Data() + manager->Pos(), extradata_size);
    }

    if (!manager->Require(6))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    // H.264-AVC-ISO_IEC_14496-15.pdf page16
    // avc sequence header
    manager->Read1Bytes();
    profile = (avc::Profile)manager->Read1Bytes();
    manager->Read1Bytes();
    level = (avc::Level)manager->Read1Bytes();

    int8_t length_size_minus_one = manager->Read1Bytes();
    length_size_minus_one &= 0x03;

    // lengthSizeMinusOne indicates the length in bytes of the NALUnitLength field in an AVC video
    // sample or AVC parameter set sample of the associated stream minus one. For example, a size of one
    // byte is indicated with a value of 0. The value of this field shall be one of 0, 1, or 3 corresponding to a
    // length encoded with 1, 2, or 4 bytes, respectively.

    length_size_minus_one = length_size_minus_one;
    if (length_size_minus_one == 2)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("seqence should never be 2. ret=%d", ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    int8_t num_of_sps = manager->Read1Bytes();
    num_of_sps &= 0x1f;

    if (num_of_sps != 1)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    if (!manager->Require(2))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    sps_length = manager->Read2Bytes();
    if (!manager->Require(sps_length))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
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
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    int8_t num_of_pps = manager->Read1Bytes();
    num_of_pps &= 0x1f;

    if (num_of_pps != 1)
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    pps_length = manager->Read2Bytes();
    if (!manager->Require(pps_length))
    {
        ret = ERROR_CODEC_DECODE_AVC_FAILED;
        rs_error("decode sequence header failed. ret=%d", ret);
        return ret;
    }

    if (pps_length > 0)
    {
        rs_freepa(pps);
        pps = new char[pps_length];
        manager->ReadBytes(pps, pps_length);
    }

    if ((ret = demux_sps()) != ERROR_SUCCESS)
    {
        return ret;
    }

    rs_trace("avc extradata parsed. profile=%s, level=%s, width=%d, height=%d",
             avc::profile_to_str(profile).c_str(),
             avc::level_to_str(level).c_str(),
             width,
             height);

    return ret;
}

bool AVCCodec::start_with_annexb(BufferManager *manager, int *pnb_start_code)
{
    char *bytes = manager->Data() + manager->Pos();
    char *p = bytes;

    while (true)
    {
        if (!manager->Require(p - bytes + 3))
        {
            return false;
        }

        if (p[0] != (char)0x00 && p[1] != (char)0x00)
        {
            return false;
        }

        if (p[2] == (char)0x01)
        {
            if (pnb_start_code)
            {
                *pnb_start_code = (int)(p - bytes) + 3;
            }
            return true;
        }

        p++;
    }

    return false;
}

int AVCCodec::demux_annexb_format(BufferManager *manager, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    if (!start_with_annexb(manager, nullptr))
    {
        return ERROR_CODEC_AVC_TRY_OTHERS;
    }

    while (!manager->Empty())
    {
        int nb_start_code = 0;
        if (!start_with_annexb(manager, &nb_start_code))
        {
            return ret;
        }

        if (nb_start_code > 0)
        {
            manager->Skip(nb_start_code);
        }

        char *p = manager->Data() + manager->Pos();

        while (!manager->Empty())
        {
            if (start_with_annexb(manager, nullptr))
            {
                break;
            }

            manager->Skip(1);
        }

        char *pp = manager->Data() + manager->Pos();

        if (pp - p <= 0)
        {
            continue;
        }

        if ((ret = sample->AddSampleUnit(p, pp - p)) != ERROR_SUCCESS)
        {
            rs_error("avc add video sample failed. ret=%d", ret);
            return ret;
        }
    }

    return ret;
}

int AVCCodec::demux_ibmf_format(BufferManager *manager, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    int picture_length = manager->Size() - manager->Pos();

    for (int i = 0; i < picture_length; i++)
    {
        if (!manager->Require(length_size_minus_one + 1))
        {
            ret = ERROR_CODEC_DECODE_AVC_FAILED;
            rs_error("avc decode nalu size failed. ret=%d", ret);
            return ret;
        }

        int32_t nalu_unit_length = 0;

        if (length_size_minus_one == 3)
        {
            nalu_unit_length = manager->Read4Bytes();
        }
        else if (length_size_minus_one == 1)
        {
            nalu_unit_length = manager->Read2Bytes();
        }
        else
        {
            nalu_unit_length = manager->Read1Bytes();
        }

        if (nalu_unit_length < 0)
        {
            ret = ERROR_CODEC_DECODE_AVC_FAILED;
            rs_error("maybe stream is annexb format. ret=%d", ret);
            return ret;
        }

        if (!manager->Require(nalu_unit_length))
        {
            ret = ERROR_CODEC_AVC_TRY_OTHERS;
            return ret;
        }

        if ((ret = sample->AddSampleUnit(manager->Data() + manager->Pos(), nalu_unit_length)) != ERROR_SUCCESS)
        {
            rs_error("avc add video sample failed. ret=%d", ret);
        }

        manager->Skip(nalu_unit_length);

        i += length_size_minus_one + 1 + nalu_unit_length;
    }

    return ret;
}

int AVCCodec::DecodecNalu(BufferManager *manager, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    if (HasSequenceHeader())
    {
        rs_warn("avc ignore type=%d for no sequence header", (int8_t)avc::NaluType::NON_IDR);
        return ret;
    }

    if (payload_format == avc::PayloadFormat::GUESS || payload_format == avc::PayloadFormat::ANNEXB)
    {
        if ((ret = demux_annexb_format(manager, sample)) != ERROR_SUCCESS)
        {
            if (ret != ERROR_CODEC_AVC_TRY_OTHERS)
            {
                rs_error("avc demux annexb failed. ret=%d", ret);
                return ret;
            }

            if ((ret = demux_ibmf_format(manager, sample)) != ERROR_SUCCESS)
            {
                if (ret == ERROR_CODEC_AVC_TRY_OTHERS)
                {
                    ret = ERROR_CODEC_DECODE_AVC_FAILED;
                }
                rs_error("avc decode nalu failed. ret=%d", ret);
                return ret;
            }
            else
            {
                payload_format = avc::PayloadFormat::IBMF;
            }
        }
        else
        {
            payload_format = avc::PayloadFormat::ANNEXB;
        }
    }
    else
    {
        if ((ret = demux_ibmf_format(manager, sample)) != ERROR_SUCCESS)
        {
            if (ret != ERROR_CODEC_AVC_TRY_OTHERS)
            {
                rs_error("avc demux ibmf failed. ret=%d", ret);
                return ret;
            }
            if ((ret = demux_annexb_format(manager, sample)) != ERROR_SUCCESS)
            {
                if (ret == ERROR_CODEC_AVC_TRY_OTHERS)
                {
                    ret = ERROR_CODEC_DECODE_AVC_FAILED;
                }
                rs_error("avc decode nalu failed. ret=%d", ret);
                return ret;
            }
            else
            {
                payload_format = avc::PayloadFormat::ANNEXB;
            }
        }
        else
        {
            payload_format = avc::PayloadFormat::IBMF;
        }
    }

    return ret;
}

AVCCodec::AVCCodec()
{
    profile = avc::Profile::UNKNOW;
    level = avc::Level::UNKNOW;
    payload_format = avc::PayloadFormat::GUESS;
    extradata_size = 0;
    extradata = nullptr;
    sps_length = 0;
    sps = nullptr;
    pps_length = 0;
    pps = nullptr;
    width = 0;
    height = 0;
    length_size_minus_one = 0;
}

AVCCodec::~AVCCodec()
{
    rs_freepa(extradata);
    rs_freepa(sps);
    rs_freepa(pps);
}