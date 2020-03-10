/*
 * @Author: linmin
 * @Date: 2020-02-08 13:14:31
 * @LastEditTime: 2020-03-10 15:49:30
 */
#include <protocol/rtmp/dvr.hpp>
#include <common/config.hpp>
#include <protocol/amf/amf0.hpp>

//48kHz/1024=46.875fps
//46.875fps*10s=468.75
#define NUM_TO_JUDGE_DVR_ONLY_HAS_AUDIO 500

namespace rtmp
{

FlvSegment::FlvSegment(DvrPlan *plan)
{
    request_ = nullptr;
    plan_ = plan;
    muxer_ = new flv::Muxer;
    jitter_ = nullptr;
    jitter_algorithm_ = JitterAlgorithm::OFF;
    writer_ = new FileWriter;
    duration_offset_ = 0;
    filesize_offset_ = 0;
    temp_flv_file_ = "";
    path_ = "";
    has_keyframe_ = false;
    start_time_ = -1;
    duration_ = 0;
    stream_start_time_ = 0;
    stream_duration_ = 0;
    stream_previous_pkt_time_ = -1;
}

FlvSegment::~FlvSegment()
{
    rs_freep(writer_);
    rs_freep(jitter_);
    rs_freep(muxer_);
}

int FlvSegment::Initialize(Request *request)
{
    int ret = ERROR_SUCCESS;

    request_ = request;
    jitter_algorithm_ = (JitterAlgorithm)_config->GetDvrTimeJitter(request->vhost);

    return ret;
}

bool FlvSegment::IsOverflow(int64_t max_duration)
{
    return duration_ > max_duration;
}

std::string FlvSegment::generate_path()
{
    // path setting like "/data/[vhost]/[app]/[stream]/[timestamp].flv"
    std::string path_config = _config->GetDvrPath(request_->vhost);

    if (path_config.find(".flv") != path_config.length() - 4)
    {
        path_config += "/[stream].[timestamp].flv";
    }

    std::string flv_path = path_config;
    flv_path = Utils::BuildStreamPath(flv_path, request_->vhost, request_->app, request_->stream);
    flv_path = Utils::BuildTimestampPath(flv_path);

    return flv_path;
}

int FlvSegment::on_update_duration(SharedPtrMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (start_time_ < 0)
    {
        start_time_ = msg->timestamp;
    }

    if (stream_previous_pkt_time_ < 0 || stream_previous_pkt_time_ > msg->timestamp)
    {
        stream_previous_pkt_time_ = msg->timestamp;
    }

    duration_ += msg->timestamp - stream_previous_pkt_time_;
    stream_duration_ += msg->timestamp - stream_previous_pkt_time_;

    stream_previous_pkt_time_ = msg->timestamp;

    return ret;
}

int FlvSegment::create_jitter(bool new_flv_file)
{
    int ret = ERROR_SUCCESS;

    if (new_flv_file)
    {
        rs_freep(jitter_);
        jitter_ = new Jitter;

        start_time_ = -1;
        stream_previous_pkt_time_ = -1;
        stream_start_time_ = Utils::GetSteadyMilliSeconds();
        stream_duration_ = 0;

        has_keyframe_ = false;
        duration_ = 0;
        return ret;
    }

    if (jitter_)
    {
        return ret;
    }

    jitter_ = new Jitter;

    return ret;
}

int FlvSegment::Open(bool use_temp_file)
{
    int ret = ERROR_SUCCESS;

    if (writer_->IsOpen())
    {
        return ret;
    }

    path_ = generate_path();
    bool new_flv_file = !Utils::IsFileExist(path_);

    std::string dir = path_.substr(0, path_.rfind("/"));
    if ((ret = Utils::CreateDirRecursively(dir)) != ERROR_SUCCESS)
    {
        rs_error("create dir %s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }

    if ((ret = create_jitter(new_flv_file)) != ERROR_SUCCESS)
    {
        rs_error("create jitter failed. path=%s, new_flv_file=%d, ret=%d", path_.c_str(), new_flv_file, ret);
        return ret;
    }

    if (!new_flv_file || !use_temp_file)
    {
        temp_flv_file_ = path_;
    }
    else
    {
        temp_flv_file_ = path_ + ".tmp";
    }

    if (!new_flv_file)
    {
        if ((ret = writer_->Open(temp_flv_file_, true)) != ERROR_SUCCESS)
        {
            rs_error("append file stream for file %s failed. ret=%d", temp_flv_file_.c_str(), ret);
            return ret;
        }
    }
    else
    {
        if ((ret = writer_->Open(temp_flv_file_)) != ERROR_SUCCESS)
        {
            rs_error("open file stream for file %s failed. ret=%d", temp_flv_file_.c_str(), ret);
            return ret;
        }
    }

    if ((ret = muxer_->Initialize(writer_)) != ERROR_SUCCESS)
    {
        rs_error("initialize enc by writer for file %s failed. ret=%d", temp_flv_file_.c_str(), ret);
        return ret;
    }

    if (new_flv_file)
    {
        if ((ret = muxer_->WriteMuxerHeader()) != ERROR_SUCCESS)
        {
            rs_error("write flv header for file %s failed. ret=%d", temp_flv_file_.c_str(), ret);
            return ret;
        }
    }

    duration_offset_ = 0;
    filesize_offset_ = 0;

    return ret;
}

int FlvSegment::UpdateFlvMetadata()
{
    int ret = ERROR_SUCCESS;

    if (!duration_offset_ || !filesize_offset_)
    {
        return ret;
    }

    int64_t cur = writer_->Tellg();

    char *buf = new char[AMF0_LEN_NUMBER];
    rs_auto_freea(char, buf);

    BufferManager manager;
    if ((ret = manager.Initialize(buf, AMF0_LEN_NUMBER)) != ERROR_SUCCESS)
    {
        return ret;
    }

    AMF0Any *size = AMF0Any::Number((double)cur);
    rs_auto_free(AMF0Any, size);

    if ((ret = size->Write(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    writer_->Lseek(filesize_offset_);
    if ((ret = writer_->Write(buf, AMF0_LEN_NUMBER, nullptr)) != ERROR_SUCCESS)
    {
        return ret;
    }

    AMF0Any *dur = AMF0Any::Number((double)duration_ / 1000.0);
    rs_auto_free(AMF0Any, dur);

    manager.Skip(-1 * manager.Pos());
    if ((ret = dur->Write(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    writer_->Lseek(duration_offset_);
    if ((ret = writer_->Write(buf, AMF0_LEN_NUMBER, nullptr)) != ERROR_SUCCESS)
    {
        return ret;
    }

    writer_->Lseek(cur);

    return ret;
}

int FlvSegment::WriteMetadata(SharedPtrMessage *shared_metadata)
{
    int ret = ERROR_SUCCESS;

    SharedPtrMessage *metadata = shared_metadata->Copy();
    rs_auto_free(SharedPtrMessage, metadata);

    if (duration_offset_ || filesize_offset_)
    {
        return ret;
    }

    BufferManager manager;
    if ((ret = manager.Initialize(metadata->payload, metadata->size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    AMF0Any *name = AMF0Any::String();
    rs_auto_free(AMF0Any, name);

    if ((ret = name->Read(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    AMF0Any *object = AMF0Any::Object();
    rs_auto_free(AMF0Any, object);

    if ((ret = object->Read(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    AMF0Object *obj = object->ToObject();
    obj->Set("filesize", nullptr);
    obj->Set("duration", nullptr);

    obj->Set("service", AMF0Any::String(RS_SERVER));
    obj->Set("filesize", AMF0Any::Number(0));
    obj->Set("duration", AMF0Any::Number(0));

    int size = name->TotalSize() + obj->TotalSize();
    char *payload = new char[size];
    rs_auto_free(char, payload);

    //11B flv header, 3B object EOF, 8B number value, 1B number flag
    duration_offset_ = writer_->Tellg() + 11 + size - AMF0_LEN_OBJ_EOF - AMF0_LEN_NUMBER;
    //2B string flag
    filesize_offset_ = duration_offset_ - AMF0_LEN_NUMBER - AMF0_LEN_STR(std::string("duration"));

    if ((ret = manager.Initialize(payload, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = name->Write(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = obj->Write(&manager)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = muxer_->WriteMetadata(payload, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int FlvSegment::WriteAudio(SharedPtrMessage *shared_audio)
{
    int ret = ERROR_SUCCESS;

    SharedPtrMessage *audio = shared_audio->Copy();
    rs_auto_free(SharedPtrMessage, audio);

    if (jitter_->Correct(audio, jitter_algorithm_) != ERROR_SUCCESS)
    {
        return ret;
    }

    char *payload = audio->payload;
    int size = audio->size;

    int64_t timestamp = plan_->filter_timestamp(audio->timestamp);
    if ((ret = muxer_->WriteAudio(timestamp, payload, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = on_update_duration(audio)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int FlvSegment::WriteVideo(SharedPtrMessage *shared_video)
{
    int ret = ERROR_SUCCESS;

    SharedPtrMessage *video = shared_video->Copy();
    rs_auto_free(SharedPtrMessage, video);

    char *payload = video->payload;
    int size = video->size;

    bool is_sequence_header = flv::Demuxer::IsAVCSequenceHeader(payload, size);
    bool is_keyframe = flv::Demuxer::IsAVC(payload, size) && flv::Demuxer::IsKeyFrame(payload, size) && !is_sequence_header;

    if (is_keyframe)
    {
        has_keyframe_ = true;
        if ((ret = plan_->on_keyframe()) != ERROR_SUCCESS)
        {
            return ret;
        }
    }

    if (!has_keyframe_ && !is_sequence_header)
    {
        bool wait_keyframe = _config->GetDvrWaitKeyFrame(request_->vhost);
        if (wait_keyframe)
        {
            return ret;
        }
    }

    if ((jitter_->Correct(video, jitter_algorithm_)) != ERROR_SUCCESS)
    {
        return ret;
    }

    int64_t timestamp = plan_->filter_timestamp(video->timestamp);
    if ((ret = muxer_->WriteVideo(timestamp, payload, size)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

std::string FlvSegment::GetPath()
{
    return path_;
}

int FlvSegment::Close()
{
    int ret = ERROR_SUCCESS;

    if (!writer_->IsOpen())
    {
        return ret;
    }

    if ((ret = UpdateFlvMetadata()) != ERROR_SUCCESS)
    {
        return ret;
    }

    writer_->Close();

    if (temp_flv_file_ != path_)
    {
        if (::rename(temp_flv_file_.c_str(), path_.c_str()) < 0)
        {
            ret = ERROR_SYSTEM_FILE_RENAME;
            rs_error("remove flv file failed. %s => %s. ret=%d", temp_flv_file_.c_str(), path_.c_str(), ret);
            return ret;
        }
    }

    if ((ret = plan_->on_reap_segment()) != ERROR_SUCCESS)
    {
        rs_error("notify plan to reap segment failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

DvrPlan::DvrPlan()
{
    request_ = nullptr;
    dvr_enabled_ = false;
    segment_ = new FlvSegment(this);
}

DvrPlan::~DvrPlan()
{
    rs_freep(segment_);
}

int DvrPlan::Initialize(Request *request)
{
    int ret = ERROR_SUCCESS;

    request_ = request;

    if ((ret = segment_->Initialize(request)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int DvrPlan::OnAudio(SharedPtrMessage *shared_audio)
{
    int ret = ERROR_SUCCESS;

    if (!dvr_enabled_)
    {
        return ret;
    }

    if ((ret = segment_->WriteAudio(shared_audio)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int DvrPlan::OnVideo(SharedPtrMessage *shared_video)
{
    int ret = ERROR_SUCCESS;

    if (!dvr_enabled_)
    {
        return ret;
    }

    if ((ret = segment_->WriteVideo(shared_video)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int DvrPlan::OnMetadata(SharedPtrMessage *shared_metadata)
{
    int ret = ERROR_SUCCESS;

    if (!dvr_enabled_)
    {
        return ret;
    }

    if ((ret = segment_->WriteMetadata(shared_metadata)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int64_t DvrPlan::filter_timestamp(int64_t timestamp)
{
    return timestamp;
}

int DvrPlan::on_keyframe()
{
    return ERROR_SUCCESS;
}

int DvrPlan::on_reap_segment()
{
    return ERROR_SUCCESS;
}

DvrPlan *DvrPlan::CreatePlan(const std::string &vhost)
{
    std::string plan = _config->GetDvrPlan(vhost);
    if (rs_config_dvr_is_plan_segment(plan))
    {
        return new DvrSegmentPlan;
    }
    else
    {
        rs_error("invalid dvr plan=%s, vhost=%s", plan.c_str(), vhost.c_str());
        rs_assert(false);
    }
}

DvrSegmentPlan::DvrSegmentPlan()
{
    segment_duration_ = -1;
    sh_video_ = sh_audio_ = metadata_ = nullptr;
    audio_num_before_segment_ = 0;
}

DvrSegmentPlan::~DvrSegmentPlan()
{
    rs_freep(sh_video_);
    rs_freep(sh_audio_);
    rs_freep(metadata_);
}

int DvrSegmentPlan::Initialize(Request *request)
{
    int ret = ERROR_SUCCESS;
    if ((ret = DvrPlan::Initialize(request)) != ERROR_SUCCESS)
    {
        return ret;
    }

    segment_duration_ = _config->GetDrvDuration(request->vhost);
    segment_duration_ *= 1000;

    return ret;
}

int DvrSegmentPlan::OnPublish()
{
    int ret = ERROR_SUCCESS;

    if (dvr_enabled_)
    {
        return ret;
    }

    if (!_config->GetDvrEnbaled(request_->vhost))
    {
        return ret;
    }

    if ((ret = segment_->Close()) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = segment_->Open()) != ERROR_SUCCESS)
    {
        return ret;
    }

    dvr_enabled_ = true;

    return ret;
}

void DvrSegmentPlan::OnUnpublish()
{
}

int DvrSegmentPlan::OnMetadata(SharedPtrMessage *shared_metadata)
{
    int ret = ERROR_SUCCESS;

    rs_freep(metadata_);
    metadata_ = shared_metadata->Copy();

    if ((ret = DvrPlan::OnMetadata(shared_metadata)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int DvrSegmentPlan::update_duration(SharedPtrMessage *msg)
{
    int ret = ERROR_SUCCESS;

    if (segment_duration_ <= 0 || !segment_->IsOverflow(segment_duration_))
    {
        return ret;
    }

    if (_config->GetDvrWaitKeyFrame(request_->vhost))
    {

        if (msg->IsVideo())
        {
            char *payload = msg->payload;
            int size = msg->size;
            bool is_keyframe = flv::Demuxer::IsAVC(payload, size) && flv::Demuxer::IsKeyFrame(payload, size) && !flv::Demuxer::IsAVCSequenceHeader(payload, size);
            if (!is_keyframe)
            {
                return ret;
            }
            audio_num_before_segment_ = 0;
        }

        if (audio_num_before_segment_ < NUM_TO_JUDGE_DVR_ONLY_HAS_AUDIO && msg->IsAudio())
        {
            audio_num_before_segment_++;
            return ret;
        }
    }

    if ((ret = segment_->Close()) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = segment_->Open()) != ERROR_SUCCESS)
    {
        return ret;
    }

    if (metadata_ && (ret = DvrPlan::OnMetadata(metadata_) != ERROR_SUCCESS))
    {
        return ret;
    }

    if (sh_video_ && (ret = DvrPlan::OnVideo(sh_video_) != ERROR_SUCCESS))
    {
        return ret;
    }

    if (sh_audio_ && (ret = DvrPlan::OnAudio(sh_audio_) != ERROR_SUCCESS))
    {
        return ret;
    }

    return ret;
}

int DvrSegmentPlan::OnAudio(SharedPtrMessage *shared_audio)
{
    int ret = ERROR_SUCCESS;

    if (flv::Demuxer::IsAACSequenceHeader(shared_audio->payload, shared_audio->size))
    {
        rs_freep(sh_audio_);
        sh_audio_ = shared_audio->Copy();
    }

    if ((ret = update_duration(shared_audio)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = DvrPlan::OnAudio(shared_audio)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int DvrSegmentPlan::OnVideo(SharedPtrMessage *shared_video)
{
    int ret = ERROR_SUCCESS;

    if (flv::Demuxer::IsAVCSequenceHeader(shared_video->payload, shared_video->size))
    {
        rs_freep(sh_video_);
        sh_video_ = shared_video->Copy();
    }

    if ((ret = update_duration(shared_video)) != ERROR_SUCCESS)
    {
        return ret;
    }

    if ((ret = DvrPlan::OnVideo(shared_video)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

Dvr::Dvr()
{
    source_ = nullptr;
    plan_ = nullptr;
}

Dvr::~Dvr()
{
    rs_freep(plan_);
}

int Dvr::Initialize(Source *source, Request *request)
{
    int ret = ERROR_SUCCESS;
    rs_freep(plan_);

    plan_ = DvrPlan::CreatePlan(request->vhost);
    if ((ret = plan_->Initialize(request)) != ERROR_SUCCESS)
    {
        return ret;
    }

    source_ = source;
    if ((ret = source->OnDvrRequestSH()) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int Dvr::OnPublish(Request *request)
{
    int ret = ERROR_SUCCESS;

    if ((ret = plan_->OnPublish()) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

void Dvr::OnUnpubish()
{
    plan_->OnUnpublish();
}

int Dvr::OnMetadata(SharedPtrMessage *shared_metadata)
{
    int ret = ERROR_SUCCESS;

    if ((ret = plan_->OnMetadata(shared_metadata)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int Dvr::OnAudio(SharedPtrMessage *shared_audio)
{
    int ret = ERROR_SUCCESS;

    if ((ret = plan_->OnAudio(shared_audio)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

int Dvr::OnVideo(SharedPtrMessage *shared_video)
{
    int ret = ERROR_SUCCESS;

    if ((ret = plan_->OnVideo(shared_video)) != ERROR_SUCCESS)
    {
        return ret;
    }

    return ret;
}

} // namespace rtmp