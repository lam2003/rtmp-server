#include <app/rtmp_connection.hpp>

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
    return rtmp_->Handshake();
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