/*
 * @Author: linmin
 * @Date: 2020-02-06 19:10:29
 * @LastEditTime : 2020-02-10 15:27:22
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
    static DvrPlan *CreatePlan(const std::string &vhost);
    virtual int Initialize(rtmp::Request *request);
    virtual int OnPublish() = 0;
    virtual void OnUnpublish() = 0;
    virtual int OnMetadata(rtmp::SharedPtrMessage *shared_metadata);
    virtual int OnAudio(rtmp::SharedPtrMessage *shared_audio);
    virtual int OnVideo(rtmp::SharedPtrMessage *shared_video);

protected:
    virtual int on_keyframe();
    virtual int on_reap_segment();
    virtual int64_t filter_timestamp(int64_t timestamp);

protected:
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
    virtual int Initialize(rtmp::Request *request) override;
    virtual int OnPublish() override;
    virtual void OnUnpublish() override;
    virtual int OnMetadata(rtmp::SharedPtrMessage *shared_metadata) override;
    virtual int OnAudio(rtmp::SharedPtrMessage *shared_audio) override;
    virtual int OnVideo(rtmp::SharedPtrMessage *shared_video) override;

private:
    int update_duration(rtmp::SharedPtrMessage *msg);

private:
    int segment_duration_;
    rtmp::SharedPtrMessage *sh_video_;
    rtmp::SharedPtrMessage *sh_audio_;
    rtmp::SharedPtrMessage *metadata_;
};

class DvrAppendPlan : public DvrPlan
{
public:
    DvrAppendPlan();
    virtual ~DvrAppendPlan();

public:
    virtual int OnPublish() override;
    virtual void OnUnpublish() override;
};

class DvrSessionPlan : public DvrPlan
{
public:
    DvrSessionPlan();
    virtual ~DvrSessionPlan();

public:
    virtual int OnPublish() override;
    virtual void OnUnpublish() override;
};

class Dvr
{
public:
    Dvr();
    virtual ~Dvr();

public:
    virtual int Initialize(rtmp::Source *source, rtmp::Request *request);
    virtual int OnPublish(rtmp::Request *request);
    virtual void OnUnpubish();
    virtual int OnMetadata(rtmp::SharedPtrMessage *shared_metadata);
    virtual int OnAudio(rtmp::SharedPtrMessage *shared_audio);
    virtual int OnVideo(rtmp::SharedPtrMessage *shared_video);

private:
    rtmp::Source *source_;
    DvrPlan *plan_;
};

#endif