#include <app/server.hpp>
#include <common/utils/utils.hpp>
#include <common/log/log.hpp>
#include <common/error.hpp>
#include <common/io/st.hpp>

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

int RTMPStreamListener::Listen(const std::string &ip, int port)
{
    int ret = ERROR_SUCCESS;

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

int RTMPStreamListener::OnTCPClient(st_netfd_t stfd)
{
    STCloseFd(stfd);
    return 0;
}

Server::Server()
{
}

Server::~Server()
{
}

int Server::InitializeST()
{
    int ret = ERROR_SUCCESS;

    if ((ret = STInit()) != ERROR_SUCCESS)
    {
        rs_error("STInit failed,ret=%d", ret);
        return ret;
    }

    _context->GenerateID();

    rs_verbose("rtmp server main cid=%d,pid=%d", _context->GetID(), ::getpid());

    return ret;
}

int Server::Initilaize()
{
    return 0;
}

int Server::AcceptClient(ListenerType type, st_netfd_t stfd)
{
    int ret = ERROR_SUCCESS;

    int fd = st_netfd_fileno(stfd);

    int val;
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

    return ret;
}