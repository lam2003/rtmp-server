#include <common/log.hpp>
#include <common/error.hpp>
#include <common/utils.hpp>

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#define LOG_MAX_SIZE 4096
#define LOG_TAIL '\n'
#define LOG_TAIL_SIZE 1

ILog::ILog()
{
}

ILog::~ILog()
{
}

void ILog::Verbose(const char *, int32_t, const char *, ...)
{
}

void ILog::Info(const char *, int32_t, const char *, ...)
{
}

void ILog::Trace(const char *, int32_t, const char *, ...)
{
}

void ILog::Warn(const char *, int32_t, const char *, ...)
{
}

void ILog::Error(const char *, int32_t, const char *, ...)
{
}

IThreadContext::IThreadContext()
{
}

IThreadContext::~IThreadContext()
{
}

int32_t IThreadContext::GenerateID()
{
    return 0;
}

int32_t IThreadContext::GetID()
{
    return 0;
}

int32_t IThreadContext::SetID(int32_t)
{
    return 0;
}

void IThreadContext::ClearID()
{
}

FastLog::FastLog()
{
    fd_ = -1;
    log_to_file_tank_ = false;
    utc_ = false;
    log_data_ = new char[LOG_MAX_SIZE];
    level_ = LogLevel::VERBOSE;
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

void FastLog::Verbose(const char *tag, int32_t context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::VERBOSE)
        return;
    int32_t size = 0;
    if (!GenerateHeader(false, tag, context_id, "VERBOSE", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::VERBOSE);
}
void FastLog::Info(const char *tag, int32_t context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::INFO)
        return;
    int32_t size = 0;
    if (!GenerateHeader(false, tag, context_id, "INFO", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::INFO);
}
void FastLog::Trace(const char *tag, int32_t context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int32_t size = 0;
    if (!GenerateHeader(false, tag, context_id, "TRACE", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::TRACE);
}
void FastLog::Warn(const char *tag, int32_t context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int32_t size = 0;
    if (!GenerateHeader(false, tag, context_id, "WARN", &size))
        return;
    va_list ap;
    va_start(ap, fmt);
    size += vsnprintf(log_data_ + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    WriteLog(fd_, log_data_, size, LogLevel::WARN);
}
void FastLog::Error(const char *tag, int32_t context_id, const char *fmt, ...)
{
    if (level_ > LogLevel::TRACE)
        return;
    int32_t size = 0;
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

int32_t FastLog::OnReloadUTCTime()
{
    return 0;
}
int32_t FastLog::OnReloadLogTank()
{
    return 0;
}
int32_t FastLog::OnReloadLogLevel()
{
    return 0;
}
int32_t FastLog::OnReloadLogFile()
{
    return 0;
}

bool FastLog::GenerateHeader(bool error, const char *tag, int32_t context_id, const char *level_name, int32_t *header_size)
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

    int32_t log_header_size = -1;

    if (error)
    {
        if (tag)
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int32_t)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id, errno);
        }
        else
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d][%d]",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int32_t)(tv.tv_usec / 1000),
                                       level_name, getpid(), context_id, errno);
        }
    }
    else
    {
        if (tag)
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s][%d][%d] ",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int32_t)(tv.tv_usec / 1000),
                                       level_name, tag, getpid(), context_id);
        }
        else
        {
            log_header_size = snprintf(log_data_, LOG_MAX_SIZE,
                                       "[%d-%02d-%02d %02d:%02d:%02d.%03d][%s][%d][%d] ",
                                       1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int32_t)(tv.tv_usec / 1000),
                                       level_name, getpid(), context_id);
        }
    }
    if (log_header_size == -1)
        return false;
    *header_size = rs_min(log_header_size, LOG_MAX_SIZE - 1);
    return true;
}
void FastLog::WriteLog(int32_t &fd_, char *str_log, int32_t size, int32_t level)
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

int32_t ThreadContext::GenerateID()
{
    static int32_t id = 100;

    int32_t cid = id++;
    cache_[st_thread_self()] = cid;
    return cid;
}
int32_t ThreadContext::GetID()
{
    st_thread_t st = st_thread_self();
    if (st == nullptr)
        return -1;
    return cache_[st];
}
int32_t ThreadContext::SetID(int32_t v)
{
    st_thread_t self = st_thread_self();

    int32_t old_v = 0;
    if (cache_.find(self) != cache_.end())
        old_v = cache_[self];
    return old_v;
}

void ThreadContext::ClearID()
{
    std::map<st_thread_t, int32_t>::iterator it = cache_.find(st_thread_self());
    if (it != cache_.end())
        cache_.erase(it);
}