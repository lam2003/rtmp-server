#ifndef RS_LOG_HPP
#define RS_LOG_HPP

#include <common/core.hpp>

#include <stdio.h>
#include <errno.h>
#include <string.h>

struct LogLevel
{
    static const int VERBOSE = 0x01;
    static const int INFO = 0x02;
    static const int TRACE = 0x03;
    static const int WARN = 0x04;
    static const int ERROR = 0x05;
    static const int DISABLE = 0x06;
};

class ILog
{
public:
    ILog();
    virtual ~ILog();

public:
    int Initialize();

public:
    virtual void Verbose(const char *tag, int context_id, const char *fmt, ...);
    virtual void Info(const char *tag, int context_id, const char *fmt, ...);
    virtual void Trace(const char *tag, int context_id, const char *fmt, ...);
    virtual void Warn(const char *tag, int context_id, const char *fmt, ...);
    virtual void Error(const char *tag, int context_id, const char *fmt, ...);
};

class IThreadContext
{
public:
    IThreadContext();
    virtual ~IThreadContext();

public:
    virtual int GenerateID();
    virtual int GetID();
    virtual int SetID(int v);
    virtual void ClearID();
};

extern ILog *_log;
extern IThreadContext *_context;

#if 1
#define rs_verbose(msg, ...) _log->verbose(NULL, _context->get_id(), msg, ##__VA_ARGS__)
#define rs_info(msg, ...) _log->info(NULL, _context->get_id(), msg, ##__VA_ARGS__)
#define rs_trace(msg, ...) _log->trace(NULL, _context->get_id(), msg, ##__VA_ARGS__)
#define rs_warn(msg, ...) _log->warn(NULL, _context->get_id(), msg, ##__VA_ARGS__)
#define rs_error(msg, ...) _log->error(NULL, _context->get_id(), msg, ##__VA_ARGS__)
#elif 0
#define srs_verbose(msg, ...) _log->verbose(__FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_info(msg, ...) _log->info(__FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_trace(msg, ...) _log->trace(__FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_warn(msg, ...) _log->warn(__FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_error(msg, ...) _log->error(__FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#else
#define srs_verbose(msg, ...) _log->verbose(__PRETTY_FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_info(msg, ...) _log->info(__PRETTY_FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_trace(msg, ...) _log->trace(__PRETTY_FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_warn(msg, ...) _log->warn(__PRETTY_FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#define srs_error(msg, ...) _log->error(__PRETTY_FUNCTION__, _context->get_id(), msg, ##__VA_ARGS__)
#endif

#endif