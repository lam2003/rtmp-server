/*
 * @Date: 2020-03-18 11:17:48
 * @LastEditors: linmin
 * @LastEditTime: 2020-03-25 12:32:09
 */
#include <common/log.hpp>
#include <muxer/flv.hpp>
#include <protocol/rtmp/consumer.hpp>
#include <protocol/rtmp/gop_cache.hpp>
#include <protocol/rtmp/jitter.hpp>
#include <protocol/rtmp/message.hpp>

#define PURE_AUDIO_GUESS_THRESHOLD 115

namespace rtmp {

GopCache::GopCache()
{
    cached_video_count_           = 0;


    //TODO just for a test
    enable_gop_cache_             = true;


    audio_after_last_video_count_ = 0;
}

GopCache::~GopCache()
{
    Clear();
}

void GopCache::Set(bool enabled)
{
    enable_gop_cache_ = enabled;

    if (!enable_gop_cache_) {
        rs_info("gop cache disabled, clear %d packets", (int)queue_.size());
        Clear();
        return;
    }
}

void GopCache::Dispose()
{
    Clear();
}

int GopCache::Cache(SharedPtrMessage* shared_msg)
{
    int ret = ERROR_SUCCESS;

    if (!enable_gop_cache_) {
        return ret;
    }

    SharedPtrMessage* msg = shared_msg;
    if (msg->IsVideo()) {
        if (!flv::Demuxer::IsAVC(msg->payload, msg->size)) {
            rs_info("gop cache drop video for none avc");
            return ret;
        }

        if (flv::Demuxer::IsKeyFrame(msg->payload, msg->size)) {
            rs_info("clear gop cache when got keyframe. vcount=%d, count=%d",
                    cached_video_count_, (int)queue_.size());
            Clear();
            cached_video_count_ = 1;
        }
        else {
            cached_video_count_++;
        }

        audio_after_last_video_count_ = 0;
    }
    else if (msg->IsAudio()) {
        audio_after_last_video_count_++;
    }

    if (audio_after_last_video_count_ > PURE_AUDIO_GUESS_THRESHOLD) {
        rs_warn("clear gop cache for guess pure audio overflow");
        Clear();
        return ret;
    }

    queue_.push_back(msg->Copy());

    return ret;
}

int GopCache::Dump(Consumer* consumer, bool atc, JitterAlgorithm ag)
{
    int ret = ERROR_SUCCESS;

    std::vector<SharedPtrMessage*>::iterator it;
    for (it = queue_.begin(); it != queue_.end(); it++) {
        if ((ret = consumer->Enqueue(*it, atc, ag)) != ERROR_SUCCESS) {
            rs_error("dispatch cached gop failed. ret=%d", ret);
            return ret;
        }
    }

    rs_trace("dispatch cached gop success. count=%d, duration=%d",
             (int)queue_.size(), consumer->GetTime());

    return ret;
}

void GopCache::Clear()
{
    std::vector<SharedPtrMessage*>::iterator it;

    for (it = queue_.begin(); it != queue_.end(); it++) {
        rs_freep(*it);
    }

    queue_.clear();
    cached_video_count_           = 0;
    audio_after_last_video_count_ = 0;
}

bool GopCache::PureAudio()
{
    return cached_video_count_ == 0;
}

int64_t GopCache::StartTime()
{
    if (Empty()) {
        return 0;
    }

    return queue_[0]->timestamp;
}

bool GopCache::Empty()
{
    return queue_.empty();
}
}  // namespace rtmp
