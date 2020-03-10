/*
 * @Author: linmin
 * @Date: 2020-02-17 12:57:29
 * @LastEditTime: 2020-02-18 12:54:40
 */
#include <protocol/rtmp/message.hpp>
#include <protocol/rtmp/defines.hpp>
#include <common/utils.hpp>
#include <common/log.hpp>
#include <common/error.hpp>

namespace rtmp
{

int ChunkHeaderC0(int perfer_cid,
                  uint32_t timestamp,
                  int32_t payload_length,
                  int8_t message_type,
                  int32_t stream_id,
                  char *buf)
{
    char *pp = nullptr;
    char *p = buf;

    *p++ = 0x00 | (0x3f & perfer_cid);
    if (timestamp < RTMP_EXTENDED_TIMESTAMP)
    {
        pp = (char *)&timestamp;
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }
    else
    {
        *p++ = 0xff;
        *p++ = 0xff;
        *p++ = 0xff;
    }

    pp = (char *)&payload_length;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    *p++ = message_type;

    //little-endian
    pp = (char *)&stream_id;
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
    *p++ = pp[3];

    if (timestamp >= RTMP_EXTENDED_TIMESTAMP)
    {
        pp = (char *)&timestamp;
        *p++ = pp[3];
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }

    return p - buf;
}

int ChunkHeaderC3(int perfer_cid, uint32_t timestamp, char *buf)
{
    char *pp = nullptr;
    char *p = buf;

    *p++ = 0xC0 | (0x3f & perfer_cid);

    if (timestamp >= RTMP_EXTENDED_TIMESTAMP)
    {
        pp = (char *)&timestamp;
        *p++ = pp[3];
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }

    return p - buf;
}

MessageHeader::MessageHeader()
{
    timestamp_delta = 0;
    payload_length = 0;
    message_type = 0;
    stream_id = 0;
    timestamp = 0;
    perfer_cid = 0;
}

MessageHeader::~MessageHeader()
{
}

bool MessageHeader::IsAudio()
{
    if (message_type == RTMP_MSG_AUDIO_MESSAGE)
        return true;
    return false;
}
bool MessageHeader::IsVideo()
{

    if (message_type == RTMP_MSG_VIDEO_MESSAGE)
        return true;
    return false;
}
bool MessageHeader::IsAMF0Command()
{
    if (message_type == RTMP_MSG_AMF0_COMMAND)
        return true;
    return false;
}
bool MessageHeader::IsAMF0Data()
{
    if (message_type == RTMP_MSG_AMF0_DATA)
        return true;
    return false;
}
bool MessageHeader::IsAMF3Command()
{
    if (message_type == RTMP_MSG_AMF3_COMMAND)
        return true;
    return false;
}
bool MessageHeader::IsAMF3Data()
{
    if (message_type == RTMP_MSG_AMF3_DATA)
        return true;
    return false;
}
bool MessageHeader::IsWindowAckledgementSize()
{
    if (message_type == RTMP_MSG_WINDOW_ACK_SIZE)
        return true;
    return false;
}

bool MessageHeader::IsAckledgement()
{
    if (message_type == RTMP_MSG_ACK)
        return true;
    return false;
}
bool MessageHeader::IsSetChunkSize()
{
    if (message_type == RTMP_MSG_SET_CHUNK_SIZE)
        return true;
    return false;
}
bool MessageHeader::IsUserControlMessage()
{
    if (message_type == RTMP_MSG_USER_CONTROL_MESSAGE)
        return true;
    return false;
}
bool MessageHeader::IsSetPeerBandWidth()
{
    if (message_type == RTMP_MSG_SET_PEER_BANDWIDTH)
        return true;
    return false;
}
bool MessageHeader::IsAggregate()
{
    if (message_type == RTMP_MSG_AGGREGATE)
        return true;
    return false;
}
void MessageHeader::InitializeAMF0Script(int32_t size, int32_t stream)
{
    message_type = RTMP_MSG_AMF0_DATA;
    payload_length = size;
    timestamp_delta = 0;
    timestamp = 0;
    stream_id = stream;
    perfer_cid = RTMP_CID_OVER_CONNECTION2;
}
void MessageHeader::InitializeVideo(int32_t size, uint32_t time, int32_t stream)
{
    message_type = RTMP_MSG_VIDEO_MESSAGE;
    payload_length = size;
    timestamp_delta = time;
    timestamp = time;
    stream_id = stream;
    perfer_cid = RTMP_CID_VIDEO;
}
void MessageHeader::InitializeAudio(int32_t size, uint32_t time, int32_t stream)
{
    message_type = RTMP_MSG_AUDIO_MESSAGE;
    payload_length = size;
    timestamp_delta = time;
    timestamp = time;
    stream_id = stream;
    perfer_cid = RTMP_CID_AUDIO;
}

CommonMessage::CommonMessage()
{
    size = 0;
    payload = nullptr;
}

CommonMessage::~CommonMessage()
{
    rs_freepa(payload);
}

void CommonMessage::CreatePayload(int32_t size)
{
    rs_freepa(payload);
    payload = new char[size];
}

ChunkStream::ChunkStream(int cid)
{
    this->cid = cid;
    fmt = 0;
    msg = nullptr;
    extended_timestamp = false;
    msg_count = 0;
}

ChunkStream::~ChunkStream()
{
}

/**
 * @name: SharedPtrPayload
 * @msg: SharedPtrPayload构造函数
 */
SharedPtrMessage::SharedPtrPayload::SharedPtrPayload()
{
    size = 0;
    payload = nullptr;
    shared_count = 0;
}

/**
 * @name: ~SharedPtrPayload
 * @msg: SharedPtrPayload析构函数
 */
SharedPtrMessage::SharedPtrPayload::~SharedPtrPayload()
{
    rs_freepa(payload);
}

SharedPtrMessage::SharedPtrMessage()
{
    ptr_ = nullptr;
}

SharedPtrMessage::~SharedPtrMessage()
{
    if (ptr_)
    {
        if (ptr_->shared_count <= 0)
        {
            rs_freep(ptr_);
        }
        else
        {
            ptr_->shared_count--;
        }
    }
}

int SharedPtrMessage::Create(MessageHeader *pheader, char *payload, int size)
{
    int ret = ERROR_SUCCESS;

    if (ptr_)
    {
        ret = ERROR_SYSTEM_ASSERT_FAILED;
        rs_error("can't set payload twice. ret=%d", ret);
        rs_assert(false);
    }

    ptr_ = new SharedPtrPayload;

    if (pheader)
    {
        ptr_->header.message_type = pheader->message_type;
        ptr_->header.payload_length = pheader->payload_length;
        ptr_->header.perfer_cid = pheader->perfer_cid;
        this->timestamp = pheader->timestamp;
        this->stream_id = pheader->stream_id;
    }
    ptr_->payload = payload;
    ptr_->size = size;

    this->payload = ptr_->payload;
    this->size = ptr_->size;

    return ret;
}

int SharedPtrMessage::Create(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if ((ret = Create(&msg->header, msg->payload, msg->size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    msg->payload = nullptr;
    msg->size = 0;

    return ret;
}

int SharedPtrMessage::Count()
{
    return ptr_->shared_count;
}

bool SharedPtrMessage::Check(int stream_id)
{
    if (ptr_->header.perfer_cid < 2 || ptr_->header.perfer_cid > 63)
    {
        rs_info("change the chunk_id=%d to default=%d", ptr_->header.perfer_cid, RTMP_CID_PROTOCOL_CONTROL);
        ptr_->header.perfer_cid = RTMP_CID_PROTOCOL_CONTROL;
    }

    if (this->stream_id == stream_id)
    {
        return true;
    }

    this->stream_id = stream_id;

    return false;
}

bool SharedPtrMessage::IsAV()
{
    return ptr_->header.message_type == RTMP_MSG_AUDIO_MESSAGE ||
           ptr_->header.message_type == RTMP_MSG_VIDEO_MESSAGE;
}

bool SharedPtrMessage::IsAudio()
{
    return ptr_->header.message_type == RTMP_MSG_AUDIO_MESSAGE;
}

bool SharedPtrMessage::IsVideo()
{
    return ptr_->header.message_type == RTMP_MSG_VIDEO_MESSAGE;
}

int SharedPtrMessage::ChunkHeader(char *buf, bool c0)
{
    if (c0)
    {
        return ChunkHeaderC0(ptr_->header.perfer_cid, timestamp, ptr_->header.payload_length, ptr_->header.message_type, stream_id, buf);
    }
    else
    {
        return ChunkHeaderC3(ptr_->header.perfer_cid, timestamp, buf);
    }
}

SharedPtrMessage *SharedPtrMessage::Copy()
{
    SharedPtrMessage *copy = new SharedPtrMessage;
    copy->ptr_ = ptr_;
    ptr_->shared_count++;

    copy->timestamp = timestamp;
    copy->stream_id = stream_id;
    copy->payload = ptr_->payload;
    copy->size = ptr_->size;

    return copy;
}

MessageArray::MessageArray(int max_msgs)
{
    msgs = new SharedPtrMessage *[max_msgs];
    max = max_msgs;

    Zero(max_msgs);
}

MessageArray::~MessageArray()
{
    rs_freepa(msgs);
}

void MessageArray::Free(int count)
{
    for (int i = 0; i < count; i++)
    {
        SharedPtrMessage *msg = msgs[i];
        rs_freep(msg);
        msgs[i] = nullptr;
    }
}

void MessageArray::Zero(int count)
{
    for (int i = 0; i < count; i++)
    {
        msgs[i] = nullptr;
    }
}

} // namespace rtmp