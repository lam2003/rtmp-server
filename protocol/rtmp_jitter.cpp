#include <protocol/rtmp_jitter.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

#define MAX_JITTER_MS 250
#define MAX_JITTER_MS_NEG -250
#define DEFAULT_FRAME_TIME_MS 10

namespace rtmp
{

Jitter::Jitter()
{
    last_pkt_time_ = 0;
    last_pkt_correct_time_ = -1;
}

Jitter::~Jitter()
{
}

int Jitter::Correct(SharedPtrMessage *msg, JitterAlgorithm ag)
{
    int ret = ERROR_SUCCESS;
    if (ag != JitterAlgorithm::FULL)
    {
        if (ag == JitterAlgorithm::OFF)
        {
            return ret;
        }
        if (ag == JitterAlgorithm::ZERO)
        {
            //ensure timestamp start at zero
            if (last_pkt_correct_time_ == -1)
            {
                last_pkt_correct_time_ = msg->timestamp;
            }
            msg->timestamp -= last_pkt_correct_time_;
            return ret;
        }

        return ret;
    }

    if (!msg->IsAV())
    {
        msg->timestamp = 0;
        return ret;
    }

    int64_t time = msg->timestamp;
    int64_t delta = time - last_pkt_time_;

    if (delta < MAX_JITTER_MS_NEG || delta > MAX_JITTER_MS)
    {
        delta = DEFAULT_FRAME_TIME_MS;
    }

    last_pkt_correct_time_ = rs_max(0, last_pkt_correct_time_ + delta);

    msg->timestamp = last_pkt_correct_time_;
    last_pkt_time_ = time;

    return ret;
}

int Jitter::GetTime()
{
    return last_pkt_correct_time_;
}

} // namespace rtmp