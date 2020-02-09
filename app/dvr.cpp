/*
 * @Author: linmin
 * @Date: 2020-02-08 13:14:31
 * @LastEditTime : 2020-02-08 17:34:05
 */
#include <app/dvr.hpp>
#include <common/config.hpp>

FlvSegment::FlvSegment(DvrPlan *plan)
{
    request_ = nullptr;
    plan_ = plan;
    enc_ = new flv::Encoder;
    jitter_ = nullptr;
    jitter_algorithm_ = rtmp::JitterAlgorithm::OFF;
    writer_ = new FileWriter;
    duration_offset_ = 0;
    filesize_offset_ = 0;
    tmp_flv_file_ = "";
    path_ = "";
    has_keyframe_ = false;
    start_time_ = -1;
    duration_ = 0;
    stream_stream_time_ = 0;
    stream_duration_ = 0;
    stream_previous_pkt_time_ = -1;
}

FlvSegment::~FlvSegment()
{
    rs_freep(writer_);
    rs_freep(jitter_);
    rs_freep(enc_);
}

int FlvSegment::Initialize(rtmp::Request *request)
{
    int ret = ERROR_SUCCESS;

    request_ = request;
    jitter_algorithm_ = (rtmp::JitterAlgorithm)_config->GetDvrTimeJitter(request->vhost);

    return ret;
}

bool FlvSegment::IsOverflow(int64_t max_duration)
{
    return duration_ > max_duration;
}

std::string FlvSegment::generate_path()
{
    std::string path_config = _config->GetDvrPath(request_->vhost);

    if (path_config.find(".flv") != path_config.length() - 4)
    {
        path_config += "/[stream].[timestamp].flv";
    }

    std::string flv_path = path_config;
}

int FlvSegment::Open(bool use_temp_file)
{
    int ret = ERROR_SUCCESS;

    if (writer_->IsOpen())
    {
        return ret;
    }

    path_ = generate_path();

    return ret;
}