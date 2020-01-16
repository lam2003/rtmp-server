#ifndef RS_LISTENER_HPP
#define RS_LISTENER_HPP

#include <common/core.hpp>
#include <common/thread.hpp>

#include <string>


class ITCPClientHandler
{
public:
    ITCPClientHandler();
    virtual ~ITCPClientHandler();

public:
    virtual int32_t OnTCPClient(st_netfd_t stfd) = 0;
};

class TCPListener : public internal::IThreadHandler
{
public:
    TCPListener(ITCPClientHandler *client_handler, const std::string &ip, int32_t port);
    virtual ~TCPListener();

public:
    virtual int32_t Listen();
    virtual int32_t GetFd();
    virtual st_netfd_t GetSTFd();

    virtual int32_t Cycle() override;

private:
    ITCPClientHandler *client_handler_;
    std::string ip_;
    int32_t port_;
    int32_t fd_;
    st_netfd_t stfd_;
    internal::Thread *thread_;
};

#endif