#include <common/log/fast_log.hpp>

FastLog::FastLog()
{
}

FastLog::~FastLog()
{
}

void FastLog::Verbose(const char *tag, int context_id, const char *fmt, ...)
{
}
void FastLog::Info(const char *tag, int context_id, const char *fmt, ...)
{
}
void FastLog::Trace(const char *tag, int context_id, const char *fmt, ...)
{
}
void FastLog::Warn(const char *tag, int context_id, const char *fmt, ...)
{
}
void FastLog::Error(const char *tag, int context_id, const char *fmt, ...)
{
}

int FastLog::OnReloadUTCTime()
{
}
int FastLog::OnReloadLogTank()
{
}
int FastLog::OnReloadLogLevel()
{
}
int FastLog::OnReloadLogFile()
{
}

bool FastLog::GenerateHeader(bool error, const char *tag, int context_id, const char *level_name, int header_size)
{
}
void FastLog::WriteLog(int &fd, char *str_log, int size, int level)
{
}
void FastLog::OpenLogFile()
{
}

ThreadContext::ThreadContext()
{
}

ThreadContext::~ThreadContext()
{
}

int ThreadContext::GenerateID()
{
    static int id = 10000;

    int cid = id++;
    cache_[st_thread_self()] = cid;
    return cid;
}
int ThreadContext::GetID()
{
    return cache_[st_thread_self()];
}
int ThreadContext::SetID(int v)
{
    st_thread_t self = st_thread_self();

    int old_v = 0;
    if (cache_.find(self) != cache_.end())
        old_v = cache_[self];
    return old_v;
}

void ThreadContext::ClearID()
{
    std::map<st_thread_t,int>::iterator it = cache_.find(st_thread_self());
    if(it != cache_.end())
        cache_.erase(it);
}