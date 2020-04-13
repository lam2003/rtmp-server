#include <common/error.hpp>
#include <common/utils.hpp>
#include <protocol/amf/amf0.hpp>
#include <protocol/rtmp/defines.hpp>
#include <protocol/rtmp/server.hpp>
#include <protocol/rtmp/stack.hpp>

namespace rtmp {

Server::Server(IProtocolReaderWriter* rw) : rw_(rw)
{
    handshake_bytes_ = new HandshakeBytes;
    protocol_        = new Protocol(rw);
}

Server::~Server()
{
    rs_freep(protocol_);
    rs_freep(handshake_bytes_);
}

int32_t Server::Handshake()
{
    int ret = ERROR_SUCCESS;

    SimpleHandshake simple_handshake;
    if ((ret = simple_handshake.HandshakeWithClient(handshake_bytes_, rw_)) !=
        ERROR_SUCCESS) {
        return ret;
    }

    rs_freep(handshake_bytes_);

    return ret;
}

void Server::SetSendTimeout(int64_t timeout_us)
{
    protocol_->SetSendTimeout(timeout_us);
}

void Server::SetRecvTimeout(int64_t timeout_us)
{
    protocol_->SetRecvTimeout(timeout_us);
}

int Server::ConnectApp(Request* req)
{
    int               ret = ERROR_SUCCESS;
    CommonMessage*    msg = nullptr;
    ConnectAppPacket* pkt = nullptr;

    if ((ret = protocol_->ExpectMessage<ConnectAppPacket>(&msg, &pkt)) !=
        ERROR_SUCCESS) {
        rs_error("expect connect app message failed,ret=%d", ret);
        return ret;
    }

    rs_auto_free(CommonMessage, msg);
    rs_auto_free(ConnectAppPacket, pkt);

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

    DiscoveryTcUrl(req->tc_url, req->schema, req->host, req->vhost, req->app,
                   req->stream, req->port, req->param);
    req->Strip();

    return ret;
}

int Server::SetWindowAckSize(int ackowledgement_window_size)
{
    int ret = ERROR_SUCCESS;

    SetWindowAckSizePacket* pkt     = new SetWindowAckSizePacket;
    pkt->ackowledgement_window_size = ackowledgement_window_size;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_ackowledgement_window_size packet failed,ret=%d",
                 ret);
        return ret;
    }

    return ret;
}

