/*
 * @Author: linmin
 * @Date: 2020-02-08 14:08:50
 * @LastEditTime : 2020-02-08 16:32:56
 */
#include <protocol/flv.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>

namespace flv
{
Encoder::Encoder()
{
    writer_ = nullptr;
    nb_tag_headers_ = 0;
    tag_headers_ = nullptr;
    nb_ppts_ = 0;
    ppts_ = nullptr;
    nb_iovss_cache_ = 0;
    iovss_cache_ = nullptr;
}
Encoder::~Encoder()
{
    rs_freep(iovss_cache_);
    rs_freep(ppts_);
    rs_freep(tag_headers_);
}

int Encoder::Initialize(FileWriter *writer)
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

int Encoder::WriteFlvHeader()
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
    if ((ret = WriteFlvHeader(flv_header)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int Encoder::WriteFlvHeader(char flv_header[9])
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

int Encoder::write_tag_header_to_cache(char type, int size, int timestamp, char *cache)
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

int Encoder::write_previous_tag_size_to_cache(int size, char *cache)
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

int Encoder::write_tag(char *header, int header_size, char *tag, int tag_size)
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

int Encoder::WriteMetadata(char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)TagType::SCRIPT, size, 0, tag_header)) != ERROR_SUCCESS)
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

int Encoder::WriteAudio(int64_t timestamp, char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)TagType::AUDIO, size, timestamp, tag_header)) != ERROR_SUCCESS)
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

int Encoder::WriteVideo(int64_t timestamp, char *data, int size)
{
    int ret = ERROR_SUCCESS;

    char tag_header[FLV_TAG_HEADER_SIZE];
    if ((ret = write_tag_header_to_cache((char)TagType::VIDEO, size, timestamp, tag_header)) != ERROR_SUCCESS)
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

int Encoder::SizeTag(int data_size)
{
    return FLV_TAG_HEADER_SIZE + data_size + FLV_PREVIOUS_TAG_SIZE;
}

int Encoder::WriteTags(rtmp::SharedPtrMessage **msgs, int count)
{
    int ret = ERROR_SUCCESS;

    int nb_iovss = 3 * count;
    iovec *iovss = iovss_cache_;

    if (nb_iovss_cache_ < nb_iovss)
    {
        rs_freepa(iovss_cache_);
        nb_iovss_cache_ = nb_iovss;
        iovss_cache_ = iovss = new iovec[nb_iovss];
    }

    char *cache = tag_headers_;
    if (nb_tag_headers_ < count)
    {
        rs_freepa(tag_headers_);
        nb_tag_headers_ = count;
        tag_headers_ = cache = new char(FLV_TAG_HEADER_SIZE * count);
    }

    char *pts = ppts_;
    if (nb_ppts_ < count)
    {
        rs_freepa(ppts_);
        nb_ppts_ = count;
        pts = ppts_ = new char[FLV_PREVIOUS_TAG_SIZE * count];
    }

    iovec *iovs = iovss;
    for (int i = 0; i < count; i++)
    {
        rtmp::SharedPtrMessage *msg = msgs[i];
        if (msg->IsAudio())
        {
            if ((ret = write_tag_header_to_cache(TagType::AUDIO, msg->size, msg->timestamp, cache)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }
        else if (msg->IsVideo())
        {
            if ((ret = write_tag_header_to_cache(TagType::VIDEO, msg->size, msg->timestamp, cache)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }
        else
        {
            if ((ret = write_tag_header_to_cache(TagType::SCRIPT, msg->size, 0, cache)) != ERROR_SUCCESS)
            {
                return ret;
            }
        }

        if ((ret = write_previous_tag_size_to_cache(FLV_TAG_HEADER_SIZE + msg->size, pts)) != ERROR_SUCCESS)
        {
            return ret;
        }

        iovs[0].iov_base = cache;
        iovs[0].iov_len = FLV_TAG_HEADER_SIZE;
        iovs[1].iov_base = msg->payload;
        iovs[1].iov_len = msg->size;
        iovs[2].iov_base = pts;
        iovs[2].iov_len = FLV_PREVIOUS_TAG_SIZE;

        cache += FLV_TAG_HEADER_SIZE;
        pts += FLV_PREVIOUS_TAG_SIZE;
        iovs += 3;
    }

    if ((ret = writer_->Writev(iovss, nb_iovss, nullptr)) != ERROR_SUCCESS)
    {
        if (!IsClientGracefullyClose(ret))
        {
            rs_error("write flv tags failed. ret=%d", ret);
        }
        return ret;
    }
    return ret;
}

} // namespace flv