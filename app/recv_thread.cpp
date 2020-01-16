#include <app/recv_thread.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>

RecvThread::RecvThread(rtmp::IMessageHandler *message_handler,
                       Server *server,
                       int32_t timeout) : message_handler_(message_handler),
                                          server_(server),
                                          timeout_(timeout)
{
    thread_ = new internal::Thread("recv", this, 0, true);
}

RecvThread::~RecvThread()
{
    Stop();
    rs_freep(thread_);
}

int32_t RecvThread::GetID()
{
    return thread_->GetID();
}

int32_t RecvThread::Start()
{
    return thread_->Start();
}

void RecvThread::Stop()
{
    thread_->Stop();
}

void RecvThread::StopLoop()
{
    thread_->StopLoop();
}

int32_t RecvThread::Cycle()
{
    int32_t ret = ERROR_SUCCESS;

    while (thread_->CanLoop())
    {
        if (!message_handler_->CanHandle())
        {
            st_usleep(timeout_ * 1000);
            continue;
        }
    }

    return ret;
}

void RecvThread::OnThreadStop()
{
}

void RecvThread::OnThreadStart()
{
}

PublishRecvThread::PublishRecvThread()
{
}

PublishRecvThread::~PublishRecvThread()
{
}