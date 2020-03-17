#include <app/rtmp_server.hpp>
#include <common/error.hpp>
#include <common/utils.hpp>
#include <protocol/amf/amf0.hpp>
#include <protocol/rtmp/defines.hpp>
#include <protocol/rtmp/stack.hpp>

RTMPServer::RTMPServer(IProtocolReaderWriter* rw) : rw_(rw)
{
    handshake_bytes_ = new rtmp::HandshakeBytes;
    protocol_        = new rtmp::Protocol(rw);
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
    if ((ret = simple_handshake.HandshakeWithClient(handshake_bytes_, rw_)) !=
        ERROR_SUCCESS) {
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

int RTMPServer::ConnectApp(rtmp::Request* req)
{
    int                     ret = ERROR_SUCCESS;
    rtmp::CommonMessage*    msg = nullptr;
    rtmp::ConnectAppPacket* pkt = nullptr;

    if ((ret = protocol_->ExpectMessage<rtmp::ConnectAppPacket>(&msg, &pkt)) !=
        ERROR_SUCCESS) {
        rs_error("expect connect app message failed,ret=%d", ret);
        return ret;
    }

    rs_auto_free(rtmp::CommonMessage, msg);
    rs_auto_free(rtmp::ConnectAppPacket, pkt);

    AMF0Any* p = nullptr;

    if ((p = pkt->command_object->EnsurePropertyString("tcUrl")) == nullptr) {
        ret = ERROR_RTMP_REQ_CONNECT;
        rs_error("invalid request,must specifies the tcUrl,ret=%d", ret);
        return ret;
    }

    req->tc_url = p->ToString();

    if ((p = pkt->command_object->EnsurePropertyString("pageUrl")) != nullptr) {
        req->page_url = p->ToString();
    }

    if ((p = pkt->command_object->EnsurePropertyString("swfUrl")) != nullptr) {
        req->swf_url = p->ToString();
    }

    if ((p = pkt->command_object->EnsurePropertyNumber("objectEncoding")) !=
        nullptr) {
        req->object_encoding = p->ToNumber();
    }

    if (pkt->args) {
        rs_freep(req->args);
        req->args = pkt->args->Copy()->ToObject();
    }

    rtmp::DiscoveryTcUrl(req->tc_url, req->schema, req->host, req->vhost,
                         req->app, req->stream, req->port, req->param);
    req->Strip();

    return ret;
}

int RTMPServer::SetWindowAckSize(int ackowledgement_window_size)
{
    int ret = ERROR_SUCCESS;

    rtmp::SetWindowAckSizePacket* pkt = new rtmp::SetWindowAckSizePacket;
    pkt->ackowledgement_window_size   = ackowledgement_window_size;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_ackowledgement_window_size packet failed,ret=%d",
                 ret);
        return ret;
    }

    return ret;
}

