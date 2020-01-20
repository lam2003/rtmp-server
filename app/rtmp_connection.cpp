#include <app/rtmp_connection.hpp>
#include <protocol/rtmp_stack.hpp>
#include <protocol/rtmp_consts.hpp>
#include <protocol/rtmp_source.hpp>
#include <common/error.hpp>
#include <common/config.hpp>
#include <common/log.hpp>

RTMPConnection::RTMPConnection(Server *server, st_netfd_t stfd) : Connection(server, stfd),
                                                                  server_(server),
                                                                  type_(rtmp::ConnType::UNKNOW)
{
    request_ = new rtmp::Request;
    response_ = new rtmp::Response;
    socket_ = new StSocket(stfd);
    rtmp_ = new RTMPServer(socket_);
}

RTMPConnection::~RTMPConnection()
{
    rs_freep(rtmp_);
    rs_freep(socket_);
    rs_freep(response_);
    rs_freep(request_);
}

int32_t RTMPConnection::DoCycle()
{
    int ret = ERROR_SUCCESS;

    rtmp_->SetRecvTimeout(RTMP_RECV_TIMEOUT_US);
    rtmp_->SetSendTimeout(RTMP_SEND_TIMEOUT_US);

    if ((ret = rtmp_->Handshake()) != ERROR_SUCCESS)
    {
        rs_error("rtmp handshake failed,ret=%d", ret);
        return ret;
    }

    if ((ret = rtmp_->ConnectApp(request_)) != ERROR_SUCCESS)
    {
        rs_error("rtmp connect app failed,ret=%d", ret);
        return ret;
    }

    request_->ip = client_ip_;

    ServiceCycle();

    return ret;
}

int32_t RTMPConnection::StreamServiceCycle()
{
    int ret = ERROR_SUCCESS;

    rtmp::ConnType type;

    if ((ret = rtmp_->IdentifyClient(response_->stream_id, type, request_->stream, request_->duration)) != ERROR_SUCCESS)
    {
        rs_error("identify client failed,ret=%d");
        return ret;
    }

    rtmp::DiscoveryTcUrl(request_->tc_url,
                         request_->schema,
                         request_->host,
                         request_->vhost,
                         request_->app,
                         request_->stream,
                         request_->port,
                         request_->param);
    request_->Strip();

    if (request_->schema.empty() || request_->vhost.empty() || request_->port.empty() || request_->app.empty())
    {
        ret = ERROR_RTMP_REQ_TCURL;
        rs_error("discovery tcUrl failed,tcUrl=%s,schema=%s,vhost=%s,port=%s,app=%s,ret=%d",
                 request_->tc_url.c_str(),
                 request_->schema.c_str(),
                 request_->vhost.c_str(),
                 request_->port.c_str(),
                 request_->app.c_str(),
                 ret);
        return ret;
    }

    if (request_->stream.empty())
    {
        ret = ERROR_RTMP_STREAM_NAME_EMPTY;
        rs_error("rtmp:empty stream name is not allowed,ret=%d", ret);
        return ret;
    }

    rtmp::Source *source = nullptr;
    if ((ret = rtmp::Source::FetchOrCreate(request_, server_, &source)) != ERROR_SUCCESS)
    {
        return ret;
    }

    type_ = type;
    switch (type)
    {
    case rtmp::ConnType::FMLE_PUBLISH:
        rs_info("FMLE start to publish stream %s", request_->stream.c_str());
        if ((ret = rtmp_->StartFmlePublish(response_->stream_id)) != ERROR_SUCCESS)
        {
            rs_error("start to publish stream failed,ret=%d",ret);
            return ret;
        }
        break;
    case rtmp::ConnType::PLAY:
        break;
    case rtmp::ConnType::UNKNOW:
        break;
    }

    return ret;
}

int32_t RTMPConnection::ServiceCycle()
{
    int ret = ERROR_SUCCESS;

    if ((ret = rtmp_->SetWindowAckSize((int)RTMP_DEFAULT_WINDOW_ACK_SIZE)) != ERROR_SUCCESS)
    {
        rs_error("set window ackowledgement size failed,ret=%d", ret);
        return ret;
    }

    if ((ret = rtmp_->SetPeerBandwidth((int)RTMP_DEFAULT_PEER_BAND_WIDTH, (int)rtmp::PeerBandwidthType::DYNAMIC)) != ERROR_SUCCESS)
    {
        rs_error("set peer bandwidth failed,ret=%d", ret);
        return ret;
    }

    std::string local_ip = Utils::GetLocalIP(st_netfd_fileno(client_stfd_));

    int chunk_size = _config->GetChunkSize(request_->vhost);
    if ((ret = rtmp_->SetChunkSize(chunk_size)) != ERROR_SUCCESS)
    {
        rs_error("set chunk size failed,ret=%d", ret);
        return ret;
    }

    if ((ret = rtmp_->ResponseConnectApp(request_, local_ip)) != ERROR_SUCCESS)
    {
        rs_error("response connect app failed,ret=%d");
        return ret;
    }

    while (!disposed_)
    {
        ret = StreamServiceCycle();
        if (ret == ERROR_SUCCESS)
        {
            continue;
        }

        return ret;
    }
    return ret;
}

void RTMPConnection::Resample()
{
}
int64_t RTMPConnection::GetSendBytesDelta()
{
    return 0;
}
int64_t RTMPConnection::GetRecvBytesDelta()
{
    return 0;
}
void RTMPConnection::CleanUp()
{
}