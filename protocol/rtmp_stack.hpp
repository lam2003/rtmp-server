#ifndef RS_RTMP_STACK_HPP
#define RS_RTMP_STACK_HPP

#include <common/core.hpp>
#include <common/io.hpp>
#include <common/buffer.hpp>

#include <map>

namespace rtmp
{
class CommonMessage;

class IMessageHandler
{
public:
    IMessageHandler();
    virtual ~IMessageHandler();

public:
    virtual bool CanHandle() = 0;
    virtual int32_t Handle(CommonMessage *msg) = 0;
    virtual void OnRecvError(int32_t ret) = 0;
    virtual void OnTrheadStart() = 0;
    virtual void OnThreadStop() = 0;
};

class MessageHeader
{
public:
    MessageHeader();
    virtual ~MessageHeader();

public:
    bool IsAudio();
    bool IsVideo();
    bool IsAMF0Command();
    bool IsAMF0Data();
    bool IsAMF3Command();
    bool IsAMF3Data();
    bool IsWindowAckledgementSize();
    bool IsAckLedgement();
    bool IsSetChunkSize();
    bool IsUserControlMessage();
    bool IsSetPeerBandWidth();
    bool IsAggregate();
    void InitializeAMF0Script(int32_t size, int32_t stream);
    void InitializeVideo(int32_t size, uint32_t timestamp, int32_t stream);
    void InitializeAudio(int32_t size, uint32_t timestamp, int32_t stream);

public:
    int32_t timestamp_delta;
    int32_t payload_length;
    int8_t message_type;
    int32_t stream_id;
    int64_t timestamp;
    int32_t perfer_cid;
};

class CommonMessage
{

public:
    CommonMessage();
    virtual ~CommonMessage();

public:
    virtual void CreatePayload(int32_t size);

public:
    MessageHeader header;
    int32_t size;
    char *payload;
};

class ChunkStream
{
public:
    ChunkStream(int cid);
    virtual ~ChunkStream();

public:
    int cid;
    char fmt;
    CommonMessage *msg;
    bool extended_timestamp;
    int msg_count;
    MessageHeader header;
};

class HandshakeBytes
{
public:
    HandshakeBytes();
    virtual ~HandshakeBytes();

    virtual int32_t ReadC0C1(IProtocolReaderWriter *rw);
    virtual int32_t ReadS0S1S2(IProtocolReaderWriter *rw);
    virtual int32_t ReadC2(IProtocolReaderWriter *rw);
    virtual int32_t CreateC0C1();
    virtual int32_t CreateS0S1S2(const char *c1 = NULL);
    virtual int32_t CreateC2();

public:
    //1+1536
    char *c0c1;
    //1+1536+1536
    char *s0s1s2;
    //1536
    char *c2;
};

class SimpleHandshake
{
public:
    SimpleHandshake();
    virtual ~SimpleHandshake();

public:
    virtual int32_t HandshakeWithClient(HandshakeBytes *handshake_bytes, IProtocolReaderWriter *rw);
};

class Protocol
{
public:
    Protocol(IProtocolReaderWriter *rw);
    virtual ~Protocol();

public:
    virtual void SetSendTimeout(int64_t timeout_us);
    virtual void SetRecvTimeout(int64_t timeout_us);
    virtual int ReadInterlacedMessage(CommonMessage **pmsg);

protected:
    virtual int ReadBasicHeader(char &fmt, int &cid);
    virtual int ReadRTMPMsgHeader(ChunkStream *cs, char fmt);

private:
    IProtocolReaderWriter *rw_;
    FastBuffer *in_buffer_;
    ChunkStream **cs_cache_;
    std::map<int, ChunkStream *> chunk_streams_;
};

} // namespace rtmp


#endif