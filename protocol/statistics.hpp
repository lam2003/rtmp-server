#ifndef RS_STATISTIC_H
#define RS_STATISTIC_H

#include <common/core.hpp>

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

#endif