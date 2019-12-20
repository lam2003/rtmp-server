#ifndef RS_LISTENER_HPP
#define RS_LISTENER_HPP

#include <common/core.hpp>
#include <common/thread/thread.hpp>

#include <string>


class ITCPClientHandler
{
public:
    ITCPClientHandler();
    virtual ~ITCPClientHandler();

public:
    virtual int OnTCPClient(st_netfd_t stfd) = 0;
};

class TCPListener : public internal::IThreadHandler
{
public:
    TCPListener(ITCPClientHandler *client_handler, const std::string &ip, int port);
    virtual ~TCPListener();

public:
    virtual int Listen();
    virtual int GetFd();
    virtual st_netfd_t GetSTFd();

    virtual int Cycle() override;

private:
    ITCPClientHandler *client_handler_;
    std::string ip_;
    int port_;
    int fd_;
    st_netfd_t stfd_;
    internal::Thread *thread_;
};

#endif