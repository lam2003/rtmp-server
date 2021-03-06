#include <common/config.hpp>
#include <muxer/flv.hpp>
#include <protocol/amf/amf0.hpp>
#include <protocol/rtmp/connection.hpp>
#include <protocol/rtmp/consumer.hpp>
#include <protocol/rtmp/defines.hpp>
#include <protocol/rtmp/dvr.hpp>
#include <protocol/rtmp/gop_cache.hpp>
#include <protocol/rtmp/jitter.hpp>
#include <protocol/rtmp/message.hpp>
#include <protocol/rtmp/packet.hpp>
#include <protocol/rtmp/source.hpp>

#include <algorithm>
#include <sstream>

#define SOURCE_CLEAN_UP_MS 5000

namespace rtmp {

ISourceHandler::ISourceHandler() {}

ISourceHandler::~ISourceHandler() {}

std::map<std::string, Source*> Source::pool_;

Source::Source()
{
    request_                   = nullptr;
    atc_                       = false;
    handler_                   = nullptr;
    can_publish_               = true;
    mix_correct_               = true;
    is_monotonically_increase_ = true;
    last_packet_time_          = 0;
    cache_metadata_            = nullptr;
    cache_sh_video_            = nullptr;
    cache_sh_audio_            = nullptr;
    ag_                        = JitterAlgorithm::FULL;
    mix_queue_                 = new MixQueue<SharedPtrMessage>;
    dvr_                       = new Dvr;
    gop_cache_                 = new GopCache;
    die_at_                    = -1;
    source_id_                 = -1;
    prev_source_id_            = -1;
}

Source::~Source()
{
    rs_freep(gop_cache_);
    rs_freep(dvr_);
    rs_freep(mix_queue_);
    rs_freep(cache_sh_audio_);
    rs_freep(cache_sh_video_);
    rs_freep(cache_metadata_);
    rs_freep(request_);
}

int Source::FetchOrCreate(Request* r, ISourceHandler* h, Source** pps)
{
    int ret = ERROR_SUCCESS;

    Source* source = nullptr;

    if ((source = fetch(r)) != nullptr) {
        *pps = source;
        return ret;
    }

    std::string stream_url = r->GetStreamUrl();

    std::string vhost = r->vhost;

    source = new Source;

    if ((ret = source->Initialize(r, h)) != ERROR_SUCCESS) {
        rs_freep(source);
        return ret;
    }

    pool_[stream_url] = source;
    *pps              = source;

    rs_info("create new source for url=%s,vhost=%s", stream_url.c_str(),
            vhost.c_str());

    return ret;
}

Source* Source::fetch(Request* r)
{
    Source* source = nullptr;

    std::string stream_url = r->GetStreamUrl();

    if (pool_.find(stream_url) == pool_.end()) {
        return nullptr;
    }

    source = pool_[stream_url];

    source->request_->Update(r);

    return source;
}

int Source::Initialize(Request* r, ISourceHandler* h)
{
    int ret = ERROR_SUCCESS;

    handler_ = h;

    request_ = r->Copy();

    atc_ = _config->GetATC(r->vhost);

    if ((ret = dvr_->Initialize(this, request_)) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

bool Source::CanPublish(bool is_edge)
{
    return can_publish_;
}

void Source::OnConsumerDestroy(Consumer* consumer)
{
    std::vector<Consumer*>::iterator it =
        std::find(consumers_.begin(), consumers_.end(), consumer);

    if (it == consumers_.end()) {
        rs_warn("consumer has been removed. ignore it");
        return;
    }

    consumers_.erase(it);
    rs_info("consumer removed");
    if (consumers_.empty()) {
        die_at_ = Utils::GetSteadyMilliSeconds();
        rs_trace("there are not consumer subscribing %s now",
                 request_->GetStreamUrl().c_str());
    }
}

int Source::on_video_impl(SharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;

    bool is_sequence_header =
        flv::Demuxer::IsAVCSequenceHeader(msg->payload, msg->size);
    bool drop_for_reduce = false;

    if (is_sequence_header && cache_sh_video_ &&
        _config->GetReduceSequenceHeader(request_->host)) {
        if (cache_sh_video_->size == msg->size) {
            drop_for_reduce = Utils::BytesEquals(cache_sh_video_->payload,
                                                 msg->payload, msg->size);
            rs_warn("drop for reduce sh video, size=%d", msg->size);
        }
    }

    if (is_sequence_header) {
        rs_freep(cache_sh_video_);
        cache_sh_video_ = msg->Copy();

        flv::CodecSample sample;
        flv::Demuxer     demuxer;
        if ((ret = demuxer.DemuxVideo(msg->payload, msg->size, &sample)) !=
            ERROR_SUCCESS) {
            rs_error("source codec demux video failed. ret=%d", ret);
            return ret;
        }
    }

    if ((ret = dvr_->OnVideo(msg)) != ERROR_SUCCESS) {
        rs_warn(
            "dvr process video message failed, ignore and disable dvr. ret=%d",
            ret);
        dvr_->OnUnpubish();
        ret = ERROR_SUCCESS;
    }

    if (!drop_for_reduce) {
        for (int i = 0; i < (int)consumers_.size(); i++) {
            Consumer* consumer = consumers_.at(i);
            if ((ret = consumer->Enqueue(msg, atc_, ag_)) != ERROR_SUCCESS) {
                rs_error("dispatch video failed. ret=%d", ret);
                return ret;
            }
        }
    }

    if (is_sequence_header) {
        return ret;
    }

    gop_cache_->Cache(msg);

    if (atc_) {
        if (cache_sh_audio_) {
            cache_sh_audio_->timestamp = msg->timestamp;
        }

        if (cache_sh_video_) {
            cache_sh_video_->timestamp = msg->timestamp;
        }
    }

    return ret;
}

int Source::on_audio_impl(SharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;

    bool is_sequence_header =
        flv::Demuxer::IsAACSequenceHeader(msg->payload, msg->size);
    bool drop_for_reduce = false;

    if (is_sequence_header && cache_sh_audio_ &&
        _config->GetReduceSequenceHeader(request_->vhost)) {
        if (cache_sh_audio_->size == msg->size) {
            drop_for_reduce = Utils::BytesEquals(cache_sh_audio_->payload,
                                                 msg->payload, msg->size);
            rs_warn("drop for reduce sh audio, size=%d", msg->size);
        }
    }

    if (is_sequence_header) {
        flv::Demuxer     demuxer;
        flv::CodecSample sample;

        if ((ret = demuxer.DemuxAudio(msg->payload, msg->size, &sample)) !=
            ERROR_SUCCESS) {
            rs_error("flvdemuxer dumux aac failed. ret=%d", ret);
            return ret;
        }
    }

    if ((ret = dvr_->OnAudio(msg)) != ERROR_SUCCESS) {
        rs_warn(
            "dvr process audio message failed, ignore and disable dvr. ret=%d",
            ret);
        dvr_->OnUnpubish();
        ret = ERROR_SUCCESS;
    }

    if (!drop_for_reduce) {
        for (int i = 0; i < (int)consumers_.size(); i++) {
            Consumer* consumer = consumers_.at(i);
            if ((ret = consumer->Enqueue(msg, atc_, ag_)) != ERROR_SUCCESS) {
                rs_error("dispatch audio failed. ret=%d", ret);
                return ret;
            }
        }
    }

    if (is_sequence_header || !cache_sh_audio_) {
        rs_freep(cache_sh_audio_);
        cache_sh_audio_ = msg->Copy();
    }

    if (is_sequence_header) {
        return ret;
    }

    gop_cache_->Cache(msg);

    if (atc_) {
        if (cache_sh_audio_) {
            cache_sh_audio_->timestamp = msg->timestamp;
        }

        if (cache_sh_video_) {
            cache_sh_video_->timestamp = msg->timestamp;
        }
    }
    return ret;
}

int Source::OnAudio(CommonMessage* msg)
{
    int ret = ERROR_SUCCESS;

    if (!mix_correct_ && is_monotonically_increase_) {
        if (last_packet_time_ > 0 &&
            msg->header.timestamp < last_packet_time_) {
            is_monotonically_increase_ = false;
            rs_warn("AUDIO: stream not monotonically increase, please open "
                    "mix_correct.");
        }
    }

    last_packet_time_ = msg->header.timestamp;

    SharedPtrMessage shared_msg;
    if ((shared_msg.Create(msg)) != ERROR_SUCCESS) {
        rs_error("initialize the audio failed. ret=%d", ret);
        return ret;
    }

    if (!mix_correct_) {
        return on_audio_impl(&shared_msg);
    }

    mix_queue_->Push(shared_msg.Copy());

    SharedPtrMessage* m = mix_queue_->Pop();
    if (!m) {
        return ret;
    }

    if (m->IsAudio()) {
        on_audio_impl(m);
    }
    else {
        on_video_impl(m);
    }

    rs_freep(m);

    return ret;
}

int Source::OnVideo(CommonMessage* msg)
{
    int ret = ERROR_SUCCESS;

    if (!mix_correct_ && is_monotonically_increase_) {
        if (last_packet_time_ > 0 &&
            msg->header.timestamp < last_packet_time_) {
            is_monotonically_increase_ = false;
            rs_warn("VIDEO: stream not monotonically increase, please open "
                    "mix_correct.");
        }
    }

    last_packet_time_ = msg->header.timestamp;

    SharedPtrMessage shared_msg;
    if ((shared_msg.Create(msg)) != ERROR_SUCCESS) {
        rs_error("initialize the video failed. ret=%d", ret);
        return ret;
    }

    if (!mix_correct_) {
        return on_video_impl(&shared_msg);
    }

    mix_queue_->Push(shared_msg.Copy());

    SharedPtrMessage* m = mix_queue_->Pop();
    if (!m) {
        return ret;
    }

    if (m->IsAudio()) {
        on_audio_impl(m);
    }
    else {
        on_video_impl(m);
    }

    rs_freep(m);

    return ret;
}

int Source::OnMetadata(CommonMessage* msg, OnMetadataPacket* pkt)
{
    int ret = ERROR_SUCCESS;

    // when exists the duration, remove it to make ExoPlayer happy.
    AMF0Any* prop = NULL;
    if (pkt->metadata->GetValue("duration") != NULL) {
        pkt->metadata->Remove("duration");
    }

    std::ostringstream oss;
    if ((prop = pkt->metadata->EnsurePropertyNumber("width")) != nullptr) {
        oss << ", width=" << (int)prop->ToNumber();
    }
    if ((prop = pkt->metadata->EnsurePropertyNumber("height")) != nullptr) {
        oss << ", height=" << (int)prop->ToNumber();
    }
    if ((prop = pkt->metadata->EnsurePropertyString("videocodecid")) !=
        nullptr) {
        oss << ", vcodec=" << prop->ToString();
    }
    if ((prop = pkt->metadata->EnsurePropertyString("audiocodecid")) !=
        nullptr) {
        oss << ", acodec=" << prop->ToString();
    }

    rs_trace("got metadata%s", oss.str().c_str());

    atc_ = _config->GetATC(request_->vhost);
    if (_config->GetATCAuto(request_->vhost)) {
        if ((prop = pkt->metadata->GetValue("bravo_atc")) != NULL) {
            if (prop->IsString() && prop->ToString() == "true") {
                atc_ = true;
            }
        }
    }

    int   size    = 0;
    char* payload = nullptr;
    if ((ret = pkt->Encode(size, payload)) != ERROR_SUCCESS) {
        rs_error("encode metadata error. ret=%d", ret);
        rs_freepa(payload);
        return ret;
    }

    if (size <= 0) {
        rs_warn("ignore the invalid metadata. size=%d", size);
        return ret;
    }

    // bool drop_for_reduce = false;
    // if (cache_metadata_ && _config->GetReduceSequenceHeader(request_->vhost))
    // {
    //     drop_for_reduce = true;
    //     rs_warn("drop for reduce sh metadata, size=%d", msg->size);
    // }

    rs_freep(cache_metadata_);
    cache_metadata_ = new SharedPtrMessage;

    if ((ret = cache_metadata_->Create(&msg->header, payload, size)) !=
        ERROR_SUCCESS) {
        rs_error("initialize the cache metadata failed. ret=%d", ret);
        return ret;
    }

    if ((ret = dvr_->OnMetadata(cache_metadata_)) != ERROR_SUCCESS) {
        rs_error("dvr process on_metadata message failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int Source::OnDvrRequestSH()
{
    int ret = ERROR_SUCCESS;

    if (cache_metadata_ &&
        ((ret = dvr_->OnMetadata(cache_metadata_)) != ERROR_SUCCESS)) {
        rs_error("dvr process on_metadata message failed. ret=%d", ret);
        return ret;
    }

    if (cache_sh_audio_ &&
        ((ret = dvr_->OnAudio(cache_sh_audio_)) != ERROR_SUCCESS)) {
        rs_error("dvr process audio sequence header message failed. ret=%d",
                 ret);
        return ret;
    }

    if (cache_sh_video_ &&
        ((ret = dvr_->OnVideo(cache_sh_video_)) != ERROR_SUCCESS)) {
        rs_error("dvr process video sequence header message failed. ret=%d",
                 ret);
        return ret;
    }

    return ret;
}

int Source::OnPublish()
{
    int ret = ERROR_SUCCESS;

    can_publish_ = false;

    OnSourceIDChange(_context->GetID());

    mix_queue_->Clear();
    gop_cache_->Clear();

    if ((ret = dvr_->OnPublish(request_)) != ERROR_SUCCESS) {
        rs_error("start dvr failed. ret=%d", ret);
        return ret;
    }
    return ret;
}

void Source::OnUnpublish()
{
    dvr_->OnUnpubish();

    if (consumers_.empty()) {
        die_at_ = Utils::GetSteadyMilliSeconds();
    }

    can_publish_ = true;
}

int Source::SourceId()
{
    return 0;
}

int Source::CreateConsumer(Connection* conn,
                           Consumer*&  consumer,
                           bool        ds,  // dispatch sequence header
                           bool        dm,  // dispatch meta data
                           bool        dg)         // dispatch gop cache
{
    int ret = ERROR_SUCCESS;

    consumer = new Consumer(this, conn);
    consumers_.push_back(consumer);

    // queue_size 单位second
    double queue_size = _config->GetQueueSize(request_->vhost);
    consumer->SetQueueSize(queue_size);

    if (atc_ && !gop_cache_->Empty()) {
        if (cache_metadata_) {
            cache_metadata_->timestamp = gop_cache_->StartTime();
        }
        if (cache_sh_audio_) {
            cache_sh_audio_->timestamp = gop_cache_->StartTime();
        }
        if (cache_sh_video_) {
            cache_sh_video_->timestamp = gop_cache_->StartTime();
        }
    }

    if (dm && cache_metadata_ &&
        (ret = consumer->Enqueue(cache_metadata_, atc_, ag_)) !=
            ERROR_SUCCESS) {
        // actually it never return failed.
        rs_error("dispatch metadata failed. ret=%d", ret);
        return ret;
    }

    if (ds && cache_sh_audio_ &&
        (ret = consumer->Enqueue(cache_sh_audio_, atc_, ag_)) !=
            ERROR_SUCCESS) {
        // actually it never return failed.
        rs_error("dispatch audio sequence header failed. ret=%d", ret);
        return ret;
    }

    if (ds && cache_sh_video_ &&
        (ret = consumer->Enqueue(cache_sh_video_, atc_, ag_)) !=
            ERROR_SUCCESS) {
        // actually it never return failed.
        rs_error("dispatch video sequence header failed. ret=%d", ret);
        return ret;
    }

    if (dg && (ret = gop_cache_->Dump(consumer, atc_, ag_)) != ERROR_SUCCESS) {
        rs_error("dispatch cached gop failed. ret=%d", ret);
        return ret;
    }

    rs_trace("create consumer. queue_size=%.2f, jitter=%d", queue_size, ag_);

    return ret;
}

bool Source::Expired()
{
    if (die_at_ == -1) {
        return false;
    }

    if (!can_publish_) {
        return false;
    }

    if (!consumers_.empty()) {
        return false;
    }

    int64_t now = Utils::GetSteadyMilliSeconds();
    if (now > die_at_ + SOURCE_CLEAN_UP_MS) {
        return true;
    }

    return false;
}

int Source::GetSouceID()
{
    return source_id_;
}

int Source::GetPrevSourceID()
{
    return prev_source_id_;
}

void Source::OnSourceIDChange(int id)
{
    if (id == source_id_) {
        return;
    }

    if (prev_source_id_ == -1) {
        prev_source_id_ = id;
    }
    else if (prev_source_id_ != source_id_) {
        prev_source_id_ = source_id_;
    }

    source_id_ = id;

    std::vector<Consumer*>::iterator it;
    for (it = consumers_.begin(); it != consumers_.end(); it++) {
        (*it)->UpdateSourceID();
    }
}

int Source::Cycle()
{
    int ret = ERROR_SUCCESS;
    // TODO:do something
    return ret;
}

int Source::do_cycle_all()
{
    int ret = ERROR_SUCCESS;

    std::map<std::string, Source*>::iterator it;
    for (it = pool_.begin(); it != pool_.end();) {
        Source* source = it->second;

        if ((ret = source->Cycle() != ERROR_SUCCESS)) {
            return ret;
        }

        if (source->Expired()) {
            int cid = source->GetSouceID();
            if (cid == -1 || source->GetPrevSourceID() > 0) {
                cid = source->GetPrevSourceID();
            }
            if (cid > 0) {
                _context->SetID(cid);
            }

            std::string url = it->first;
            it              = pool_.erase(it);
            rs_trace("clean die source[%s]. now total=%d", url.c_str(),
                     (int)pool_.size());
            rs_freep(source);
        }
        else {
            it++;
        }
    }

    return ret;
}

int Source::CycleAll()
{
    int ret = ERROR_SUCCESS;

    int cid = _context->GetID();
    ret     = do_cycle_all();
    _context->SetID(cid);

    return ret;
}
}  // namespace rtmp