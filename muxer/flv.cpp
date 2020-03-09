#include <muxer/flv.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>
#include <codec/aac.hpp>
#include <codec/avc.hpp>

namespace flv
{
std::string sample_rate_to_str(AudioSampleRate sample_rate)
{
    switch (sample_rate)
    {
    case AudioSampleRate::SAMPLE_RATE_11025:
        return "11025";
    case AudioSampleRate::SAMPLE_RATE_22050:
        return "22050";
    case AudioSampleRate::SAMPLE_RATE_5512:
        return "5512";
    case AudioSampleRate::SAMPLE_RATE_44100:
        return "44100";
    default:
        return "Unkow";
    }
}

std::string audio_codec_type_to_str(AudioCodecType code_type)
{
    switch (code_type)
    {
    case AudioCodecType::LINEAR_PCM_PLATFORM_ENDIAN:
        return "LINEAR_PCM_PLATFORM_ENDIAN";
    case AudioCodecType::ADPCM:
        return "ADPCM";
    case AudioCodecType::MP3:
        return "MP3";
    case AudioCodecType::LINEAR_PCM_LITTLE_ENDIAN:
        return "LINEAR_PCM_LITTLE_ENDIAN";
    case AudioCodecType::NELLY_MOSER_16KHZ_MONO:
        return "NELLY_MOSER_16KHZ_MONO";
    case AudioCodecType::NELLY_MOSER_8KHZ_MONO:
        return "NELLY_MOSER_8KHZ_MONO";
    case AudioCodecType::NELLY_MOSER:
        return "NELLY_MOSER";
    case AudioCodecType::AAC:
        return "AAC";
    case AudioCodecType::SPEEX:
        return "SPEEX";
    case AudioCodecType::MP3_8KHZ:
        return "MP3_8KHZ";
    case AudioCodecType::DEVICE_SPECIFIC_SOUND:
        return "DEVICE_SPECIFIC_SOUND";
    default:
        return "Unkonw";
    }
}

std::string sound_type_to_str(AudioSoundType sound_type)
{
    switch (sound_type)
    {
    case AudioSoundType::MONO:
        return "Mono";
    case AudioSoundType::STEREO:
        return "Stereo";
    default:
        return "Unkonw";
    }
}

std::string sound_size_to_str(AudioSoundSize sound_size)
{
    switch (sound_size)
    {
    case AudioSoundSize::BIT_DEPTH_16BIT:
        return "16Bit";
    case AudioSoundSize::BIT_DEPTH_8BIT:
        return "8Bit";
    default:
        return "Unknow";
    }
}

} // namespace flv

FlvCodecSample::FlvCodecSample()
{
    is_video = false;
    acodec_type = flv::AudioCodecType::UNKNOW;
    sound_type = flv::AudioSoundType::UNKNOW;
    sound_size = flv::AudioSoundSize::UNKNOW;
    sample_rate = flv::AudioSampleRate::UNKNOW;
    aac_pkt_type = flv::AACPacketType::UNKNOW;
    has_print = false;
}

FlvCodecSample::~FlvCodecSample()
{
}

int FlvMuxer::write_previous_tag_size_to_cache(int size, char *cache)
{
    int ret = ERROR_SUCCESS;

    BufferManager manager;
    if ((ret = manager.Initialize(cache, size)) != ERROR_SUCCESS)
    {
        return ret;
    }
    manager.Write4Bytes(size);
    return ret;
}

