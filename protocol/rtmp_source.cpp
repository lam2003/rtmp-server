#include <protocol/rtmp_source.hpp>
#include <protocol/rtmp_consts.hpp>
#include <protocol/flv.hpp>
#include <common/config.hpp>

namespace rtmp
{
ISourceHandler::ISourceHandler()
{
}

ISourceHandler::~ISourceHandler()
{
}

IWakeable::IWakeable()
{
}

IWakeable::~IWakeable()
{
}

FastVector::FastVector()
{
    count_ = 0;
    nb_msgs_ = RTMP_PERF_MW_MSGS * 8;
    msgs_ = new SharedPtrMessage *[nb_msgs_];
}

FastVector::~FastVector()
{
    Free();
    rs_freepa(msgs_);
}

int FastVector::Size()
{
    return count_;
}

int FastVector::Begin()
{
    return 0;
}

int FastVector::End()
{
    return count_;
}

SharedPtrMessage **FastVector::Data()
{
    return msgs_;
}

SharedPtrMessage *FastVector::At(int index)
{
    return msgs_[index];
}

void FastVector::Free()
{
    for (int i = 0; i < count_; i++)
    {
        SharedPtrMessage *msg = msgs_[i];
        rs_freep(msg);
    }
    count_ = 0;
}

void FastVector::Clear()
{
    count_ = 0;
}

void FastVector::Erase(int begin, int end)
{
    for (int i = begin; i < end; i++)
    {
        SharedPtrMessage *msg = msgs_[i];
        rs_freep(msg);
    }

    for (int i = 0; i < count_ - end; i++)
    {
        msgs_[begin + i] = msgs_[end + i];
    }

    count_ -= (end - begin);
}

void FastVector::PushBack(SharedPtrMessage *msg)
{
    if (count_ >= nb_msgs_)
    {
        int size = nb_msgs_ * 2;

        SharedPtrMessage **buf = new SharedPtrMessage *[size];
        for (int i = 0; i < nb_msgs_; i++)
        {
            buf[i] = msgs_[i];
        }

        rs_warn("fast vector incrase %d=>%d", nb_msgs_, size);
        rs_freepa(msgs_);
        msgs_ = buf;
        nb_msgs_ = size;
    }
    msgs_[count_++] = msg;
}

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

