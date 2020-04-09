#include <common/kbps.hpp>
#include <common/utils.hpp>

IKbpsDelta::IKbpsDelta()
{
}

IKbpsDelta::~IKbpsDelta()
{
}

KbpsSample::KbpsSample() : bytes(0), time(0), kbps(0)
{
}

KbpsSlice::KbpsSlice() : delta_bytes(0),
                         bytes(0),
                         start_time(0),
                         end_time(0),
                         io_bytes_base(0),
                         last_bytes(0)
{
    io.in = nullptr;
    io.out = nullptr;
}

KbpsSlice::~KbpsSlice()
{
}

int64_t KbpsSlice::GetTotalBytes()
{
    return bytes + last_bytes - io_bytes_base;
}

void KbpsSlice::Sample()
{
    int64_t now = Utils::GetSteadyMilliSeconds();
    int64_t total_bytes = GetTotalBytes();

    if (sample_30s.time <= 0)
    {
        sample_30s.kbps = 0;
        sample_30s.time = now;
        sample_30s.bytes = total_bytes;
    }
    if (sample_1m.time <= 0)
    {
        sample_1m.kbps = 0;
        sample_1m.time = now;
        sample_1m.bytes = total_bytes;
    }
    if (sample_5m.time <= 0)
    {
        sample_5m.kbps = 0;
        sample_5m.time = now;
        sample_5m.bytes = total_bytes;
    }
    if (sample_60m.time <= 0)
    {
        sample_60m.kbps = 0;
        sample_60m.time = now;
        sample_60m.bytes = total_bytes;
    }

    if (now - sample_30s.time > 30 * 1000)
    {
        sample_30s.kbps = (int)((total_bytes - sample_30s.bytes) * 8 / (now - sample_30s.time));
        sample_30s.time = now;
        sample_30s.bytes = total_bytes;
    }
    if (now - sample_1m.time > 60 * 1000)
    {
        sample_1m.kbps = (int)((total_bytes - sample_1m.bytes) * 8 / (now - sample_1m.time));
        sample_1m.time = now;
        sample_1m.bytes = total_bytes;
    }
    if (now - sample_5m.time > 300 * 1000)
    {
        sample_5m.kbps = (int)((total_bytes - sample_5m.bytes) * 8 / (now - sample_5m.time));
        sample_5m.time = now;
        sample_5m.bytes = total_bytes;
    }
    if (now - sample_60m.time > 3600 * 1000)
    {
        sample_60m.kbps = (int)((total_bytes - sample_60m.bytes) * 8 / (now - sample_60m.time));
        sample_60m.time = now;
        sample_60m.bytes = total_bytes;
    }
}

Kbps::Kbps()
{
}

Kbps::~Kbps()
{
}

int64_t Kbps::GetRecvBytes()
{
    int64_t bytes = is_.bytes;

    if (is_.io.in)
    {
        bytes += is_.io.in->GetRecvBytes() - is_.io_bytes_base;
        return bytes;
    }

    bytes += is_.last_bytes - is_.io_bytes_base;

    return bytes;
}

int64_t Kbps::GetSendBytes()
{
    int64_t bytes = os_.bytes;

    if (os_.io.out)
    {
        bytes += os_.io.out->GetSendBytes() - os_.io_bytes_base;
        return bytes;
    }

    bytes += os_.last_bytes - os_.io_bytes_base;

    return bytes;
}

void Kbps::Resample()
{
    Sample();
}

int64_t Kbps::GetSendBytesDelta()
{
    int delta = os_.GetTotalBytes() - os_.delta_bytes;
    return delta;
}

int64_t Kbps::GetRecvBytesDelta()
{
    int delta = is_.GetTotalBytes() - is_.delta_bytes;
    return delta;
}

void Kbps::CleanUp()
{
    is_.delta_bytes = is_.GetTotalBytes();
    os_.delta_bytes = os_.GetTotalBytes();
}

void Kbps::SetIO(IStatistic *in, IStatistic *out)
{
    if (is_.start_time == 0)
    {
        is_.start_time = Utils::GetSteadyMilliSeconds();
    }

    if (is_.io.in)
    {
        is_.bytes += is_.io.in->GetRecvBytes() - is_.io_bytes_base;
    }

    is_.io.in = in;
    is_.last_bytes = is_.io_bytes_base = 0;

    if (in)
    {
        is_.last_bytes = is_.io_bytes_base = in->GetRecvBytes();
    }

    is_.Sample();

    if (os_.start_time == 0)
    {
        os_.start_time = Utils::GetSteadyMilliSeconds();
    }

    if (os_.io.out)
    {
        os_.bytes += os_.io.out->GetSendBytes() - os_.io_bytes_base;
    }

    os_.io.out = out;
    os_.last_bytes = os_.io_bytes_base = 0;

    if (out)
    {
        os_.last_bytes = os_.io_bytes_base = out->GetSendBytes();
    }

    os_.Sample();
}

void Kbps::Sample()
{
    if (is_.io.in)
    {
        is_.last_bytes = is_.io.in->GetRecvBytes();
    }

    if (os_.io.out)
    {
        os_.last_bytes = os_.io.out->GetSendBytes();
    }

    is_.Sample();
    os_.Sample();
}

void Kbps::AddDelta(IKbpsDelta *delta)
{
    is_.last_bytes += delta->GetRecvBytesDelta();
    os_.last_bytes += delta->GetSendBytesDelta();
}
