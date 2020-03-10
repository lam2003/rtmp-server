/*
 * @Author: linmin
 * @Date: 2020-02-10 16:16:08
 * @LastEditTime: 2020-03-10 16:32:49
 */
#ifndef RS_RTMP_CONNECTION_HPP
#define RS_RTMP_CONNECTION_HPP

#include <common/core.hpp>
#include <common/socket.hpp>
#include <common/connection.hpp>
#include <app/rtmp_server.hpp>
#include <app/server.hpp>

namespace rtmp
{
    
class PublishRecvThread;

class Connection : virtual public IConnection
{
    friend class PublishRecvThread;

public:
    Connection(Server *server, st_netfd_t stfd);
    virtual ~Connection();

public:
    //IKbpsDelta
    virtual void Resample() override;
    virtual int64_t GetSendBytesDelta() override;
    virtual int64_t GetRecvBytesDelta() override;
    virtual void CleanUp() override;

protected:
    virtual int32_t StreamServiceCycle();
    virtual int32_t ServiceCycle();
    virtual int32_t Publishing(Source *source);
    //IConnection
    virtual int32_t DoCycle() override;

private:
    int handle_publish_message(Source *source, CommonMessage *msg, bool is_fmle, bool is_edge);
    int process_publish_message(Source *source, CommonMessage *msg, bool is_edge);
    int do_publishing(Source *source, PublishRecvThread *recv_thread);
    void set_socket_option();
    int acquire_publish(Source *source, bool is_edge);

private:
    Server *server_;
    StSocket *socket_;
    RTMPServer *rtmp_;
    Request *request_;
    Response *response_;
    ConnType type_;
    bool tcp_nodelay_;

    int publish_first_pkt_timeout_;
    int publish_normal_pkt_timeout_;
};
} // namespace rtmp

#endif