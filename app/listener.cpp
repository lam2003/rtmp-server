
#include <app/listener.hpp>
#include <common/utils.hpp>
#include <common/st.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

#include <arpa/inet.h>

#define RS_SERVER_LISTEN_BACKLOG 512

ITCPClientHandler::ITCPClientHandler()
{
}

ITCPClientHandler::~ITCPClientHandler()
{
}

TCPListener::TCPListener(ITCPClientHandler *client_handler,
                         const std::string &ip,
                         int32_t port) : client_handler_(client_handler),
                                     ip_(ip),
                                     port_(port),
                                     fd_(-1),
                                     stfd_(nullptr)
{
    thread_ = new internal::Thread("tcp-listener", this, 0, true);
}

TCPListener::~TCPListener()
{
    thread_->Stop();
    rs_freep(thread_);
    STCloseFd(stfd_);
}

int32_t TCPListener::GetFd()
{
    return fd_;
}

st_netfd_t TCPListener::GetSTFd()
{
    return stfd_;
}

int32_t TCPListener::Listen()
{
    int32_t ret = ERROR_SUCCESS;

    if ((fd_ = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        ret = ERROR_SOCKET_CREATE;
        rs_error("create socket failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("create socket success,ep=[%s:%d]", ip_.c_str(), port_);

    int32_t reuse_socket = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int32_t)) == -1)
    {
        rs_error("set socket reuse address failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("set socket reuse address success,ep=[%s:%d]", ip_.c_str(), port_);

    int32_t tcp_keepalive = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &tcp_keepalive, sizeof(int32_t)) == -1)
    {
        rs_error("set socket keep alive failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }
    rs_verbose("set socket keep alive success,ep=[%s:%d]", ip_.c_str(), port_);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());
    if (::bind(fd_, (const sockaddr *)&addr, sizeof(sockaddr_in)) == -1)
    {
        ret = ERROR_SOCKET_BIND;
        rs_error("bind socket failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("bind socket success,ep=[%s:%d] ", ip_.c_str(), port_);

    if (::listen(fd_, RS_SERVER_LISTEN_BACKLOG) == -1)
    {
        ret = ERROR_SOCKET_LISTEN;
        rs_error("listen socket failed,ep=[%s:%d] ,ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("listen socket success,ep=[%s:%d]", ip_.c_str(), port_);

    if ((stfd_ = st_netfd_open_socket(fd_)) == nullptr)
    {
        ret = ERROR_ST_OPEN_SOCKET;
        rs_error("st_netfd_open_socket failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("st_netfd_open_socket success,ep=[%s:%d]", ip_.c_str(), port_);

    if ((ret = thread_->Start()) != ERROR_SUCCESS)
    {
        rs_error("start accept thread failed,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);
        return ret;
    }

    rs_verbose("start accept thread success,ep=[%s:%d]", ip_.c_str(), port_);
    return ret;
}

int32_t TCPListener::Cycle()
{
    int32_t ret = ERROR_SUCCESS;

    st_netfd_t client_stfd = st_accept(stfd_, nullptr, nullptr, ST_UTIME_NO_TIMEOUT);
    if (client_stfd == nullptr)
    {
        if (errno != EINTR)
        {
            rs_error("ignore accept thread stopped for accept client error,ep=[%s:%d]", ip_.c_str(), port_);
        }
        return ret;
    }

    rs_verbose("get a client,ep=[%s:%d],ret=%d", ip_.c_str(), port_, ret);

    if ((ret = client_handler_->OnTCPClient(client_stfd)) != ERROR_SUCCESS)
    {
        rs_error("handle new client failed,ep=[%s:%d]", ip_.c_str(), port_);
        return ret;
    }

    return ret;
}