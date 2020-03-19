/*
 * @Author: linmin
 * @Date: 2020-02-10 16:16:08
 * @LastEditTime: 2020-03-18 15:06:59
 */
#ifndef RS_RTMP_CONNECTION_HPP
#define RS_RTMP_CONNECTION_HPP

#include <common/connection.hpp>
#include <common/core.hpp>
#include <common/socket.hpp>

class Server;
class RTMPServer;

namespace rtmp {

enum class ConnType;
class PublishRecvThread;
class QueueRecvThread;
class Source;
class Consumer;
class Response;
class Request;
class CommonMessage;
class IWakeable;

class Connection : virtual public IConnection {
    friend class PublishRecvThread;

  public:
    Connection(Server* server, st_netfd_t stfd);
    virtual ~Connection();

  public:
    virtual void Dispose();
    // IKbpsDelta
    virtual void    Resample() override;
    virtual int64_t GetSendBytesDelta() override;
    virtual int64_t GetRecvBytesDelta() override;
    virtual void    CleanUp() override;

  protected:
    virtual int32_t StreamServiceCycle();
    virtual int32_t ServiceCycle();
    virtual int32_t Publishing(Source* source);
    virtual int32_t Playing(Source* source);
    // IConnection
    virtual int32_t do_cycle() override;

  private:
    int handle_publish_message(Source*        source,
                               CommonMessage* msg,
                               bool           is_fmle,
                               bool           is_edge);
    int
         process_publish_message(Source* source, CommonMessage* msg, bool is_edge);
    int  do_publishing(Source* source, PublishRecvThread* recv_thread);
    void set_socket_option();
    int  acquire_publish(Source* source, bool is_edge);
    int  do_playing(Source*          source,
                    Consumer*        consumer,
                    QueueRecvThread* recv_thread);

  private:
    Server*     server_;
    StSocket*   socket_;
    RTMPServer* rtmp_;
    Request*    request_;
    Response*   response_;
    ConnType    type_;
    bool        tcp_nodelay_;
    int         mw_sleep_;
    IWakeable*  wakeable_;

    int publish_first_pkt_timeout_;
    int publish_normal_pkt_timeout_;
};
}  // namespace rtmp

#endif