int Server::SetPeerBandwidth(int bandwidth, int type)
{
    int ret = ERROR_SUCCESS;

    SetPeerBandwidthPacket* pkt = new SetPeerBandwidthPacket;
    pkt->bandwidth              = bandwidth;
    pkt->type                   = type;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_peer_bandwidth packet failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int Server::SetChunkSize(int chunk_size)
{
    int ret = ERROR_SUCCESS;

    SetChunkSizePacket* pkt = new SetChunkSizePacket;
    pkt->chunk_size         = chunk_size;
    if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send set_chunk_size_packet failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int Server::ResponseConnectApp(Request* req, const std::string& local_ip)
{
    int ret = ERROR_SUCCESS;

    ConnectAppResPacket* pkt = new ConnectAppResPacket;

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

int Server::identify_fmle_publish_client(FMLEStartPacket* pkt,
                                         ConnType&        type,
                                         std::string&     stream_name)
{
    int ret = ERROR_SUCCESS;

    type        = ConnType::FMLE_PUBLISH;
    stream_name = pkt->stream_name;

    FMLEStartResPacket* res_pkt = new FMLEStartResPacket(pkt->transaction_id);
    if ((ret = protocol_->SendAndFreePacket(res_pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send release stream response message failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

int Server::identify_play_client(PlayPacket*  pkt,
                                 ConnType&    type,
                                 std::string& stream_name,
                                 double&      duration)
{
    int ret = ERROR_SUCCESS;

    type        = ConnType::PLAY;
    stream_name = pkt->stream_name;
    duration    = pkt->duration;

    rs_info("identity client type=play, stream_name=%s, duration=%.2f",
            stream_name.c_str(), duration);

    return ret;
}

int Server::identify_create_stream_client(CreateStreamPacket* pkt,
                                          int                 stream_id,
                                          ConnType&           type,
                                          std::string&        stream_name,
                                          double&             duration)
{
    int ret = ERROR_SUCCESS;

    CreateStreamResPacket* res_pkt =
        new CreateStreamResPacket(pkt->transaction_id, stream_id);
    if ((ret = protocol_->SendAndFreePacket(res_pkt, 0)) != ERROR_SUCCESS) {
        rs_error("send createStream response message failed. ret=%d", ret);
        return ret;
    }

    while (true) {
        CommonMessage* msg = nullptr;
        if ((ret = protocol_->RecvMessage(&msg)) != ERROR_SUCCESS) {
            if (!is_client_gracefully_close(ret)) {
                rs_error("recv identify client message failed. ret=%d", ret);
            }
            return ret;
        }

        rs_auto_free(CommonMessage, msg);
        MessageHeader& h = msg->header;

        if (!h.IsAMF0Command() && !h.IsAMF3Command()) {
            continue;
        }

        Packet* packet = nullptr;
        if ((ret = protocol_->DecodeMessage(msg, &packet)) != ERROR_SUCCESS) {
            rs_error("decodec identify client message failed. ret=%d", ret);
            return ret;
        }

        rs_auto_free(Packet, packet);

        if (dynamic_cast<PlayPacket*>(packet)) {
            return identify_play_client(dynamic_cast<PlayPacket*>(packet), type,
                                        stream_name, duration);
        }
    }

    return ret;
}

int Server::IdentifyClient(int          stream_id,
                           ConnType&    type,
                           std::string& stream_name,
                           double&      duration)
{
    int ret = ERROR_SUCCESS;
    type    = ConnType::UNKNOW;

    while (true) {
        CommonMessage* msg = nullptr;
        if ((ret = protocol_->RecvMessage(&msg)) != ERROR_SUCCESS) {
            if (!is_client_gracefully_close(ret)) {
                rs_error("recv identify client message failed,ret=%d", ret);
            }
            return ret;
        }

        rs_auto_free(CommonMessage, msg);
        MessageHeader& h = msg->header;

        if (!h.IsAMF0Command() && !h.IsAMF3Command()) {
            continue;
        }

        Packet* packet = nullptr;
        if ((ret = protocol_->DecodeMessage(msg, &packet)) != ERROR_SUCCESS) {
            rs_error("identify decode message failed,ret=%d", ret);
            return ret;
        }

        rs_auto_free(Packet, packet);
        if (dynamic_cast<FMLEStartPacket*>(packet)) {
            return identify_fmle_publish_client(
                dynamic_cast<FMLEStartPacket*>(packet), type, stream_name);
        }
        else if (dynamic_cast<CreateStreamPacket*>(packet)) {
            return identify_create_stream_client(
                dynamic_cast<CreateStreamPacket*>(packet), stream_id, type,
                stream_name, duration);
        }
        else {
            rs_assert(0);
        }
    }

    return ret;
}

int Server::StartFmlePublish(int stream_id)
{
    int ret = ERROR_SUCCESS;

    double fc_publish_tid = 0;
    {
        CommonMessage*   msg = nullptr;
        FMLEStartPacket* pkt = nullptr;

        if ((ret = protocol_->ExpectMessage<FMLEStartPacket>(&msg, &pkt)) !=
            ERROR_SUCCESS) {
            rs_error("recv FCPublish message failed,ret=%d", ret);
            return ret;
        }

        rs_auto_free(CommonMessage, msg);
        rs_auto_free(FMLEStartPacket, pkt);

        fc_publish_tid = pkt->transaction_id;
    }
    {
        FMLEStartResPacket* pkt = new FMLEStartResPacket(fc_publish_tid);

        if ((ret = protocol_->SendAndFreePacket(pkt, 0)) != ERROR_SUCCESS) {
            rs_error("send FCPublish response message failed,ret=%d", ret);
            return ret;
        }
    }

    double create_stream_id = 0;
    {
        CommonMessage*      msg = nullptr;
        CreateStreamPacket* pkt = nullptr;

        if ((ret = protocol_->ExpectMessage<CreateStreamPacket>(&msg, &pkt)) !=
            ERROR_SUCCESS) {
            rs_error("recv createStream message failed,ret=%d", ret);
            return ret;
        }
        rs_auto_free(CommonMessage, msg);
        rs_auto_free(CreateStreamPacket, pkt);

        create_stream_id = pkt->transaction_id;
    }

    {
        CreateStreamResPacket* pkt =
            new CreateStreamResPacket(create_stream_id, stream_id);
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error("send createStream response message failed,ret=%d", ret);
            return ret;
        }
    }

    {
        CommonMessage* msg;
        PublishPacket* pkt;

        if ((ret = protocol_->ExpectMessage<PublishPacket>(&msg, &pkt)) !=
            ERROR_SUCCESS) {
            rs_error("recv publish message failed,ret=%d", ret);
            return ret;
        }

        rs_info("recv publish request message success,stream_name:%s,type=%s",
                pkt->stream_name.c_str(), pkt->type.c_str());

        rs_auto_free(CommonMessage, msg);
        rs_auto_free(PublishPacket, pkt);
    }
    {
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
        pkt->command_name       = RTMP_AMF0_COMMAND_ON_FC_PUBLISH;
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
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
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

int Server::RecvMessage(CommonMessage** pmsg)
{
    return protocol_->RecvMessage(pmsg);
}

void Server::SetRecvBuffer(int buffer_size)
{
    protocol_->SetRecvBuffer(buffer_size);
}

void Server::SetMargeRead(bool v, IMergeReadHandler* handler)
{
    protocol_->SetMargeRead(v, handler);
}

int Server::DecodeMessage(CommonMessage* msg, Packet** ppacket)
{
    return protocol_->DecodeMessage(msg, ppacket);
}

int Server::FMLEUnPublish(int stream_id, double unpublish_tid)
{
    int ret = ERROR_SUCCESS;
    {
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
        pkt->command_name       = RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH;
        pkt->data->Set("code", AMF0Any::String("NetStream.Unpublish.Success"));
        pkt->data->Set("description",
                       AMF0Any::String("Stop publishing stream"));
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!is_system_control_error(ret) &&
                !is_client_gracefully_close(ret)) {
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
        FMLEStartResPacket* pkt = new FMLEStartResPacket(unpublish_tid);
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!is_system_control_error(ret) &&
                !is_client_gracefully_close(ret)) {
                rs_error("send FCUnpublish response messsage failed. ret=%d",
                         ret);
            }
            return ret;
        }
    }
    {
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
        pkt->data->Set("level", AMF0Any::String("status"));
        pkt->data->Set("code", AMF0Any::String("NetStream.Unpublish.Success"));
        pkt->data->Set("description",
                       AMF0Any::String("Stream is now unpublished"));
        pkt->data->Set("clientid", AMF0Any::String("ASAICiss"));

        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            if (!is_system_control_error(ret) &&
                !is_client_gracefully_close(ret)) {
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

int Server::StartPlay(int stream_id)
{
    int ret = ERROR_SUCCESS;
    {
        UserControlPacket* pkt = new UserControlPacket;
        pkt->event_type        = (int16_t)UserEventType::STREAM_BEGIN;
        pkt->event_data        = stream_id;
        if ((ret = protocol_->SendAndFreePacket(pkt, stream_id)) !=
            ERROR_SUCCESS) {
            rs_error("send StreamBegin failed. ret=%d", ret);
            return ret;
        }

        rs_trace("send StreamBegin success");
    }
    {
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
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
        OnStatusCallPacket* pkt = new OnStatusCallPacket;
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
        OnStatusDataPacket* pkt = new OnStatusDataPacket;
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

void Server::SetAutoResponse(bool v)
{
    protocol_->SetAutoResponse(v);
}

int Server::SendAndFreeMessages(SharedPtrMessage** msgs,
                                int                nb_msgs,
                                int                stream_id)
{
    return protocol_->SendAndFreeMessages(msgs, nb_msgs, stream_id);
}

}  // namespace rtmp