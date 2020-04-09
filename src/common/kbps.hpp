#ifndef RS_KBPS_H
#define RS_KBPS_H

#include <common/core.hpp>
#include <common/io.hpp>

class IKbpsDelta
{
public:
    IKbpsDelta();
    virtual ~IKbpsDelta();

public:
    virtual void Resample() = 0;
    virtual int64_t GetSendBytesDelta() = 0;
    virtual int64_t GetRecvBytesDelta() = 0;
    virtual void CleanUp() = 0;
};

struct KbpsSample
{
    int64_t bytes;
    int64_t time;
    int kbps;
    KbpsSample();
};

class KbpsSlice
{
public:
    union slice_io {
        IStatistic *in;
        IStatistic *out;
    };
    KbpsSlice();
    virtual ~KbpsSlice();

public:
    virtual int64_t GetTotalBytes();
    virtual void Sample();

public:
    int64_t delta_bytes;
    int64_t bytes;
    int64_t start_time;
    int64_t end_time;
    int64_t io_bytes_base;
    int64_t last_bytes;
    slice_io io;
    KbpsSample sample_30s;
    KbpsSample sample_1m;
    KbpsSample sample_5m;
    KbpsSample sample_60m;
};

class Kbps : public virtual IStatistic,
             public virtual IKbpsDelta
{
public:
    Kbps();
    virtual ~Kbps();

public:
    virtual void AddDelta(IKbpsDelta *delta);
    virtual void Sample();
    virtual void SetIO(IStatistic *in, IStatistic *out);
    //IStatistic
    virtual int64_t GetRecvBytes() override;
    virtual int64_t GetSendBytes() override;
    //IKbpsDelta
    virtual void Resample() override;
    virtual int64_t GetSendBytesDelta() override;
    virtual int64_t GetRecvBytesDelta() override;
    virtual void CleanUp() override;

private:
    KbpsSlice is_;
    KbpsSlice os_;
};

#endif