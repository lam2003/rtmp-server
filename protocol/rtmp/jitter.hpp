#ifndef RS_RTMP_JITTER_HPP
#define RS_RTMP_JITTER_HPP

#include <common/core.hpp>
#include <protocol/rtmp/message.hpp>

namespace rtmp
{

enum class JitterAlgorithm
{
    FULL = 1,
    ZERO,
    OFF
};

class Jitter
{
public:
    Jitter();
    virtual ~Jitter();

public:
    virtual int Correct(rtmp::SharedPtrMessage *msg, rtmp::JitterAlgorithm ag);
    virtual int GetTime();

private:
    int64_t last_pkt_time_;
    int64_t last_pkt_correct_time_;
};

} // namespace rtmp

#endif