#ifndef RS_RTMP_STACK_HPP
#define RS_RTMP_STACK_HPP

#include <common/buffer.hpp>
#include <common/core.hpp>
#include <common/error.hpp>
#include <common/io.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>
#include <protocol/rtmp/defines.hpp>
#include <protocol/rtmp/message.hpp>
#include <protocol/rtmp/packet.hpp>

#include <map>
#include <vector>

class AMF0Object;

namespace rtmp {

class HandshakeBytes;

enum class ConnType {
    UNKNOW           = 0,
    PLAY             = 1,
    FMLE_PUBLISH     = 2,
    FLASH_PUBLISH    = 3,
    HIVISION_PUBLISH = 4
};

enum class UserEventType {
    STREAM_BEGIN       = 0x00,
    STREAM_EOF         = 0x01,
    STREAM_DRY         = 0x02,
    SET_BUFFER_LEN     = 0x03,
    STREAM_IS_RECORDED = 0x04,
    PING_REQUEST       = 0x06,
    PING_RESPONSE      = 0x07,
    FMS_EVENT0         = 0x1a
};

extern void DiscoveryTcUrl(const std::string& tc_url,
                           std::string&       schema,
                           std::string&       host,
                           std::string&       vhost,
                           std::string&       app,
                           std::string&       stream,
                           std::string&       port,
                           std::string&       param);

class Request {
  public:
    Request();
    virtual ~Request();

  public:
    virtual void        Strip();
    virtual Request*    Copy();
    virtual std::string GetStreamUrl();
    virtual void        Update(Request* req);

  public:
    // base attributes
    std::string ip;
    std::string tc_url;
    std::string page_url;
    std::string swf_url;
    double      object_encoding;
    // parsed attributes
    std::string schema;
    std::string vhost;
    std::string host;
    std::string port;
    std::string app;
    std::string param;
    std::string stream;
    double      duration;
    AMF0Object* args;
};

class Response {
  public:
    int stream_id = 1;
};

class AckWindowSize {
  public:
    AckWindowSize();
    virtual ~AckWindowSize();

  public:
    uint32_t window;
    uint32_t sequence_number;
    int64_t  recv_bytes;
};

class Protocol {
  public:
  public:
    Protocol(IProtocolReaderWriter* rw);
    virtual ~Protocol();

  public:
    virtual void SetSendTimeout(int64_t timeout_us);
    virtual void SetRecvTimeout(int64_t timeout_us);
    virtual int  RecvMessage(CommonMessage** pmsg);
    virtual int  DecodeMessage(CommonMessage* msg, Packet** ppacket);
    virtual int  SendAndFreePacket(Packet* packet, int stream_id);
    virtual int
                 SendAndFreeMessages(SharedPtrMessage** msgs, int nb_msgs, int stream_id);
    virtual void SetRecvBuffer(int buffer_size);
    virtual void SetMargeRead(bool v, IMergeReadHandler* handler);
    virtual void SetAutoResponse(bool v);

    template <typename T> int ExpectMessage(CommonMessage** pmsg, T** ppacket)
    {
        int ret = ERROR_SUCCESS;

        while (true) {
            CommonMessage* msg = nullptr;
            if ((ret = RecvMessage(&msg)) != ERROR_SUCCESS) {
                if (!is_client_gracefully_close(ret)) {
                    rs_error("recv message failed. ret=%d", ret);
                }
                return ret;
            }

            Packet* packet = nullptr;
            if ((ret = DecodeMessage(msg, &packet)) != ERROR_SUCCESS) {
                rs_error("decode message failed. ret=%d", ret);
                rs_freep(msg);
                rs_freep(packet);
                return ret;
            }

            T* pkt = dynamic_cast<T*>(packet);
            if (!pkt) {
                rs_freep(msg);
                rs_freep(packet);
                continue;
            }

            *pmsg    = msg;
            *ppacket = pkt;
            break;
        }
        return ret;
    }

  protected:
    virtual int RecvInterlacedMessage(CommonMessage** pmsg);
    virtual int ReadBasicHeader(char& fmt, int& cid);
    virtual int ReadMessageHeader(ChunkStream* cs, char fmt);
    virtual int ReadMessagePayload(ChunkStream* cs, CommonMessage** pmsg);
    virtual int OnRecvMessage(CommonMessage* msg);
    virtual int ResponseAckMessage();
    virtual int DoDecodeMessage(MessageHeader& header,
                                BufferManager* manager,
                                Packet**       packet);
    virtual int DoSendAndFreePacket(Packet* packet, int stream_id);
    virtual int DoSimpleSend(MessageHeader* header, char* payload, int size);
    virtual int OnSendPacket(MessageHeader* header, Packet* packet);
    virtual int ManualResponseFlush();
    virtual int do_send_messages(SharedPtrMessage** msgs, int nb_msgs);

  private:
    IProtocolReaderWriter*        rw_;
    int32_t                       in_chunk_size_;
    int32_t                       out_chunk_size_;
    FastBuffer*                   in_buffer_;
    ChunkStream**                 cs_cache_;
    bool                          auto_response_when_recv_;
    std::map<int, ChunkStream*>   chunk_streams_;
    AckWindowSize                 in_ack_size_;
    AckWindowSize                 out_ack_size_;
    std::vector<Packet*>          manual_response_queue_;
    std::map<double, std::string> requests_;
    iovec*                        out_iovs_;
    int                           nb_out_iovs_;
    char                          out_c0c3_caches_[RTMP_C0C3_HEADERS_MAX];
};

}  // namespace rtmp

#endif