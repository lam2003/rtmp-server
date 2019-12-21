#ifndef RS_RTMP_MESSAGE_HPP
#define RS_RTMP_MESSAGE_HPP

#include <common/core.hpp>

class RTMPCommonMessage;

class IMessageHandler
{
public:
    IMessageHandler();
    virtual ~IMessageHandler();

public:
    virtual bool CanHandle() = 0;
    virtual int32_t Handle(RTMPCommonMessage *msg) = 0;
    virtual void OnRecvError(int32_t ret) = 0;
    virtual void OnTrheadStart() = 0;
    virtual void OnThreadStop() = 0;
};

class RTMPMessageHeader
{
public:
    int32_t timestamp_delta;
    int32_t payload_length;
    int8_t message_type;
    int32_t stream_id;
    int64_t timestamp;
    int32_t perfer_cid;

public:
    RTMPMessageHeader();
    virtual ~RTMPMessageHeader();

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
};

class RTMPCommonMessage
{

public:
    RTMPCommonMessage();
    virtual ~RTMPCommonMessage();

public:
    virtual void CreatePayload(int32_t size);

public:
    RTMPMessageHeader header;
    int32_t size;
    char *payload;
};

#endif