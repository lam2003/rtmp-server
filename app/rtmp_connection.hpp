/*
 * @Author: linmin
 * @Date: 2020-02-10 16:16:08
 * @LastEditTime: 2020-02-10 17:37:01
 */
#ifndef RS_RTMP_CONNECTION_HPP
#define RS_RTMP_CONNECTION_HPP

#include <common/core.hpp>
#include <common/socket.hpp>
#include <common/connection.hpp>
#include <app/rtmp_server.hpp>
#include <app/server.hpp>
#include <app/rtmp_recv_thread.hpp>

class PublishRecvThread;

class RTMPConnection : virtual public Connection
{
    friend class PublishRecvThread;

public:
    RTMPConnection(Server *server, st_netfd_t stfd);
    virtual ~RTMPConnection();

public:
    //IKbpsDelta
    virtual void Resample() override;
    virtual int64_t GetSendBytesDelta() override;
    virtual int64_t GetRecvBytesDelta() override;
    virtual void CleanUp() override;

protected:
    virtual int32_t StreamServiceCycle();
    virtual int32_t ServiceCycle();
    virtual int32_t Publishing(rtmp::Source *source);
    //Connection
    virtual int32_t DoCycle() override;

private:
    int handle_publish_message(rtmp::Source *source, rtmp::CommonMessage *msg, bool is_fmle, bool is_edge);
    int process_publish_message(rtmp::Source *source, rtmp::CommonMessage *msg, bool is_edge);
    int do_publishing(rtmp::Source *source, PublishRecvThread *recv_thread);
    void set_socket_option();
    int acquire_publish(rtmp::Source *source, bool is_edge);

private:
    Server *server_;
    StSocket *socket_;
    RTMPServer *rtmp_;
    rtmp::Request *request_;
    rtmp::Response *response_;
    rtmp::ConnType type_;
    bool tcp_nodelay_;

    int publish_first_pkt_timeout_;
    int publish_normal_pkt_timeout_;
};

#endif