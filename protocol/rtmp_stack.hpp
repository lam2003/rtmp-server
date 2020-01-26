#ifndef RS_RTMP_STACK_HPP
#define RS_RTMP_STACK_HPP

#include <common/core.hpp>
#include <common/io.hpp>
#include <common/buffer.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>
#include <protocol/rtmp_amf0.hpp>

#include <map>

namespace rtmp
{

extern void DiscoveryTcUrl(const std::string &tc_url,
                           std::string &schema,
                           std::string &host,
                           std::string &vhost,
                           std::string &app,
                           std::string &stream,
                           std::string &port,
                           std::string &param);

enum class PeerBandwidthType
{
    HARD = 0,
    SOTF = 1,
    DYNAMIC = 2,
};

enum class ConnType
{
    UNKNOW = 0,
    PLAY = 1,
    FMLE_PUBLISH = 2,
};

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
    virtual void OnThreadStart() = 0;
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
    bool IsAckledgement();
    bool IsSetChunkSize();
    bool IsUserControlMessage();
    bool IsSetPeerBandWidth();
    bool IsAggregate();
    void InitializeAMF0Script(int32_t size, int32_t stream);
    void InitializeVideo(int32_t size, uint32_t time, int32_t stream);
    void InitializeAudio(int32_t size, uint32_t time, int32_t stream);

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
    int32_t size;
    char *payload;
    MessageHeader header;
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

struct SharedMessageHeader
{
    int32_t payload_length;
    int8_t message_type;
    int perfer_cid;
};

class SharedPtrMessage
{
public:
    SharedPtrMessage();
    virtual ~SharedPtrMessage();

public:
    virtual int Create(CommonMessage *msg);
    virtual int Create(MessageHeader *pheader, char *payload, int size);
    virtual int Count();
    virtual bool Check(int stream_id);
    virtual bool IsAV();
    virtual bool IsAudio();
    virtual bool IsVideo();
    virtual int ChunkHeader(char *buf, bool c0);
    virtual SharedPtrMessage *Copy();

private:
    class SharedPtrPayload
    {
    public:
        SharedPtrPayload();
        virtual ~SharedPtrPayload();

    public:
        SharedMessageHeader header;
        char *payload;
        int size;
        int shared_count;
    };

public:
    int64_t timestamp;
    int32_t stream_id;
    int size;
    char *payload;

private:
    SharedPtrPayload *ptr_;
};

class MessageArray
{
public:
    MessageArray(int max_msgs);
    virtual ~MessageArray();

public:
    virtual void Free(int count);
    virtual void Zero(int count);

public:
    SharedPtrMessage **msgs;
    int max;
};

class Client
{
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

class Request
{
public:
    Request();
    virtual ~Request();

public:
    virtual void Strip();
    virtual Request *Copy();
    virtual std::string GetStreamUrl();
    virtual void Update(Request *req);

public:
    //base attributes
    std::string ip;
    std::string tc_url;
    std::string page_url;
    std::string swf_url;
    double object_encoding;
    //parsed attributes
    std::string schema;
    std::string vhost;
    std::string host;
    std::string port;
    std::string app;
    std::string param;
    std::string stream;
    double duration;
    AMF0Object *args;
};

class Response
{
public:
    int stream_id = 1;
};

class Packet
{
public:
    Packet();
    virtual ~Packet();

public:
    virtual int GetPreferCID();
    virtual int GetMessageType();
    virtual int Encode(int &size, char *&payload);
    virtual int Decode(BufferManager *manager);

protected:
    virtual int GetSize();
    virtual int EncodePacket(BufferManager *manager);
};

class SetChunkSizePacket : public Packet
{
public:
    SetChunkSizePacket();
    virtual ~SetChunkSizePacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    int32_t chunk_size;
};

class ConnectAppPacket : public Packet
{
public:
    ConnectAppPacket();
    virtual ~ConnectAppPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Object *args;
    AMF0Object *command_object;
};

class ConnectAppResPacket : public Packet
{
public:
    ConnectAppResPacket();
    virtual ~ConnectAppResPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Object *props;
    AMF0Object *info;
};

class SetWindowAckSizePacket : public Packet
{
public:
    SetWindowAckSizePacket();
    virtual ~SetWindowAckSizePacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    int32_t ackowledgement_window_size;
};

class SetPeerBandwidthPacket : public Packet
{
public:
    SetPeerBandwidthPacket();
    virtual ~SetPeerBandwidthPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    int32_t bandwidth;
    int8_t type;
};

class FMLEStartPacket : public Packet
{
public:
    FMLEStartPacket();
    virtual ~FMLEStartPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    std::string stream_name;
};

class FMLEStartResPacket : public Packet
{
public:
    FMLEStartResPacket(double trans_id);
    virtual ~FMLEStartResPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    double transaction_id;
    std::string command_name;
    std::string stream_name;
    AMF0Any *command_object;
    AMF0Any *args;
};

class CreateStreamPacket : public Packet
{
public:
    CreateStreamPacket();
    virtual ~CreateStreamPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Any *command_object;
};

class CreateStreamResPacket : public Packet
{
public:
    CreateStreamResPacket(double trans_id, int sid);
    virtual ~CreateStreamResPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Any *command_object;
    double stream_id;
};

class PublishPacket : public Packet
{
public:
    PublishPacket();
    virtual ~PublishPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Any *command_object;
    std::string stream_name;
    std::string type;
};

class OnStatusCallPacket : public Packet
{
public:
    OnStatusCallPacket();
    virtual ~OnStatusCallPacket();

public:
    //Packet
    virtual int GetPreferCID() override;
    virtual int GetMessageType() override;
    virtual int Decode(BufferManager *manager) override;

protected:
    //Packet
    virtual int GetSize() override;
    virtual int EncodePacket(BufferManager *manager) override;

public:
    std::string command_name;
    double transaction_id;
    AMF0Any *args;
    AMF0Object *data;
};

class AckWindowSize
{

public:
    AckWindowSize();
    virtual ~AckWindowSize();

public:
    uint32_t window;
    uint32_t sequence_number;
    int64_t recv_bytes;
};

class Protocol
{
public:
public:
    Protocol(IProtocolReaderWriter *rw);
    virtual ~Protocol();

public:
    virtual void SetSendTimeout(int64_t timeout_us);
    virtual void SetRecvTimeout(int64_t timeout_us);
    virtual int RecvMessage(CommonMessage **pmsg);
    virtual int DecodeMessage(CommonMessage *msg, Packet **ppacket);
    virtual int SendAndFreePacket(Packet *packet, int stream_id);
    virtual void SetRecvBuffer(int buffer_size);
    virtual void SetMargeRead(bool v, IMergeReadHandler *handler);

