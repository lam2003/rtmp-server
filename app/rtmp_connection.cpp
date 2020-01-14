#include <app/rtmp_connection.hpp>
#include <protocol/rtmp_stack.hpp>
#include <protocol/rtmp_consts.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

RTMPConnection::RTMPConnection(Server *server, st_netfd_t stfd) : Connection(server, stfd)
{
    request_ = new rtmp::Request;
    socket_ = new StSocket(stfd);
    rtmp_ = new RTMPServer(socket_);
}

RTMPConnection::~RTMPConnection()
{
    rs_freep(rtmp_);
    rs_freep(socket_);
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

int32_t RTMPConnection::ServiceCycle()
{
    int ret = ERROR_SUCCESS;

    if ((ret = rtmp_->SetWindowAckSize((int)RTMP_DEFAULT_WINDOW_ACK_SIZE)) != ERROR_SUCCESS)
    {
        rs_error("set window ackowledgement size failed,ret=%d",ret);
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