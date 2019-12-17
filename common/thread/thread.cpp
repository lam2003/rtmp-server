#include <common/thread/thread.hpp>

#include <common/log/log.hpp>
#include <common/error.hpp>
#include <common/utils.hpp>

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
                                dispose_(false),
                                can_run_(false),
                                cid_(-1)

{
    if (!name_)
    {
        name_ = "";
    }
}

Thread::~Thread()
{
    Stop();
}

void *Thread::Function(void *arg)
{
    Thread *obj = (Thread *)arg;
    rs_assert(obj);

    obj->Dispatch();
}

void Thread::Dispatch()
{
    int ret = ERROR_SUCCESS;

    _context->GenerateID();

    rs_info("thread %s dispatch start", name_);

    cid_ = _context->GetID();

    rs_assert(handler_);
    handler_->OnThreadStart();

    really_terminated_ = false;

    while (loop_)
    {
        if ((ret = handler_->OnBeforeCycle()) != ERROR_SUCCESS)
        {
            rs_warn("thread %s OnBeforeCycle failed,ignored and retry,ret=%d", name_, ret);
            goto failed;
        }

        rs_info("thread %s OnBeforeCycle success", name_);
    failed:
        if(!loop_)
            break;
    }
}

} // namespace internal