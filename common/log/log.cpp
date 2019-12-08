#include <common/log/log.hpp>

#include <common/error.hpp>

ILog::ILog()
{
}

ILog::~ILog()
{
}

void ILog::Verbose(const char *, int, const char *, ...)
{
}

void ILog::Info(const char *, int, const char *, ...)
{
}

void ILog::Trace(const char *, int, const char *, ...)
{
}

void ILog::Warn(const char *, int, const char *, ...)
{
}

void ILog::Error(const char *, int, const char *, ...)
{
}

IThreadContext::IThreadContext()
{
}

IThreadContext::~IThreadContext()
{
}

int IThreadContext::GenerateID()
{
    return 0;
}

int IThreadContext::GetID()
{
    return 0;
}

int IThreadContext::SetID(int)
{
    return 0;
}