int RTMPServer::SetPeerBandwidth(int bandwidth, int type)
{
    int ret = ERROR_SUCCESS;

    rtmp::SetPeerBandwidthPacket* pkt = new rtmp::SetPeerBandwidthPacket;
    pkt->bandwidth                    = bandwidth;
    pkt->type                         = type;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_peer_bandwidth packet failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int RTMPServer::SetChunkSize(int chunk_size)
{
    int ret = ERROR_SUCCESS;

    rtmp::SetChunkSizePacket* pkt = new rtmp::SetChunkSizePacket;
    pkt->chunk_size               = chunk_size;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_chunk_size_packet failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int RTMPServer::ResponseConnectApp(rtmp::Request*     req,
                                   const std::string& local_ip)
{
    int ret = ERROR_SUCCESS;

    rtmp::ConnectAppResPacket* pkt = new rtmp::ConnectAppResPacket;

    pkt->props->Set("fmsVer", AMF0Any::String("FMS/3,5,3,888"));
    pkt->props->Set("capabilities", AMF0Any::Number(127));
    pkt->props->Set("mode", AMF0Any::Number(1));
    pkt->props->Set("level", AMF0Any::String("status"));
    pkt->props->Set("code", AMF0Any::String("NetConnection.Connect.Success"));
    pkt->props->Set("description", AMF0Any::String("IConnection succeeded"));
    pkt->props->Set("objectEncoding", AMF0Any::Number(req->object_encoding));

    AMF0EcmaArray* ecma_array = AMF0Any::EcmaArray();
    pkt->props->Set("data", ecma_array);

    ecma_array->Set("version", AMF0Any::String("3,5,3,888"));

    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send connect app response message failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int RTMPServer::identify_fmle_publish_client(rtmp::FMLEStartPacket* pkt,
                                             rtmp::ConnType&        type,
                                             std::string&           stream_name)
{
    int ret = ERROR_SUCCESS;

    type        = rtmp::ConnType::FMLE_PUBLISH;
    stream_name = pkt->stream_name;

    rtmp::FMLEStartResPacket* res_pkt =
        new rtmp::FMLEStartResPacket(pkt->transaction_id);
    if ((ret = protocol_->SendAndFreePacket(res_pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send release stream response message failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int RTMPServer::identify_play_client(rtmp::PlayPacket* pkt,
                                     rtmp::ConnType&   type,
                                     std::string&      stream_name,
                                     double&           duration)
{
    int ret = ERROR_SUCCESS;

    type        = rtmp::ConnType::PLAY;
    stream_name = pkt->stream_name;
    duration    = pkt->duration;

    rs_info("identity client type=play, stream_name=%s, duration=%.2f",
            stream_name.c_str(), duration);

    return ret;
}

int RTMPServer::identify_create_stream_client(rtmp::CreateStreamPacket* pkt,
                                              int             stream_id,
                                              rtmp::ConnType& type,
                                              std::string&    stream_name,
                                              double&         duration)
{
    int ret = ERROR_SUCCESS;

    rtmp::CreateStreamResPacket* res_pkt =
        new rtmp::CreateStreamResPacket(pkt->transaction_id, stream_id);
    if ((ret = protocol_->SendAndFreePacket(res_pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send createStream response message failed. ret=%d", ret);
        return ret;
    }

    while (true) {
        rtmp::CommonMessage* msg = nullptr;
        if ((ret = protocol_->RecvMessage(&msg)) != ERROR_SUCCESS) {
            if (!IsClientGracefullyClose(ret)) {
                rs_error("recv identify client message failed. ret=%d", ret);
            }
            return ret;
        }

        rs_auto_free(rtmp::CommonMessage, msg);
        rtmp::MessageHeader& h = msg->header;

        if (!h.IsAMF0Command() && !h.IsAMF3Command()) {
            continue;
        }

        rtmp::Packet* packet = nullptr;
        if ((ret = protocol_->DecodeMessage(msg, &packet)) != ERROR_SUCCESS) {
            rs_error("decodec identify client message failed. ret=%d", ret);
            return ret;
        }

        rs_auto_free(rtmp::Packet, packet);

        if (dynamic_cast<rtmp::PlayPacket*>(packet)) {
            return identify_play_client(dynamic_cast<rtmp::PlayPacket*>(packet),
                                        type, stream_name, duration);
        }
    }

    return ret;
}

int RTMPServer::IdentifyClient(int             stream_id,
                               rtmp::ConnType& type,
                               std::string&    stream_name,
                               double&         duration)
{
    int ret = ERROR_SUCCESS;
    type    = rtmp::ConnType::UNKNOW;

    while (true) {
        rtmp::CommonMessage* msg = nullptr;
        if ((ret = protocol_->RecvMessage(&msg)) != ERROR_SUCCESS) {
            if (!IsClientGracefullyClose(ret)) {
                rs_error("recv identify client message failed,ret=%d", ret);
            }
            return ret;
        }

        rs_auto_free(rtmp::CommonMessage, msg);
        rtmp::MessageHeader& h = msg->header;

        if (!h.IsAMF0Command() && !h.IsAMF3Command()) {
            continue;
        }

        rtmp::Packet* packet = nullptr;
        if ((ret = protocol_->DecodeMessage(msg, &packet)) != ERROR_SUCCESS) {
            rs_error("identify decode message failed,ret=%d", ret);
            return ret;
        }

        rs_auto_free(rtmp::Packet, packet);
        if (dynamic_cast<rtmp::FMLEStartPacket*>(packet)) {
            return identify_fmle_publish_client(
                dynamic_cast<rtmp::FMLEStartPacket*>(packet), type,
                stream_name);
        }
        else if (dynamic_cast<rtmp::CreateStreamPacket*>(packet)) {
            return identify_create_stream_client(
                dynamic_cast<rtmp::CreateStreamPacket*>(packet), stream_id,
                type, stream_name, duration);
        }
        else {
            rs_assert(0);
        }
    }

    return ret;
}

int RTMPServer::StartFmlePublish(int stream_id)
{
    int ret = ERROR_SUCCESS;

    double fc_publish_tid = 0;
    {
        rtmp::CommonMessage*   msg = nullptr;
        rtmp::FMLEStartPacket* pkt = nullptr;

        if ((ret = protocol_->ExpectMessage<rtmp::FMLEStartPacket>(
                 &msg, &pkt)) != ERROR_SUCCESS) {
            rs_error("recv FCPublish message failed,ret=%d", ret);
            return ret;
        }

        rs_auto_free(rtmp::CommonMessage, msg);
        rs_auto_free(rtmp::FMLEStartPacket, pkt);

        fc_publish_tid = pkt->transaction_id;
    }
    {
        rtmp::FMLEStartResPacket* pkt =
            new rtmp::FMLEStartResPacket(fc_publish_tid);

        if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
            rs_error("send FCPublish response message failed,ret=%d", ret);
            return ret;
        }
    }

    double create_stream_id = 0;
    {
        rtmp::CommonMessage*      msg = nullptr;
        rtmp::CreateStreamPacket* pkt = nullptr;

        if ((ret = protocol_->ExpectMessage<rtmp::CreateStreamPacket>(
                 &msg, &pkt)) != ERROR_SUCCESS) {
            rs_error("recv createStream message failed,ret=%d", ret);
            return ret;
        }
        rs_auto_free(rtmp::CommonMessage, msg);
        rs_auto_free(rtmp::CreateStreamPacket, pkt);

        create_stream_id = pkt->transaction_id;
    }

    {
        rtmp::CreateStreamResPacket* pkt =
            new rtmp::CreateStreamResPacket(create_stream_id, stream_id);
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error("send createStream response message failed,ret=%d", ret);
            return ret;
        }
    }

    {
        rtmp::CommonMessage* msg;
        rtmp::PublishPacket* pkt;

        if ((ret = protocol_->ExpectMessage<rtmp::PublishPacket>(&msg, &pkt)) !=
            ERROR_SUCCESS) {
            rs_error("recv publish message failed,ret=%d", ret);
            return ret;
        }

        rs_info("recv publish request message success,stream_name:%s,type=%s",
                pkt->stream_name.c_str(), pkt->type.c_str());

        rs_auto_free(rtmp::CommonMessage, msg);
        rs_auto_free(rtmp::PublishPacket, pkt);
    }
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->command_name             = RTMP_AMF0_COMMAND_ON_FC_PUBLISH;
        pkt->data->Set("code", AMF0Any::String("NetStream.Publish.Start"));
        pkt->data->Set("description",
                       AMF0Any::String("Started publishing stream"));

        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error("send onFCPublish(NetStream.Publish.Start) message "
                     "failed,ret=%d",
                     ret);
            return ret;
        }
        rs_info("send onFCPublish(NetStream.Publish.Start) message success");
    }
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->data->Set("level", AMF0Any::String("status"));
        pkt->data->Set("code", AMF0Any::String("NetStream.Publish.Start"));
        pkt->data->Set("description",
                       AMF0Any::String("Started publishing stream"));
        pkt->data->Set("clientid", AMF0Any::String("ASAICiss"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error(
                "send onStatus(NetStream.Publish.Start) message failed,ret=%d",
                ret);
            return ret;
        }
        rs_info("send onStatus(NetStream.Publish.Start) message success");
    }

    return ret;
}

int RTMPServer::RecvMessage(rtmp::CommonMessage** pmsg)
{
    return protocol_->RecvMessage(pmsg);
}

void RTMPServer::SetRecvBuffer(int buffer_size)
{
    protocol_->SetRecvBuffer(buffer_size);
}

void RTMPServer::SetMargeRead(bool v, IMergeReadHandler* handler)
{
    protocol_->SetMargeRead(v, handler);
}

int RTMPServer::DecodeMessage(rtmp::CommonMessage* msg, rtmp::Packet** ppacket)
{
    return protocol_->DecodeMessage(msg, ppacket);
}

int RTMPServer::FMLEUnPublish(int stream_id, double unpublish_tid)
{
    int ret = ERROR_SUCCESS;
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->command_name             = RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH;
        pkt->data->Set("code", AMF0Any::String("NetStream.Unpublish.Success"));
        pkt->data->Set("description",
                       AMF0Any::String("Stop publishing stream"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!IsSystemControlError(ret) && !IsClientGracefullyClose(ret)) {
                rs_error("send onFCUnpublish(NetStream.Unpublish.Success) "
                         "message failed. ret=%d",
                         ret);
            }
            return ret;
        }

        rs_info(
            "send onFCUnpublish(NetStream.Unpublish.Success) message success.");
    }
    {
        rtmp::FMLEStartResPacket* pkt =
            new rtmp::FMLEStartResPacket(unpublish_tid);
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!IsSystemControlError(ret) && !IsClientGracefullyClose(ret)) {
                rs_error("send FCUnpublish response messsage failed. ret=%d",
                         ret);
            }
            return ret;
        }
    }
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->data->Set("level", AMF0Any::String("status"));
        pkt->data->Set("code", AMF0Any::String("NetStream.Unpublish.Success"));
        pkt->data->Set("description",
                       AMF0Any::String("Stream is now unpublished"));
        pkt->data->Set("clientid", AMF0Any::String("ASAICiss"));

        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!IsSystemControlError(ret) && !IsClientGracefullyClose(ret)) {
                rs_error("send onStatus(NetStream.Unpublish.Success) message "
                         "failed. ret=%d",
                         ret);
            }
            return ret;
        }

        rs_info("send onStatus(NetStream.Unpublish.Success) message success.");
    }

    rs_trace("FMLE unpublish success");

    return ret;
}

