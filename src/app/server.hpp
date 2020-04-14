/*
 * @Date: 2020-02-24 11:23:35
 * @LastEditors: linmin
 * @LastEditTime: 2020-03-17 18:46:20
 */
#ifndef RS_SERVER_HPP
#define RS_SERVER_HPP

#include <common/connection.hpp>
#include <common/core.hpp>
#include <common/listener.hpp>
#include <protocol/rtmp/source.hpp>

#include <string>

enum class ListenerType {
    RTMP = 0,
};

class StreamServer;

class IServerListener {
  public:
    IServerListener(StreamServer* server, ListenerType type);
    virtual ~IServerListener();

  public:
    virtual int32_t      Listen(const std::string& ip, int32_t port) = 0;
    virtual ListenerType GetType();

  protected:
    StreamServer* server_;
    ListenerType  type_;
    std::string   ip_;
    int32_t       port_;
};

class RTMPStreamListener : virtual public IServerListener,
                           virtual public ITCPClientHandler {
  public:
    RTMPStreamListener(StreamServer* server, ListenerType type);
    virtual ~RTMPStreamListener();

  public:
    // IServerListener
    virtual int32_t Listen(const std::string& ip, int32_t port) override;
    // ITCPClientHandler
    virtual int32_t OnTCPClient(st_netfd_t stfd) override;

  private:
    TCPListener* listener_;
};

class StreamServer : virtual public IConnectionManager,
                     virtual public rtmp::ISourceHandler {
  public:
    StreamServer();
    virtual ~StreamServer();

  public:
    virtual int32_t Initilaize();
    virtual int32_t InitializeST();
    virtual int32_t Listen();
    virtual int32_t AcceptClient(ListenerType type, st_netfd_t stfd);
    // IConnectionManager
    virtual void OnRemove(IConnection* conn) override;
    // rtmp::ISourceHandler
    virtual int OnPublish(rtmp::Source* s, rtmp::Request* r) override;
    virtual int OnUnPublish(rtmp::Source* s, rtmp::Request* r) override;

  protected:
    virtual int32_t listen_rtmp();

  private:
    std::vector<IConnection*> conns_;
};

#endif