/*
 * @Author: linmin
 * @Date: 2020-02-18 12:52:35
 * @LastEditTime: 2020-02-18 13:03:16
 */
#ifndef RS_RTMP_EDGE_HPP
#define RS_RTMP_EDGE_HPP

#include <common/core.hpp>
#include <common/thread.hpp>
#include <common/io.hpp>
#include <common/kbps.hpp>
#include <protocol/rtmp_source.hpp>
#include <protocol/rtmp_stack.hpp>

namespace rtmp
{

enum EdgeState
{
    INIT = 0,
    PLAY = 100,
    INGEST_CONNECTED = 201,
    PUBLISH = 200,
};



class PublishEdge;



class EdgeForwarder : public internal::IThreadHandler
{
public:
    EdgeForwarder();
    virtual ~EdgeForwarder();

public:
    virtual int Initialize();
    virtual void SetQueueSize(double queue_size);
    virtual int Start();
    virtual void Stop();
    virtual int Proxy(CommonMessage *msg);
    //internal::IThreadHandler
    virtual int Cycle() override;

private:
    virtual void CloseUnderLayerSocket();
    virtual int ConnectServer(const std::string &ep_server, const std::string &ep_port);
    virtual int ConnectApp(const std::string &ep_server, const std::string &ep_port);

private:
    int stream_id_;
    Source *source_;
    PublishEdge *edge_;
    Request request_;
    internal::Thread *thread_;
    st_netfd_t stfd_;
    IProtocolReaderWriter *rw_;
    Kbps *kbps_;
    int origin_index_;
};

class PublishEdge
{
public:
    PublishEdge();
    virtual ~PublishEdge();

public:
    int Initialize(Source *source, Request *req);

private:
    EdgeState state_;
};

} // namespace rtmp

#endif