int FlvMuxer::write_tag(char *header, int header_size, char *tag, int tag_size)
{
    int ret = ERROR_SUCCESS;

    char pre_size[FLV_PREVIOUS_TAG_SIZE];
    if ((ret = write_previous_tag_size_to_cache(header_size + tag_size, pre_size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    iovec iovs[3];
    iovs[0].iov_base = header;
    iovs[0].iov_len = header_size;
    iovs[1].iov_base = tag;
    iovs[1].iov_len = tag_size;
    iovs[2].iov_base = pre_size;
    iovs[2].iov_len = FLV_PREVIOUS_TAG_SIZE;

    if ((ret = writer_->Writev(iovs, 3, nullptr)) != ERROR_SUCCESS)
    {
        if (!IsClientGracefullyClose(ret))
        {
            rs_error("write flv tag failed. ret=%d", ret);
        }
        return ret;
    }
    return ret;
}

int FlvMuxer::write_tag_header_to_cache(char type, int size, int timestamp, char *cache)
{
    int ret = ERROR_SUCCESS;

    timestamp &= 0x7fffffff;

    BufferManager manager;
    if (manager.Initialize(cache, 11) != ERROR_SUCCESS)
    {
        return ret;
    }

    manager.Write1Bytes(type);
    manager.Write3Bytes(size);
    manager.Write3Bytes((int32_t)timestamp);
    manager.Write1Bytes((timestamp >> 24) & 0xff);
    manager.Write3Bytes(0x00);

    return ret;
}

int FlvMuxer::write_flv_header(char flv_header[9])
{
    int ret = ERROR_SUCCESS;

    if ((ret = writer_->Write(flv_header, 9, nullptr)) != ERROR_SUCCESS)
    {
        rs_error("write flv header failed. ret=%d", ret);
        return ret;
    }

    char previous_tag_size[] = {(char)0x00, (char)0x00, (char)0x00, (char)0x00};
    if ((ret = writer_->Write(previous_tag_size, 4, nullptr)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int FlvMuxer::WriteMuxerHeader()
{
    int ret = ERROR_SUCCESS;

    char flv_header[] = {
        'F',
        'L',
        'V',
        (char)0x01,
        (char)0x05,
        (char)0x00,
        (char)0x00,
        (char)0x00,
        (char)0x09};
    if ((ret = write_flv_header(flv_header)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

FlvMuxer::FlvMuxer()
{
    writer_ = nullptr;
    nb_tag_headers_ = 0;
    tag_headers_ = nullptr;
    nb_ppts_ = 0;
    ppts_ = nullptr;
    nb_iovss_cache_ = 0;
    iovss_cache_ = nullptr;
}
FlvMuxer::~FlvMuxer()
{
    rs_freepa(iovss_cache_);
    rs_freepa(ppts_);
    rs_freepa(tag_headers_);
}

int FlvMuxer::SizeTag(int data_size)
{
    return FLV_TAG_HEADER_SIZE + data_size + FLV_PREVIOUS_TAG_SIZE;
}

int FlvMuxer::Initialize(FileWriter *writer)
{
    int ret = ERROR_SUCCESS;

    if (!writer->IsOpen())
    {
        ret = ERROR_KERNEL_FLV_STREAM_CLOSED;
        rs_warn("stream is not open for encoder. ret=%d", ret);
        return ret;
    }
    writer_ = writer;

    return ret;
}

int FlvMuxer::WriteMetadata(char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)flv::TagType::SCRIPT, size, 0, tag_header)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS)
    {
        if (!IsClientGracefullyClose(ret))
        {
            rs_error("write flv metadata tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int FlvMuxer::WriteAudio(int64_t timestamp, char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)flv::TagType::AUDIO, size, timestamp, tag_header)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS)
    {
        if (!IsClientGracefullyClose(ret))
        {
            rs_error("write flv audio tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int FlvMuxer::WriteVideo(int64_t timestamp, char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)flv::TagType::VIDEO, size, timestamp, tag_header)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS)
    {
        if (!IsClientGracefullyClose(ret))
        {
            rs_error("write flv video tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

FlvDemuxer::FlvDemuxer()
{
    vcodec_type = flv::VideoCodecType::UNKNOW;
    vcodec = nullptr;
    acodec_type = flv::AudioCodecType::UNKNOW;
    acodec = nullptr;
}

FlvDemuxer::~FlvDemuxer()
{
    rs_freep(vcodec);
    rs_freep(acodec);
}

int FlvDemuxer::demux_aac(BufferManager *manager, FlvCodecSample *sample)
{
    int ret = ERROR_SUCCESS;

    if (!acodec)
    {
        rs_freep(acodec);
        acodec = new AACCodec;
    }

    AACCodec *aac_codec = dynamic_cast<AACCodec *>(acodec);
    if (!aac_codec)
    {
        ret = ERROR_CODEC_ACODEC_TYPE_ERROR;
        rs_error("audio codec type error. ret=%d", ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_MUXER_DEMUX_FLV_DEMUX_FAILED;
        rs_error("flv decode aac_packet_type failed. ret=%d", ret);
        return ret;
    }

    sample->aac_pkt_type = (flv::AACPacketType)manager->Read1Bytes();
    switch (sample->aac_pkt_type)
    {
    case flv::AACPacketType::SEQUENCE_HEADER:
    {
        aac_codec->extradata_size = manager->Size() - manager->Pos();
        if (aac_codec->extradata_size > 0)
        {
            rs_freepa(aac_codec->extradata);
            aac_codec->extradata = new char[aac_codec->extradata_size];
            memcpy(aac_codec->extradata, manager->Data() + manager->Pos(), aac_codec->extradata_size);
        }
        aac_codec->DecodeSequenceHeader();
        break;
    }
    case flv::AACPacketType::RAW_DATA:
    {
        if (!aac_codec->HasSequenceHeader())
        {
            rs_warn("aac ignore type=%d for no sequence header. ret=%d", sample->aac_pkt_type, ret);
            return ret;
        }

        if ((ret = sample->AddSampleUnit(manager->Data() + manager->Pos(), manager->Size() - manager->Pos())) != ERROR_SUCCESS)
        {
            rs_error("add aac sample failed. ret=%d", ret);
            return ret;
        }
        break;
    }

    default:
        ret = ERROR_MUXER_DEMUX_FLV_DEMUX_FAILED;
        rs_error("flv aac_packet_type error. ret=%d", ret);
        return ret;
    }

    return ret;
}

int FlvDemuxer::DemuxVideo(char *data, int size, CodecSample *s)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int FlvDemuxer::DemuxAudio(char *data, int size, CodecSample *s)
{
    int ret = ERROR_SUCCESS;

    if (!data || size <= 0)
    {
        // audio has not data
        return ret;
    }

    if (!dynamic_cast<FlvCodecSample *>(s))
    {
        ret = ERROR_CODEC_SAMPLE_TYPE_ERROR;
        rs_error("codec sample type is not FlvCodecSample. ret=%d", ret);
        return ret;
    }

    BufferManager manager;
    if ((ret = manager.Initialize(data, size)) != ERROR_SUCCESS)
    {
        return ret;
    }
    if (!manager.Require(1))
    {
        ret = ERROR_MUXER_DEMUX_FLV_DEMUX_FAILED;
        rs_error("flv deocde audio sound_size failed. ret=%d", ret);
        return ret;
    }

    int temp = manager.Read1Bytes();
    int sound_type = temp & 0x01;
    int sound_size = (temp >> 1) & 0x01;
    int sample_rate = (temp >> 2) & 0x03;
    acodec_type = (flv::AudioCodecType)((temp >> 4) & 0x0f);

    FlvCodecSample *sample = dynamic_cast<FlvCodecSample *>(s);
    sample->is_video = false;
    sample->acodec_type = acodec_type;
    sample->sound_type = (flv::AudioSoundType)sound_type;
    sample->sound_size = (flv::AudioSoundSize)sound_size;
    sample->sample_rate = (flv::AudioSampleRate)sample_rate;

    if (!sample->has_print)
    {
        sample->has_print = true;
        rs_trace("flv audio data parsed. codec=%s, sound_type=%s, sound_size=%s, sample_rate=%s",
                 flv::audio_codec_type_to_str(sample->acodec_type).c_str(),
                 flv::sound_type_to_str(sample->sound_type).c_str(),
                 flv::sound_size_to_str(sample->sound_size).c_str(),
                 flv::sample_rate_to_str(sample->sample_rate).c_str());
    }

    switch (sample->acodec_type)
    {
    case flv::AudioCodecType::AAC:
        return demux_aac(&manager, sample);
    default:
        ret = ERROR_CODEC_UNSUPPORT;
        rs_error("codec %s is not support yet. ret=%d", flv::audio_codec_type_to_str(sample->acodec_type).c_str(), ret);
        return ret;
    }
}

bool FlvDemuxer::IsAVC(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0] & 0x0f;

    return codec_id == (char)flv::VideoCodecType::AVC;
}

bool FlvDemuxer::IsAAC(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char codec_id = data[0];
    codec_id = (codec_id >> 4) & 0x0f;

    return codec_id == (char)flv::AudioCodecType::AAC;
}

bool FlvDemuxer::IsAVCSequenceHeader(char *data, int size)
{
    if (!IsAVC(data, size))
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

    return frame_type == (char)flv::VideoFrameType::KEY_FRAME &&
           packet_type == (char)flv::VideoPacketType::SEQUENCE_HEADER;
}

bool FlvDemuxer::IsAACSequenceHeader(char *data, int size)
{
    if (!IsAAC(data, size))
    {
        return false;
    }

    if (size < 2)
    {
        return false;
    }

    char packet_type = data[1];

    return packet_type == (char)flv::AACPacketType::SEQUENCE_HEADER;
}

bool FlvDemuxer::IsKeyFrame(char *data, int size)
{
    if (size < 1)
    {
        return false;
    }

    char frame_type = data[0];
    frame_type = (frame_type >> 4) & 0x0F;

    return frame_type == (char)flv::VideoFrameType::KEY_FRAME;
}