#ifndef RS_THREAD_HPP
#define RS_THREAD_HPP

#include <common/core.hpp>

#include <st.h>

#include <string>

namespace internal
{
class IThreadHandler
{
public:
    IThreadHandler();
    virtual ~IThreadHandler();

public:
    virtual int32_t OnBeforeCycle();
    virtual void OnThreadStart();
    virtual int32_t Cycle();
    virtual int32_t OnEndCycle();
    virtual void OnThreadStop();
};

class Thread
{
public:
    Thread(const std::string &name, IThreadHandler *thread_handler, int64_t interval_us, bool joinable);
    virtual ~Thread();

public:
    virtual int32_t Start();
    virtual void Stop();
    virtual bool CanLoop();
    virtual void StopLoop();
    virtual int32_t GetID();

protected:
    virtual void Dispatch();
    virtual void Dispose();
    static void *Function(void *arg);

private:
    std::string name_;
    IThreadHandler *handler_;
    int64_t interval_us_;
    bool joinable_;
    st_thread_t st_;
    bool loop_;
    bool really_terminated_;
    bool disposed_;
    bool can_run_;
    int32_t cid_;
};

} // namespace internal

#endif