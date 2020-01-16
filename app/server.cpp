#include <app/server.hpp>
#include <app/rtmp_connection.hpp>
#include <common/utils.hpp>
#include <common/log.hpp>
#include <common/error.hpp>
#include <common/st.hpp>

#include <unistd.h>
#include <fcntl.h>

IServerListener::IServerListener(Server *server, ListenerType type) : server_(server),
                                                                      type_(type),
                                                                      ip_(""),
                                                                      port_(-1)
{
}

IServerListener::~IServerListener()
{
}

ListenerType IServerListener::GetType()
{
    return type_;
}

RTMPStreamListener::RTMPStreamListener(Server *server, ListenerType type) : IServerListener(server, type),
                                                                            listener_(nullptr)
{
}

RTMPStreamListener::~RTMPStreamListener()
{
    rs_freep(listener_);
}

int32_t RTMPStreamListener::Listen(const std::string &ip, int32_t port)
{
    int32_t ret = ERROR_SUCCESS;

    ip_ = ip;
    port_ = port;

    rs_freep(listener_);
    listener_ = new TCPListener(this, ip, port);

    if ((ret = listener_->Listen()) != ERROR_SUCCESS)
    {
        rs_error("tcp listen failed,ep=[%s:%d],ret=%d", ip.c_str(), port, ret);
        return ret;
    }

    rs_info("RTMP streamer listen on [%s:%d]", ip.c_str(), port);

    return ret;
}

int32_t RTMPStreamListener::OnTCPClient(st_netfd_t stfd)
{
    int ret = ERROR_SUCCESS;
    if ((ret = server_->AcceptClient(ListenerType::RTMP, stfd)) != ERROR_SUCCESS)
    {
        rs_error("accpet client failed,ret=%d", ret);
        return ret;
    }
    return ret;
}

Server::Server()
{
}

Server::~Server()
{
}

int32_t Server::InitializeST()
{
    int32_t ret = ERROR_SUCCESS;

    if ((ret = STInit()) != ERROR_SUCCESS)
    {
        rs_error("STInit failed,ret=%d", ret);
        return ret;
    }

    _context->GenerateID();

    rs_verbose("rtmp server main cid=%d,pid=%d", _context->GetID(), ::getpid());

    return ret;
}

int32_t Server::Initilaize()
{
    return 0;
}

int32_t Server::AcceptClient(ListenerType type, st_netfd_t stfd)
{
    int32_t ret = ERROR_SUCCESS;

    int32_t fd = st_netfd_fileno(stfd);

    int32_t val;
    if ((val = fcntl(fd, F_GETFD, 0)) < 0)
    {
        ret = ERROR_SYSTEM_PID_GET_FILE_INFO;
        rs_error("fcntl F_GETFD failed,fd=%d,ret=%d", fd, ret);
        return ret;
    }
    val |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, val) < 0)
    {
        ret = ERROR_SYSTEM_PID_SET_FILE_INFO;
        rs_error("fcntl F_SETFD failed,fd=%d,ret=%d", fd, ret);
        return ret;
    }

    Connection *conn = nullptr;
    if (type == ListenerType::RTMP)
    {
        conn = new RTMPConnection(this, stfd);
    }

    if ((ret = conn->Start()) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int32_t Server::Listen()
{
    int ret = ERROR_SUCCESS;

    if ((ret = ListenRTMP()) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int32_t Server::ListenRTMP()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

void Server::OnRemove(Connection *conn)
{
}