#include <protocol/rtmp_message.hpp>

IMessageHandler::IMessageHandler()
{
}

IMessageHandler::~IMessageHandler()
{
}

RTMPMessageHeader::RTMPMessageHeader()
{
}

RTMPMessageHeader::~RTMPMessageHeader()
{
}

bool RTMPMessageHeader::IsAudio()
{
    return true;
}
bool RTMPMessageHeader::IsVideo()
{
    return true;
}
bool RTMPMessageHeader::IsAMF0Command()
{
    return true;
}
bool RTMPMessageHeader::IsAMF0Data()
{
    return true;
}
bool RTMPMessageHeader::IsAMF3Command()
{
    return true;
}
bool RTMPMessageHeader::IsAMF3Data()
{
    return true;
}
bool RTMPMessageHeader::IsWindowAckledgementSize()
{
    return true;
}

bool RTMPMessageHeader::IsAckLedgement()
{
    return true;
}
bool RTMPMessageHeader::IsSetChunkSize()
{
    return true;
}
bool RTMPMessageHeader::IsUserControlMessage()
{
    return true;
}
bool RTMPMessageHeader::IsSetPeerBandWidth()
{
    return true;
}
bool RTMPMessageHeader::IsAggregate()
{
    return true;
}
void RTMPMessageHeader::InitializeAMF0Script(int32_t size, int32_t stream)
{
}
void RTMPMessageHeader::InitializeVideo(int32_t size, uint32_t timestamp, int32_t stream)
{
}
void RTMPMessageHeader::InitializeAudio(int32_t size, uint32_t timestamp, int32_t stream)
{
}

RTMPCommonMessage::RTMPCommonMessage()
{
}

RTMPCommonMessage::~RTMPCommonMessage()
{
}

void RTMPCommonMessage::CreatePayload(int32_t size)
{
}