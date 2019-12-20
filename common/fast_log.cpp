#include <common/fast_log.hpp>
#include <common/utils.hpp>

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define LOG_MAX_SIZE 4096
#define LOG_TAIL '\n'
#define LOG_TAIL_SIZE 1

FastLog::FastLog() : log_data_(new char[LOG_MAX_SIZE]),
                     fd_(-1),
                     log_to_file_tank_(false),
                     utc_(false)
{
}

FastLog::~FastLog()
{
    rs_freepa(log_data_);
    if (fd_ > 0)
    {
        close(fd_);
        fd_ = -1;
    }
}

void FastLog::Verbose(const char *tag, int context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::VERBOSE)
        return;
    int size = 0;
    if (!GenerateHeader(false, tag, context_id, "VERBOSE", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::VERBOSE);
}
void FastLog::Info(const char *tag, int context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::INFO)
        return;
    int size = 0;
    if (!GenerateHeader(false, tag, context_id, "INFO", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::INFO);
}
void FastLog::Trace(const char *tag, int context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int size = 0;
    if (!GenerateHeader(false, tag, context_id, "TRACE", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::TRACE);
}
void FastLog::Warn(const char *tag, int context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int size = 0;
    if (!GenerateHeader(false, tag, context_id, "WARN", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::WARN);
}
void FastLog::Error(const char *tag, int context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int size = 0;
    if (!GenerateHeader(true, tag, context_id, "ERROR", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    if (errno != 0 && size < LOG_MAX_SIZE)
        size += snprintf(log_data_ + size, LOG_MAX_SIZE - size, "(%s)", strerror(errno));
    WriteLog(fd_, log_data_, size, LogLevel::ERROR);
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
    if (utc_)
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
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id, errno);
        }
        else
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, getpid(), context_id, errno);
        }
    }
    else
    {
        if (tag)
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d] ",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id);
        }
        else
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
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
void FastLog::WriteLog(int &fd_, char *str_log, int size, int level)
{
    size = rs_min(LOG_MAX_SIZE - 1, size);
    str_log[size++] = LOG_TAIL;
    if (!log_to_file_tank_)
    {
        if (level == LogLevel::VERBOSE)
        {
            printf("\033[36m%.*s\033[0m", size, str_log);
        }
        else if (level == LogLevel::TRACE)
        {
            printf("\033[34m%.*s\033[0m", size, str_log);
        }
        else if (level == LogLevel::INFO)
        {
            printf("\033[32m%.*s\033[0m", size, str_log);
        }
        else if (level == LogLevel::WARN)
        {
            printf("\033[33m%.*s\033[0m", size, str_log);
        }
        else
        {
            printf("\033[31m%.*s\033[0m", size, str_log);
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
    static int id = 100;

    int cid = id++;
    cache_[st_thread_self()] = cid;
    return cid;
}
int ThreadContext::GetID()
{
    st_thread_t st = st_thread_self();
    if (st == nullptr)
        return -1;
    return cache_[st];
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