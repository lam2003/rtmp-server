#include <common/log/fast_log.hpp>

#include <common/utils.hpp>

#include <sys/time.h>
#include <time.h>

#define LOG_MAX_SIZE 4096
#define LOG_TAIL '\n'
#define LOG_TAIL_SIZE 1

FastLog::FastLog() : log_data(new char[LOG_MAX_SIZE]),
                     fd(-1),
                     log_to_file_tank(false),
                     utc(false)
{
}

FastLog::~FastLog()
{
    rs_freepa(log_data);
    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }
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

bool FastLog::GenerateHeader(bool error, const char *tag, int context_id, const char *level_name, int *header_size)
{
    timeval tv;
    if (gettimeofday(&tv, nullptr) == -1)
        return false;

    struct tm *tm;
    if (utc)
    {
        if ((tm = gmtime(&tv.tv_sec)) == nullptr)
            return false;
    }
    else
    {
        if ((tm = localtime(&tv.tv_sec)) == nullptr)
            return false;
    }

    int log_header_size = -1;

    if (error)
    {
        if (tag)
        {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id, errno);
        }
        else
        {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, getpid(), context_id, errno);
        }
    }
    else
    {
        if (tag)
        {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d] ",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id);
        }
        else
        {
            log_header_size = snprintf(log_data, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d] ",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, getpid(), context_id);
        }
    }
    if (log_header_size == -1)
        return false;
    *header_size = rs_min(log_header_size, LOG_MAX_SIZE - 1);
    return true;
}
void FastLog::WriteLog(int &fd, char *str_log, int size, int level)
{
    size = rs_min(LOG_MAX_SIZE - 1, size);
    str_log[size++] = LOG_TAIL;
    if (!log_to_file_tank)
    {
        if (level <= LogLevel::TRACE)
        {
            printf("%.*s", size, str_log);
        }
        else if (level == LogLevel::WARN)
        {
            printf("\033[33m%.*s\033[33m", size, str_log);
        }
        else
        {
            printf("\033[31m%.*s\033[33m", size, str_log);
        }
        fflush(stdout);
    }
    return;
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
    std::map<st_thread_t, int>::iterator it = cache_.find(st_thread_self());
    if (it != cache_.end())
        cache_.erase(it);
}