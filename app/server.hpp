#ifndef RS_SERVER_HPP
#define RS_SERVER_HPP

#include <common/core.hpp>
#include <app/listener.hpp>

#include <string>

enum class ListenerType
{
    RTMP_STEAM = 0,
};

class Server;

class IServerListener
{
public:
    IServerListener(Server *server, ListenerType type);
    virtual ~IServerListener();

public:
    virtual int Listen(const std::string &ip, int port) = 0;
    virtual ListenerType GetType();

protected:
    Server *server_;
    ListenerType type_;
    std::string ip_;
    int port_;
};

class RTMPStreamListener : virtual public IServerListener, virtual public ITCPClientHandler
{
public:
    RTMPStreamListener(Server *server, ListenerType type);
    virtual ~RTMPStreamListener();

public:
    virtual int Listen(const std::string &ip, int port) override;
    virtual int OnTCPClient(st_netfd_t stfd) override;

private:
    TCPListener *listener_;
};

class Server
{
public:
    Server();
    virtual ~Server();

public:
    virtual int Initilaize();
    virtual int InitializeST();

    virtual int AcceptClient(ListenerType type, st_netfd_t stfd);
};

#endif