    template <typename T>
    int ExpectMessage(CommonMessage **pmsg, T **ppacket)
    {
        int ret = ERROR_SUCCESS;

        while (true)
        {
            CommonMessage *msg = nullptr;
            if ((ret = RecvMessage(&msg)) != ERROR_SUCCESS)
            {
                if (!IsClientGracefullyClose(ret))
                {
                    rs_error("recv message failed,ret=%d", ret);
                }
                return ret;
            }

            Packet *packet = nullptr;
            if ((ret = DecodeMessage(msg, &packet)) != ERROR_SUCCESS)
            {
                rs_error("decode message failed,ret=%d", ret);
                rs_freep(msg);
                rs_freep(packet);
                return ret;
            }

            T *pkt = dynamic_cast<T *>(packet);
            if (!pkt)
            {
                rs_warn("drop message(type=%d,size=%d,time=%lld,sid=%d)", msg->header.message_type, msg->header.payload_length, msg->header.timestamp, msg->header.stream_id);
                rs_freep(msg);
                rs_freep(packet);
                continue;
            }

            *pmsg = msg;
            *ppacket = pkt;
            break;
        }
        return ret;
    }

protected:
    virtual int RecvInterlacedMessage(CommonMessage **pmsg);
    virtual int ReadBasicHeader(char &fmt, int &cid);
    virtual int ReadMessageHeader(ChunkStream *cs, char fmt);
    virtual int ReadMessagePayload(ChunkStream *cs, CommonMessage **pmsg);
    virtual int OnRecvMessage(CommonMessage *msg);
    virtual int ResponseAckMessage();
    virtual int DoDecodeMessage(MessageHeader &header, BufferManager *manager, Packet **packet);
    virtual int DoSendAndFreePacket(Packet *packet, int stream_id);
    virtual int DoSimpleSend(MessageHeader *header, char *payload, int size);
    virtual int OnSendPacket(MessageHeader *header, Packet *packet);
    virtual int ManualResponseFlush();

private:
    IProtocolReaderWriter *rw_;
    int32_t in_chunk_size_;
    int32_t out_chunk_size_;
    FastBuffer *in_buffer_;
    ChunkStream **cs_cache_;
    std::map<int, ChunkStream *> chunk_streams_;
    AckWindowSize in_ack_size_;
    AckWindowSize out_ack_size_;
    std::vector<Packet *> manual_response_queue_;
};

} // namespace rtmp

#endif