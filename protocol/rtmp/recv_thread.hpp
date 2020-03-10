/*
 * @Author: linmin
 * @Date: 2020-02-06 17:02:50
 * @LastEditTime: 2020-03-10 15:41:27
 */
#ifndef RS_RTMP_RECV_THREAD_HPP
#define RS_RTMP_RECV_THREAD_HPP

#include <common/core.hpp>
#include <common/thread.hpp>
#include <protocol/rtmp/stack.hpp>
#include <protocol/rtmp/connection.hpp>
#include <app/rtmp_server.hpp>
namespace rtmp
{
class Connection;

class RecvThread : virtual public internal::IThreadHandler
{
public:
    RecvThread(rtmp::IMessageHandler *handler, RTMPServer *rtmp, int32_t timeout_ms);
    virtual ~RecvThread();

public:
    virtual int32_t GetID();
    virtual int32_t Start();
    virtual void Stop();
    virtual void StopLoop();

    // internal::IThreadHandler
    virtual void OnThreadStart() override;
    virtual int32_t Cycle() override;
    virtual void OnThreadStop() override;

private:
    internal::Thread *thread_;
    rtmp::IMessageHandler *handler_;
    RTMPServer *rtmp_;
    int32_t timeout_;
};

class PublishRecvThread : virtual public rtmp::IMessageHandler,
                          virtual public IMergeReadHandler,
                          virtual public IReloadHandler
{
public:
    PublishRecvThread(RTMPServer *rtmp,
                      rtmp::Request *request,
                      int mr_socket_fd,
                      int timeout_ms,
                      rtmp::Connection *conn,
                      rtmp::Source *source,
                      bool is_fmle,
                      bool is_edge);
    virtual ~PublishRecvThread();

public:
    virtual int Wait(int timeout_ms);
    virtual int Start();
    virtual void Stop();
    virtual bool CanHandle();
    virtual int GetCID();
    virtual void SetCID(int cid);
    virtual int ErrorCode();
    virtual int64_t GetMsgNum();
    //rtmp::IMessageHandler
    virtual void OnThreadStart() override;
    virtual void OnThreadStop() override;
    virtual void OnRead(ssize_t nread) override;
    virtual int Handle(rtmp::CommonMessage *msg) override;
    virtual void OnRecvError(int32_t ret) override;

private:
    void set_socket_buffer(int sleep_ms);

private:
    RecvThread *thread_;
    RTMPServer *rtmp_;
    rtmp::Request *request_;
    int64_t nb_msgs_;
    uint64_t video_frames_;
    bool mr_;
    int mr_fd_;
    int mr_sleep_;
    bool real_time_;
    int recv_error_code_;
    rtmp::Connection *conn_;
    rtmp::Source *source_;
    bool is_fmle_;
    bool is_edge_;
    st_cond_t error_;
    int cid;
    int ncid;
};

} // namespace rtmp

#endif