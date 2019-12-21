#include <common/log.hpp>
#include <common/error.hpp>

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