int RTMPServer::StartPlay(int stream_id)
{
    int ret = ERROR_SUCCESS;
    {
        rtmp::UserControlPacket* pkt = new rtmp::UserControlPacket;
        pkt->event_type = (int16_t)rtmp::UserEventType::STREAM_BEGIN;
        pkt->event_data = stream_id;
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error("send StreamBegin failed. ret=%d", ret);
            return ret;
        }

        rs_trace("send StreamBegin success");
    }
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->data->Set("level", AMF0Any::String("status"));
        pkt->data->Set("code", AMF0Any::String("NetStream.Play.Reset"));
        pkt->data->Set("description",
                       AMF0Any::String("Stream is now reseting"));
        pkt->data->Set("details", AMF0Any::String("stream"));
        pkt->data->Set("clientid", AMF0Any::String("ASAICiss"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error(
                "send onStatus(NetStream.Play.Reset) message failed. ret=%d",
                ret);
            return ret;
        }

        rs_trace("send onStatus(NetStream.Play.Reset) success");
    }
    {
        rtmp::OnStatusCallPacket* pkt = new rtmp::OnStatusCallPacket;
        pkt->data->Set("level", AMF0Any::String("status"));
        pkt->data->Set("code", AMF0Any::String("NetStream.Play.Start"));
        pkt->data->Set("description", AMF0Any::String("Stream is now playing"));
        pkt->data->Set("details", AMF0Any::String("stream"));
        pkt->data->Set("clientid", AMF0Any::String("ASAICiss"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error(
                "send onStatus(NetStream.Play.Start) message failed. ret=%d",
                ret);
            return ret;
        }

        rs_trace("send onStatus(NetStream.Play.Start) success");
    }
    {
        rtmp::OnStatusDataPacket* pkt = new rtmp::OnStatusDataPacket;
        pkt->data->Set("code", AMF0Any::String("NetStream.Data.Start"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error(
                "send onStatus(NetStream.Data.Start) message failed. ret=%d",
                ret);
            return ret;
        }
        rs_trace("send onStatus(NetStream.Data.Start) success");
    }

    rs_trace("start play success");

    return ret;
}

void RTMPServer::SetAutoResponse(bool v)
{
    protocol_->SetAutoResponse(v);
}