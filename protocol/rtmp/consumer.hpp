#ifndef RS_RTMP_CONSUMER_HPP
#define RS_RTMP_CONSUMER_HPP

#include <common/core.hpp>

namespace rtmp
{

class Source;

class IWakeable
{
public:
    IWakeable();
    virtual ~IWakeable();

public:
    virtual void WakeUp() = 0;
};

class Consumer : public IWakeable
{
public:
    Consumer();
    virtual ~Consumer();

public:
    virtual void SetQueueSize(double queue_size);
    virtual void GetTime();
    virtual int Enqueue(SharedPtrMessage *shared_msg, bool atc, JitterAlgorithm ag);
    virtual int DumpPackets(MessageArray *msg_arr, int &count);
    virtual void Wait(int nb_msgs, int duration);
    virtual int OnPlayClientPause(bool is_pause);
    //IWakeable
    virtual void WakeUp() override;

private:
    Source *source_;
    Connection *conn_;
    bool pause_;
    Jitter *jitter_;
    MessageQueue *queue_;
    st_cond_t mw_wait_;
    bool mw_waiting_;
    int mw_min_msgs_;
    int mw_duration_;
};

} // namespace rtmp

#endif