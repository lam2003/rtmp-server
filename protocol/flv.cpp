#include <protocol/flv.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/buffer.hpp>

namespace flv
{
bool Codec::is_h264(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0];
    codec_id &= 0x0f;

    return codec_id == (char)VideoCodecType::AVC;
}

bool Codec::is_aac(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0];
    codec_id &= 0x0f;
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

int Codec::DemuxAudio(char *data, int size, CodecSampleUnit *sample)
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

    int sound_type = data[0] & 0x01;
    int sound_size = (data[0] >> 1)0x01;
    int sound_rate = (data[0] >> 2) & 0x03;
    int sound_format = (data[0] >> 4) & 0x0f;

    return ret;
}
} // namespace flv