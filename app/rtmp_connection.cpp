#include <app/rtmp_connection.hpp>
#include <protocol/rtmp_stack.hpp>


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
    rtmp::CommonMessage *msg;
    rtmp_->Handshake();
    rtmp_->RecvMessage(&msg);
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