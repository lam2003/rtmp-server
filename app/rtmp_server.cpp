#include <app/rtmp_server.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>
#include <protocol/rtmp_stack.hpp>

RTMPServer::RTMPServer(IProtocolReaderWriter *rw) : rw_(rw)
{
    handshake_bytes_ = new rtmp::HandshakeBytes;
    protocol_ = new rtmp::Protocol(rw);
}

RTMPServer::~RTMPServer()
{
    rs_freep(protocol_);
    rs_freep(handshake_bytes_);
}

int32_t RTMPServer::Handshake()
{
    int ret = ERROR_SUCCESS;

    rtmp::SimpleHandshake simple_handshake;
    if ((ret = simple_handshake.HandshakeWithClient(handshake_bytes_, rw_)) != ERROR_SUCCESS)
    {
        return ret;
    }

    rs_freep(handshake_bytes_);

    return ret;
}

void RTMPServer::SetSendTimeout(int64_t timeout_us)
{
    protocol_->SetSendTimeout(timeout_us);
}

void RTMPServer::SetRecvTimeout(int64_t timeout_us)
{
    protocol_->SetRecvTimeout(timeout_us);
}

int32_t RTMPServer::RecvMessage(rtmp::CommonMessage **pmsg)
{
    int ret = ERROR_SUCCESS;
    protocol_->RecvMessage(pmsg);
    return ret;
}

int RTMPServer::ConnectApp(rtmp::Request *req)
{
    int ret = ERROR_SUCCESS;
    rtmp::CommonMessage *msg = nullptr;
    rtmp::ConnectAppPacket *pkt = nullptr;

    if ((ret = protocol_->ExpectMessage<rtmp::ConnectAppPacket>(&msg, &pkt)) != ERROR_SUCCESS)
    {
        rs_error("expect connect app message failed,ret=%d", ret);
        return ret;
    }

    rs_auto_free(rtmp::CommonMessage, msg);
    rs_auto_free(rtmp::ConnectAppPacket, pkt);

    rtmp::AMF0Any *p = nullptr;

    if ((p = pkt->command_object->EnsurePropertyString("tcUrl")) == nullptr)
    {
        ret = ERROR_RTMP_REQ_CONNECT;
        rs_error("invalid request,must specifies the tcUrl,ret=%d", ret);
        return ret;
    }

    req->tc_url = p->ToString();

    if ((p = pkt->command_object->EnsurePropertyString("pageUrl")) != nullptr)
    {
        req->page_url = p->ToString();
    }

    if ((p = pkt->command_object->EnsurePropertyString("swfUrl")) != nullptr)
    {
        req->swf_url = p->ToString();
    }

    if ((p = pkt->command_object->EnsurePropertyNumber("objectEncoding")) != nullptr)
    {
        req->object_encoding = p->ToNumber();
    }

    if (pkt->args)
    {
        rs_freep(req->args);
        req->args = pkt->args->Copy()->ToObject();
    }

    rtmp::DiscoveryTcUrl(req->tc_url, req->schema, req->host, req->vhost, req->app, req->stream, req->port, req->param);
    req->Strip();

    return ret;
}

int RTMPServer::SetWindowAckSize(int ackowledgement_window_size)
{
    int ret = ERROR_SUCCESS;

    rtmp::SetWindowAckSizePacket *pkt = new rtmp::SetWindowAckSizePacket;
    pkt->ackowledgement_window_size = ackowledgement_window_size;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS)
    {
        rs_error("send set_ackowledgement_window_size packet failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int RTMPServer::SetPeerBandwidth(int bandwidth)
{
    int ret = ERROR_SUCCESS;

    rtmp::SetPeerBandwidthPacket *pkt = new rtmp::SetPeerBandwidthPacket;
    pkt->bandwidth_ = bandwidth;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS)
    {
        rs_error("send set_peer_bandwidth packet failed,ret=%d",ret);
        return ret;
    }

    return ret;
}