#include <protocol/rtmp_source.hpp>
#include <protocol/rtmp_consts.hpp>
#include <protocol/av.hpp>
#include <common/config.hpp>

#include <common/file.hpp>

#define MIX_CORRECT_PURE_AV 10

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

        if (msg->IsAudio() && av::Codec::IsAudioSeqenceHeader(msg->payload, msg->size))
        {
            rs_freep(audio_sh);
            audio_sh = msg;
            continue;
        }
        if (msg->IsVideo() && av::Codec::IsVideoSeqenceHeader(msg->payload, msg->size))
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

MixQueue::MixQueue()
{
    nb_videos_ = 0;
    nb_audios_ = 0;
}

MixQueue::~MixQueue()
{
    Clear();
}

void MixQueue::Clear()
{
    std::multimap<int64_t, SharedPtrMessage *>::iterator it;
    for (it = msgs_.begin(); it != msgs_.end(); it++)
    {
        SharedPtrMessage *msg = it->second;
        rs_freep(msg);
    }

    msgs_.clear();
    nb_videos_ = 0;
    nb_audios_ = 0;
}

void MixQueue::Push(SharedPtrMessage *msg)
{
    if (msg->IsVideo())
    {
        nb_videos_++;
    }
    else
    {
        nb_audios_++;
    }

    msgs_.insert(std::make_pair(msg->timestamp, msg));
}

SharedPtrMessage *MixQueue::Pop()
{
    bool mix_ok = false;

    if (nb_videos_ >= MIX_CORRECT_PURE_AV && nb_audios_ == 0)
    {
        mix_ok = true;
    }
    if (nb_audios_ > MIX_CORRECT_PURE_AV && nb_videos_ == 0)
    {
        mix_ok = true;
    }
    if (nb_videos_ > 0 && nb_audios_ > 0)
    {
        mix_ok = true;
    }

    if (!mix_ok)
    {
        return nullptr;
    }

    std::multimap<int64_t, SharedPtrMessage *>::iterator it = msgs_.begin();
    SharedPtrMessage *msg = it->second;
    msgs_.erase(it);

    if (msg->IsVideo())
    {
        nb_videos_--;
    }
    else
    {
        nb_audios_--;
    }

    return msg;
}

std::map<std::string, Source *> Source::pool_;

Source::Source()
{
    request_ = nullptr;
    atc_ = false;
    handler_ = nullptr;
    can_publish_ = true;
    mix_correct_ = true;
    is_monotonically_increase_ = true;
    last_packet_time_ = 0;
    cache_metadata_ = nullptr;
    cache_sh_video_ = nullptr;
    cache_sh_audio_ = nullptr;
    mix_queue_ = new MixQueue;
}

Source::~Source()
{
    rs_freep(mix_queue_);
    rs_freep(cache_sh_audio_);
    rs_freep(cache_sh_video_);
    rs_freep(cache_metadata_);

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

int Source::on_video_impl(SharedPtrMessage *msg)
{
    int ret = ERROR_SUCCESS;

    bool is_sequence_hander = av::Codec::IsVideoSeqenceHeader(msg->payload, msg->size);

    bool drop_for_reduce = false;
    if (is_sequence_hander && cache_sh_video_ && _config->GetReduceSequenceHeader(request_->host))
    {
        if (cache_sh_video_->size == msg->size)
        {
            drop_for_reduce = Utils::BytesEquals(cache_sh_video_->payload, msg->payload, msg->size);
            rs_warn("drop for reduce sh video, size=%d", msg->size);
        }
    }

    // if (is_sequence_hander)
    // {
    //     rs_freep(cache_sh_video_);
    // }
    return ret;
}

int Source::on_audio_impl(SharedPtrMessage *msg)
{
    int ret = ERROR_SUCCESS;

    bool is_aac_sequence_header = av::Codec::IsAudioSeqenceHeader(msg->payload, msg->size);
    bool is_sequence_header = is_aac_sequence_header;

    bool drop_for_reduce = false;
    if (is_sequence_header && cache_sh_audio_ && _config->GetReduceSequenceHeader(request_->vhost))
    {
        if (cache_sh_audio_->size == msg->size)
        {
            drop_for_reduce = Utils::BytesEquals(cache_sh_audio_->payload, msg->payload, msg->size);
            rs_warn("drop for reduce sh audio, size=%d", msg->size);
        }
    }

    if (is_aac_sequence_header)
    {
        av::Codec codec;
        av::CodecSample sample;

        if ((ret = codec.DemuxAudio(msg->payload, msg->size, &sample)) != ERROR_SUCCESS)
        {
            rs_error("source codec demux audio failed. ret=%d", ret);
            return ret;
        }

        static int sample_sizes[] = {8, 16, 0};
        static int sound_types[] = {1, 2, 0};
        static int flv_sample_rates[] = {5512, 11025, 22050, 44100, 0};

        rs_trace("%dB audio sh, codec(%d, profile=%s, %dHz, %dbits, %dchannels) flv(%dHz)", msg->size,
                 codec.audio_codec_id,
                 av::AACProfile2Str(codec.aac_object_type).c_str(),
                 sample.aac_sample_rate,
                 sample_sizes[(int)sample.sound_size],
                 sound_types[(int)sample.sound_type],
                 flv_sample_rates[(int)sample.flv_sample_rate]);
    }

    if (!drop_for_reduce)
    {
        for (int i = 0; i < (int)consumers_.size(); i++)
        {
            Consumer *consumer = consumers_.at(i);
            if ((ret = consumer->Enqueue(msg, atc_, jitter_algorithm_)) != ERROR_SUCCESS)
            {
                rs_error("dispatch audio failed. ret=%d", ret);
                return ret;
            }
        }
    }
    return ret;
}

int Source::OnAudio(CommonMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (!mix_correct_ && is_monotonically_increase_)
    {
        if (last_packet_time_ > 0 && msg->header.timestamp < last_packet_time_)
        {
            is_monotonically_increase_ = false;
            rs_warn("AUDIO: stream not monotonically increase, please open mix_correct.");
        }
    }

    last_packet_time_ = msg->header.timestamp;

    SharedPtrMessage shared_msg;
    if ((shared_msg.Create(msg)) != ERROR_SUCCESS)
    {
        rs_error("initialize the audio failed. ret=%d", ret);
        return ret;
    }

    if (!mix_correct_)
    {
        return on_audio_impl(&shared_msg);
    }

    mix_queue_->Push(shared_msg.Copy());

    SharedPtrMessage *m = mix_queue_->Pop();
    if (!m)
    {
        return ret;
    }

    if (m->IsAudio())
    {
        on_audio_impl(m);
    }
    else
    {
        on_video_impl(m);
    }

    rs_freep(m);

    return ret;
}
} // namespace rtmp