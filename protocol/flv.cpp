#include <protocol/flv.hpp>

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
} // namespace flv