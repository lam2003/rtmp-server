#include <common/thread/thread.hpp>

#include <common/log/log.hpp>
#include <common/error.hpp>

namespace internal
{
IThreadHandler::IThreadHandler()
{
}

IThreadHandler::~IThreadHandler()
{
}

int IThreadHandler::OnBeforeCycle()
{
    return ERROR_SUCCESS;
}

void IThreadHandler::OnThreadStart()
{
}

int IThreadHandler::Cycle()
{
    return ERROR_SUCCESS;
}

int IThreadHandler::OnEndCycle()
{
    return ERROR_SUCCESS;
}

void IThreadHandler::OnThreadStop()
{
}

Thread::Thread(const char *name,
               IThreadHandler *handler,
               int64_t interval_us,
               bool joinable) : name_(name),
                                handler_(handler),
                                interval_us_(interval_us),
                                joinable_(joinable),
                                st_(nullptr),
                                loop_(false),
                                really_terminated_(false),
                                disposed_(false),
                                can_run_(false),
                                cid_(-1)

{
}

Thread::~Thread()
{
    Stop();
}

void *Thread::Function(void *arg)
{
    Thread *obj = (Thread *)arg;

    obj->Dispatch();

    st_thread_exit(nullptr);
}

void Thread::Dispatch()
{
    int ret = ERROR_SUCCESS;

    _context->GenerateID();

    rs_info("thread %s start", name_);

    cid_ = _context->GetID();

    handler_->OnThreadStart();

    really_terminated_ = false;

    while (!can_run_ && loop_)
    {
        st_usleep(10 * 1000);
    }

    while (loop_)
    {
        if ((ret = handler_->OnBeforeCycle()) != ERROR_SUCCESS)
        {
            rs_warn("thread %s OnBeforeCycle failed,ignored and retry,ret=%d", name_, ret);
            goto failed;
        }

        rs_verbose("thread %s OnBeforeCycle success", name_);

        if ((ret = handler_->Cycle()) != ERROR_SUCCESS)
        {
            rs_warn("thread %s Cycle failed,ignore and retry,ret=%d", name_, ret);
            goto failed;
        }

        rs_verbose("thread %s Cycle success", name_);

        if ((ret = handler_->OnEndCycle()) != ERROR_SUCCESS)
        {
            rs_warn("thread %s OnEndCycle failed,ignore and retry,ret=%d", name_, ret);
            goto failed;
        }

        rs_verbose("thread %s OnEndCycle success", name_);
    failed:
        if (!loop_)
        {
            break;
        }
        if (interval_us_ != 0)
        {
            st_usleep(interval_us_);
        }
    }

    really_terminated_ = true;
    handler_->OnThreadStop();
    rs_info("thread %s quit", name_);
    _context->ClearID();
}

int Thread::Start()
{
    int ret = ERROR_SUCCESS;
    if (st_)
    {
        rs_warn("thread %s already running", name_);
        return ret;
    }

    if ((st_ = st_thread_create(Thread::Function, this, (joinable_ ? 1 : 0), 0)) == nullptr)
    {
        ret = ERROR_ST_CREATE_CYCLE_THREAD;
        rs_error("thread %s,st_thread_create failed,ret=%d", name_, ret);
        return ret;
    }

    disposed_ = false;
    loop_ = true;

    while (cid_ == 0)
    {
        st_usleep(10 * 1000);
    }

    can_run_ = true;
    return ret;
}

void Thread::Dispose()
{
    if (disposed_)
    {
        return;
    }

    st_thread_interrupt(st_);

    if (joinable_)
    {
        st_thread_join(st_, nullptr);
    }

    while (!really_terminated_)
    {
        st_usleep(10 * 1000);
    }

    disposed_ = true;
}

void Thread::Stop()
{
    if (!st_)
    {
        return;
    }

    loop_ = false;

    Dispose();

    cid_ = -1;
    can_run_ = false;
    st_ = nullptr;
}

bool Thread::CanLoop()
{
    return loop_;
}

void Thread::StopLoop()
{
    loop_ = false;
}
} // namespace internal