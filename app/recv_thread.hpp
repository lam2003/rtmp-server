#ifndef RS_RECV_THREAD_HPP
#define RS_RECV_THREAD_HPP

#include <common/core.hpp>
#include <common/thread.hpp>
#include <protocol/rtmp_message.hpp>
// #include <protocol/buffer.hpp>
#include <app/server.hpp>

class RecvThread : virtual public internal::IThreadHandler
{
public:
    RecvThread(IMessageHandler *messgae_handler, Server *server, int32_t timeout);
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
    IMessageHandler *message_handler_;
    Server *server_;
    int32_t timeout_;
    internal::Thread *thread_;
};

class PublishRecvThread : virtual public IMessageHandler
{
public:
    PublishRecvThread();
    virtual ~PublishRecvThread();
};

#endif