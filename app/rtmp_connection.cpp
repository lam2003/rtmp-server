#include <app/rtmp_connection.hpp>
#include <protocol/rtmp_stack.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

RTMPConnection::RTMPConnection(Server *server, st_netfd_t stfd) : Connection(server, stfd)
{
    socket_ = new StSocket(stfd);
    rtmp_ = new RTMPServer(socket_);
}

RTMPConnection::~RTMPConnection()
{
}

int32_t RTMPConnection::DoCycle()
{
    int ret = ERROR_SUCCESS;
    if ((ret = rtmp_->Handshake()) != ERROR_SUCCESS)
    {
        rs_error("rtmp handshake failed,ret=%d", ret);
        return ret;
    }

    rtmp::CommonMessage *msg;
    rtmp_->RecvMessage(&msg);

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