    if (delta < RTMP_MAX_JITTER_MS_NEG || delta > RTMP_MAX_JITTER_MS)
    {
        delta = RTMP_DEFAULT_FRAME_TIME_MS;
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

Consumer::Consumer(Source *s, Connection *c)
{
    source_ = s;
    conn_ = c;
    pause_ = false;
    jitter_ = new Jitter;
    queue_ = new MessageQueue;
    mw_wait_ = st_cond_new();
    mw_waiting_ = false;
    mw_min_msgs_ = 0;
    mw_duration_ = 0;
}

Consumer::~Consumer()
{
    source_->OnConsumerDestroy(this);
    rs_freep(jitter_);
    rs_freep(queue_);
    st_cond_destroy(mw_wait_);
}

void Consumer::SetQueueSize(double second)
{
    queue_->SetQueueSize(second);
}

int Consumer::GetTime()
{
    return jitter_->GetTime();
}

int Consumer::Enqueue(SharedPtrMessage *shared_msg, bool atc, JitterAlgorithm ag)
{
    int ret = ERROR_SUCCESS;

    SharedPtrMessage *msg = shared_msg->Copy();

    if (!atc)
    {
        if ((ret = jitter_->Correct(msg, ag)) != ERROR_SUCCESS)
        {
            rs_freep(msg);
            return ret;
        }
    }

    if ((ret = queue_->Enqueue(msg, nullptr)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (mw_waiting_)
    {
        int duration_ms = queue_->Duration();
        bool match_min_msgs = queue_->Size() > mw_min_msgs_;

        //for atc,maybe the sequeue header timestamp bigger than A/V packet
        //when encode republish or overflow
        if (atc && duration_ms < 0)
        {
            st_cond_signal(mw_wait_);
            mw_waiting_ = false;
            return ret;
        }

        if (match_min_msgs && duration_ms > mw_duration_)
        {
            st_cond_signal(mw_wait_);
            mw_waiting_ = false;
            return ret;
        }
    }

    return ret;
}

int Consumer::DumpPackets(MessageArray *msg_arr, int &count)
{
    int ret = ERROR_SUCCESS;

    int max = count ? rs_min(count, msg_arr->max) : msg_arr->max;

    count = 0;

    if (pause_)
    {
        return ret;
    }

    if ((ret = queue_->DumpPackets(max, msg_arr->msgs, count)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

void Consumer::Wait(int nb_msgs, int duration)
{
    if (pause_)
    {
        st_usleep(RTMP_PULSE_TIMEOUT_US);
        return;
    }

    mw_min_msgs_ = nb_msgs;
    mw_duration_ = duration;

    int duration_ms = queue_->Duration();
    bool match_min_msgs = queue_->Size() > mw_min_msgs_;

    if (match_min_msgs && duration_ms > mw_min_msgs_)
    {
        return;
    }

    mw_waiting_ = true;

    st_cond_wait(mw_wait_);
}

void Consumer::WakeUp()
{
    if (mw_waiting_)
    {
        st_cond_signal(mw_wait_);
        mw_waiting_ = false;
    }
}

int Consumer::OnPlayClientPause(bool is_pause)
{
    int ret = ERROR_SUCCESS;

    pause_ = is_pause;

    return ret;
}

MessageQueue::MessageQueue()
{
    av_start_time_ = -1;
    av_end_time_ = -1;
    queue_size_ms_ = 0;
}

MessageQueue::~MessageQueue()
{
    Clear();
}

int MessageQueue::Size()
{
    return msgs_.Size();
}

int MessageQueue::Duration()
{
    return (int)(av_end_time_ - av_start_time_);
}

void MessageQueue::SetQueueSize(double second)
{
    queue_size_ms_ = (int)(second * 1000);
}

void MessageQueue::Shrink()
{
    SharedPtrMessage *video_sh = nullptr;
    SharedPtrMessage *audio_sh = nullptr;

    int msgs_size = msgs_.Size();

    for (int i = 0; i < msgs_size; i++)
    {
        SharedPtrMessage *msg = msgs_.At(i);

        if (msg->IsAudio() && flv::Codec::IsAudioSeqenceHeader(msg->payload, msg->size))
        {
            rs_freep(audio_sh);
            audio_sh = msg;
            continue;
        }
        if (msg->IsVideo() && flv::Codec::IsVideoSeqenceHeader(msg->payload, msg->size))
        {
            rs_freep(video_sh);
            video_sh = msg;
            continue;
        }

        rs_freep(msg);
    }
    msgs_.Clear();

    av_start_time_ = av_end_time_;

    if (audio_sh)
    {
        audio_sh->timestamp = av_end_time_;
        msgs_.PushBack(audio_sh);
    }
    if (video_sh)
    {
        video_sh->timestamp = av_end_time_;
        msgs_.PushBack(video_sh);
    }
}

int MessageQueue::Enqueue(SharedPtrMessage *msg, bool *is_overflow)
{
    int ret = ERROR_SUCCESS;

    if (msg->IsAV())
    {
        if (av_start_time_ == -1)
        {
            av_start_time_ = msg->timestamp;
        }
        av_end_time_ = msg->timestamp;
    }

    msgs_.PushBack(msg);

    while (av_end_time_ - av_start_time_ > queue_size_ms_)
    {
        if (is_overflow)
        {
            *is_overflow = true;
        }
        Shrink();
    }

    return ret;
}

void MessageQueue::Clear()
{
    msgs_.Free();
    av_start_time_ = av_end_time_ = -1;
}

int MessageQueue::DumpPackets(int max_count, SharedPtrMessage **pmsgs, int &count)
{
    int ret = ERROR_SUCCESS;

    int nb_msgs = msgs_.Size();
    if (nb_msgs <= 0)
    {
        return ret;
    }

    count = rs_min(nb_msgs, max_count);

    SharedPtrMessage **omsgs = msgs_.Data();
    for (int i = 0; i < count; i++)
    {
        pmsgs[i] = omsgs[i];
    }

    SharedPtrMessage *last = omsgs[count - 1];
    av_start_time_ = last->timestamp;

    if (count >= nb_msgs)
    {
        msgs_.Clear();
    }
    else
    {
        msgs_.Erase(msgs_.Begin(), msgs_.Begin() + count);
    }

    return ret;
}

int MessageQueue::DumpPackets(Consumer *consumer, bool atc, JitterAlgorithm ag)
{
    int ret = ERROR_SUCCESS;

    return ret;
}

Source::Source() : request_(nullptr), can_publish_(true)
{
}

std::map<std::string, Source *> Source::pool_;

Source::~Source()
{
    rs_freep(request_);
}

int Source::FetchOrCreate(rtmp::Request *r, ISourceHandler *h, Source **pps)
{
    int ret = ERROR_SUCCESS;

    Source *source = nullptr;

    if ((source = Fetch(r)) != nullptr)
    {
        *pps = source;
        return ret;
    }

    std::string stream_url = r->GetStreamUrl();

    std::string vhost = r->vhost;

    source = new Source;

    if ((ret = source->Initialize(r, h)) != ERROR_SUCCESS)
    {
        rs_freep(source);
        return ret;
    }

    pool_[stream_url] = source;
    *pps = source;

    rs_info("create new source for url=%s,vhost=%s", stream_url.c_str(), vhost.c_str());

    return ret;
}

Source *Source::Fetch(rtmp::Request *r)
{
    Source *source = nullptr;

    std::string stream_url = r->GetStreamUrl();

    if (pool_.find(stream_url) == pool_.end())
    {
        return nullptr;
    }

    source = pool_[stream_url];

    source->request_->Update(r);

    return source;
}

int Source::Initialize(rtmp::Request *r, ISourceHandler *h)
{
    int ret = ERROR_SUCCESS;

    handler_ = h;

    request_ = r->Copy();

    atc_ = _config->GetATC(r->vhost);

    return ret;
}

bool Source::CanPublish(bool is_edge)
{
    return can_publish_;
}

void Source::OnConsumerDestroy(Consumer *consumer)
{
}
} // namespace rtmp