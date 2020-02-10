/*
 * @Author: linmin
 * @Date: 2020-02-06 19:10:29
 * @LastEditTime : 2020-02-10 14:01:01
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
    virtual int WriteMetadata(rtmp::SharedPtrMessage *shared_metadata);
    virtual int WriteAudio(rtmp::SharedPtrMessage *shared_audio);
    virtual int WriteVideo(rtmp::SharedPtrMessage *shared_video);
    virtual int UpdateFlvMetadata();
    virtual std::string GetPath();

private:
    std::string generate_path();
    int create_jitter(bool new_flv_file);
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
    std::string temp_flv_file_;
    std::string path_;
    bool has_keyframe_;
    int64_t start_time_;
    int64_t duration_;
    int64_t stream_start_time_;
    int64_t stream_duration_;
    int64_t stream_previous_pkt_time_;
};

class DvrPlan
{
    friend class FlvSegment;

public:
    DvrPlan();
    virtual ~DvrPlan();

public:
    static DvrPlan *create_plan(const std::string &vhost);
    virtual int Initialize(rtmp::Request *request);
    virtual int OnPublish() = 0;
    virtual int OnUnpublish() = 0;
    virtual int OnMetadata(rtmp::SharedPtrMessage *shared_metadata);
    virtual int OnAudio(rtmp::SharedPtrMessage *shared_audio);
    virtual int OnVideo(rtmp::SharedPtrMessage *shared_video);

protected:
    virtual int on_keyframe();
    virtual int on_reap_segment();
    virtual int64_t filter_timestamp(int64_t timestamp);

private:
    rtmp::Request *request_;
    bool dvr_enabled_;
    FlvSegment *segment_;
};

class DvrSegmentPlan : public DvrPlan
{
public:
    DvrSegmentPlan();
    virtual ~DvrSegmentPlan();

public:
    virtual int OnPublish() override;
    virtual int OnUnpublish() override;
};

class DvrAppendPlan : public DvrPlan
{
public:
    DvrAppendPlan();
    virtual ~DvrAppendPlan();

public:
    virtual int OnPublish() override;
    virtual int OnUnpublish() override;
};

class DvrSessionPlan : public DvrPlan
{
public:
    DvrSessionPlan();
    virtual ~DvrSessionPlan();

public:
    virtual int OnPublish() override;
    virtual int OnUnpublish() override;
};

class Dvr
{
};

#endif