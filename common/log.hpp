#ifndef RS_LOG_HPP
#define RS_LOG_HPP

#include <common/core.hpp>
#include <common/reload.hpp>

#include <map>

#include <st.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

struct LogLevel
{
    static const int32_t VERBOSE = 0x01;
    static const int32_t INFO = 0x02;
    static const int32_t TRACE = 0x03;
    static const int32_t WARN = 0x04;
    static const int32_t ERROR = 0x05;
    static const int32_t DISABLE = 0x06;
};

class ILog
{
public:
    ILog();
    virtual ~ILog();

public:
    int32_t Initialize();

public:
    virtual void Verbose(const char *tag, int32_t context_id, const char *fmt, ...);
    virtual void Info(const char *tag, int32_t context_id, const char *fmt, ...);
    virtual void Trace(const char *tag, int32_t context_id, const char *fmt, ...);
    virtual void Warn(const char *tag, int32_t context_id, const char *fmt, ...);
    virtual void Error(const char *tag, int32_t context_id, const char *fmt, ...);
};

class IThreadContext
{
public:
    IThreadContext();
    virtual ~IThreadContext();

public:
    virtual int32_t GenerateID();
    virtual int32_t GetID();
    virtual int32_t SetID(int32_t v);
    virtual void ClearID();
};

extern ILog *_log;
extern IThreadContext *_context;

#if 0
#define rs_verbose(msg, ...) _log->Verbose(NULL, _context->GetID(), msg, ##__VA_ARGS__)
#define rs_info(msg, ...) _log->Info(NULL, _context->GetID(), msg, ##__VA_ARGS__)
#define rs_trace(msg, ...) _log->Trace(NULL, _context->GetID(), msg, ##__VA_ARGS__)
#define rs_warn(msg, ...) _log->Warn(NULL, _context->GetID(), msg, ##__VA_ARGS__)
#define rs_error(msg, ...) _log->Error(NULL, _context->GetID(), msg, ##__VA_ARGS__)
#elif 1
#define rs_verbose(msg, ...)                                                \
    do                                                                      \
    {                                                                       \
        char tag[1024] = {0};                                               \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __FUNCTION__); \
        _log->Verbose(tag, _context->GetID(), msg, ##__VA_ARGS__);          \
    } while (0)

#define rs_info(msg, ...)                                                   \
    do                                                                      \
    {                                                                       \
        char tag[1024] = {0};                                               \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __FUNCTION__); \
        _log->Info(tag, _context->GetID(), msg, ##__VA_ARGS__);             \
    } while (0)

#define rs_trace(msg, ...)                                                  \
    do                                                                      \
    {                                                                       \
        char tag[1024] = {0};                                               \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __FUNCTION__); \
        _log->Trace(tag, _context->GetID(), msg, ##__VA_ARGS__);            \
    } while (0)

#define rs_warn(msg, ...)                                                   \
    do                                                                      \
    {                                                                       \
        char tag[1024] = {0};                                               \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __FUNCTION__); \
        _log->Warn(tag, _context->GetID(), msg, ##__VA_ARGS__);             \
    } while (0)
#define rs_error(msg, ...)                                                  \
    do                                                                      \
    {                                                                       \
        char tag[1024] = {0};                                               \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __FUNCTION__); \
        _log->Error(tag, _context->GetID(), msg, ##__VA_ARGS__);            \
    } while (0)
#else
#define rs_verbose(msg, ...)                                                       \
    do                                                                             \
    {                                                                              \
        char tag[1024] = {0};                                                      \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _log->Verbose(tag, _context->GetID(), msg, ##__VA_ARGS__);                 \
    } while (0)

#define rs_info(msg, ...)                                                          \
    do                                                                             \
    {                                                                              \
        char tag[1024] = {0};                                                      \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _log->Info(tag, _context->GetID(), msg, ##__VA_ARGS__);                    \
    } while (0)

#define rs_trace(msg, ...)                                                         \
    do                                                                             \
    {                                                                              \
        char tag[1024] = {0};                                                      \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _log->Trace(tag, _context->GetID(), msg, ##__VA_ARGS__);                   \
    } while (0)

#define rs_warn(msg, ...)                                                          \
    do                                                                             \
    {                                                                              \
        char tag[1024] = {0};                                                      \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _log->Warn(tag, _context->GetID(), msg, ##__VA_ARGS__);                    \
    } while (0)
#define rs_error(msg, ...)                                                         \
    do                                                                             \
    {                                                                              \
        char tag[1024] = {0};                                                      \
        snprintf(tag, 1024, "%s:%d<%s>", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        _log->Error(tag, _context->GetID(), msg, ##__VA_ARGS__);                   \
    } while (0)
#endif

class FastLog : public ILog, IReloadHandler
{
public:
    FastLog();
    virtual ~FastLog();

public:
    virtual void Verbose(const char *tag, int32_t context_id, const char *fmt, ...) override;
    virtual void Info(const char *tag, int32_t context_id, const char *fmt, ...) override;
    virtual void Trace(const char *tag, int32_t context_id, const char *fmt, ...) override;
    virtual void Warn(const char *tag, int32_t context_id, const char *fmt, ...) override;
    virtual void Error(const char *tag, int32_t context_id, const char *fmt, ...) override;

public:
    virtual int32_t OnReloadUTCTime() override;
    virtual int32_t OnReloadLogTank() override;
    virtual int32_t OnReloadLogLevel() override;
    virtual int32_t OnReloadLogFile() override;

protected:
    virtual bool GenerateHeader(bool error, const char *tag, int32_t context_id, const char *level_name, int32_t *header_size);
    virtual void WriteLog(int32_t &fd, char *str_log, int32_t size, int32_t level);
    virtual void OpenLogFile();

private:
    int32_t fd_;
    bool log_to_file_tank_;
    bool utc_;
    char *log_data_;
    int32_t level_;
};

class ThreadContext : public IThreadContext
{
public:
    ThreadContext();
    virtual ~ThreadContext();

public:
    virtual int32_t GenerateID() override;
    virtual int32_t GetID() override;
    virtual int32_t SetID(int32_t v) override;
    virtual void ClearID() override;

private:
    std::map<st_thread_t, int32_t> cache_;
};

#endif