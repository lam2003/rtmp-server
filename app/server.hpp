#ifndef RS_SERVER_HPP
#define RS_SERVER_HPP

#include <common/core.hpp>
#include <common/listener.hpp>
#include <common/connection.hpp>
#include <protocol/rtmp/source.hpp>

#include <string>

enum class ListenerType
{
    RTMP = 0,
};

class Server;

class IServerListener
{
public:
    IServerListener(Server *server, ListenerType type);
    virtual ~IServerListener();

public:
    virtual int32_t Listen(const std::string &ip, int32_t port) = 0;
    virtual ListenerType GetType();

protected:
    Server *server_;
    ListenerType type_;
    std::string ip_;
    int32_t port_;
};

class RTMPStreamListener : virtual public IServerListener,
                           virtual public ITCPClientHandler
{
public:
    RTMPStreamListener(Server *server, ListenerType type);
    virtual ~RTMPStreamListener();

public:
    //IServerListener
    virtual int32_t Listen(const std::string &ip, int32_t port) override;
    //ITCPClientHandler
    virtual int32_t OnTCPClient(st_netfd_t stfd) override;

private:
    TCPListener *listener_;
};

class Server : virtual public IConnectionManager,
               virtual public rtmp::ISourceHandler
{
public:
    Server();
    virtual ~Server();

public:
    virtual int32_t Initilaize();
    virtual int32_t InitializeST();
    virtual int32_t Listen();
    virtual int32_t AcceptClient(ListenerType type, st_netfd_t stfd);
    //IConnectionManager
    virtual void OnRemove(IConnection *conn) override;
    //rtmp::ISourceHandler
    virtual int OnPublish(rtmp::Source *s, rtmp::Request *r) override;
    virtual int OnUnPublish(rtmp::Source *s, rtmp::Request *r) override;

protected:
    virtual int32_t ListenRTMP();
};

#endif