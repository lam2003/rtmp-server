/*
 * @Author: linmin
 * @Date: 2020-02-06 19:10:29
 * @LastEditTime : 2020-02-08 17:33:55
 */
#ifndef RS_DVR_HPP
#define RS_DVR_HPP

#include <common/core.hpp>
#include <common/file.hpp>
#include <protocol/flv.hpp>
#include <protocol/rtmp_source.hpp>

class DvrPlan;

class FlvSegment
{
public:
    FlvSegment(DvrPlan *plan);
    virtual ~FlvSegment();

public:
    virtual int Initialize(rtmp::Request *request);
    virtual bool IsOverflow(int64_t max_duration);
    virtual int Open(bool use_temp_file = true);
    virtual int Close();
    virtual int WriteMetadata(rtmp::SharedPtrMessage *metadata);
    virtual int WriteAudio(rtmp::SharedPtrMessage *shared_audio);
    virtual int WriteVideo(rtmp::SharedPtrMessage *shared_video);
    virtual int UpdateFlvMetadata();
    virtual std::string GetPath();

private:
    std::string generate_path();
    int create_jitter(bool loads_from_flv);
    int on_update_duration(rtmp::SharedPtrMessage *msg);

private:
    rtmp::Request *request_;
    DvrPlan *plan_;
    flv::Encoder *enc_;
    rtmp::Jitter *jitter_;
    rtmp::JitterAlgorithm jitter_algorithm_;
    FileWriter *writer_;
    int64_t duration_offset_;
    int64_t filesize_offset_;
    std::string tmp_flv_file_;
    std::string path_;
    bool has_keyframe_;
    int64_t start_time_;
    int64_t duration_;
    int64_t stream_stream_time_;
    int64_t stream_duration_;
    int64_t stream_previous_pkt_time_;
};

class DvrPlan
{
};

class Dvr
{
};

#endif