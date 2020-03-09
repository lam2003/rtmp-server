#include <codec/aac.hpp>
#include <common/log.hpp>
#include <common/buffer.hpp>
#include <common/error.hpp>
#include <common/utils.hpp>

namespace aac
{
std::string object_type_to_str(aac::ObjectType object_type)
{
    switch (object_type)
    {
    case aac::ObjectType::HE:
        return "HE";
    case aac::ObjectType::LC:
        return "LC";
    case aac::ObjectType::HEV2:
        return "HEv2";
    case aac::ObjectType::MAIN:
        return "MAIN";
    case aac::ObjectType::SSR:
        return "SSR";
    default:
        return "Unknow";
    }
}

std::string sample_rate_to_str(int sample_rate_idx)
{
    static const char *aac_sample_rates_str[] = {
        "96000", "88200", "64000",
        "48000", "44100", "32000",
        "24000", "22050", "16000",
        "12000", "11025", "8000",
        "0", "0", "0", "0"};

    if (sample_rate_idx >= (int)(sizeof(aac_sample_rates_str) / sizeof(char *)))
    {
        return "Unknow";
    }

    return aac_sample_rates_str[sample_rate_idx];
}

std::string channel_to_str(int channel)
{
    switch (channel)
    {
    case 1:
        return "1";
    case 2:
        return "2";
    case 3:
        return "3";
    case 4:
        return "4";
    case 5:
        return "5";
    case 6:
        return "5.1";
    case 7:
        return "7.1";
    default:
        return "Unknow";
    }
}
} // namespace aac

AACCodec::AACCodec()
{
    extradata_size = 0;
    extradata = nullptr;
    channels = -1;
    sample_rate = -1;
    object_type = aac::ObjectType::UNKNOW;
}

AACCodec::~AACCodec()
{
    rs_freepa(extradata);
}

bool AACCodec::HasSequenceHeader()
{
    return extradata_size > 0 && extradata;
}

// support mpeg4-aac
// 1.6.2.1 AudioSpecificConfig, in aac-mp4a-format-ISO_IEC_14496-3+2001.pdf, page 33.
int AACCodec::DecodeSequenceHeader(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    extradata_size = manager->Size() - manager->Pos();
    if (extradata_size > 0)
    {
        rs_freepa(extradata);
        extradata = new char[extradata_size];
        memcpy(extradata, manager->Data() + manager->Pos(), extradata_size);
    }

    if (!manager->Require(2))
    {
        ret = ERROR_CODEC_AAC_DECODE_EXTRADATA_FAILED;
        rs_error("aac decode extradata failed. ret=%d", ret);
        return ret;
    }

    uint8_t temp1 = manager->Read1Bytes();
    uint8_t temp2 = manager->Read1Bytes();

    channels = (temp2 >> 3) & 0x0f;
    sample_rate = ((temp1 & 0x07) << 1) | ((temp2 >> 7) & 0x01);
    object_type = (aac::ObjectType)((temp1 >> 3) & 0x1f);

    rs_trace("aac extradata parsed. profile=%s, sample_rate=%sHz, channel=%s",
             aac::object_type_to_str(object_type).c_str(),
             aac::sample_rate_to_str(sample_rate).c_str(),
             aac::channel_to_str(channels).c_str());

    return ret;
}

int AACCodec::DecodeRawData(BufferManager *manager, CodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    if (!HasSequenceHeader())
    {
        rs_warn("aac ignore raw data before sequence header");
        return ret;
    }

    if ((ret = sample->AddSampleUnit(manager->Data() + manager->Pos(), manager->Size() - manager->Pos())) != ERROR_SUCCESS)
    {
        rs_error("add aac sample failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}