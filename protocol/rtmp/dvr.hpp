/*
 * @Author: linmin
 * @Date: 2020-02-06 19:10:29
 * @LastEditTime: 2020-03-10 17:55:03
 */
#ifndef RS_DVR_HPP
#define RS_DVR_HPP

#include <common/core.hpp>
#include <common/file.hpp>
#include <common/queue.hpp>
#include <protocol/rtmp/jitter.hpp>
#include <muxer/flv.hpp>

namespace rtmp
{

class DvrPlan;
class Source;

class FlvSegment
{
public:
    FlvSegment(DvrPlan *plan);
    virtual ~FlvSegment();

public:
    virtual int Initialize(Request *request);
    virtual bool IsOverflow(int64_t max_duration);
    virtual int Open(bool use_temp_file = true);
    virtual int Close();
    virtual int WriteMetadata(SharedPtrMessage *shared_metadata);
    virtual int WriteAudio(SharedPtrMessage *shared_audio);
    virtual int WriteVideo(SharedPtrMessage *shared_video);
    virtual int UpdateFlvMetadata();
    virtual std::string GetPath();

private:
    std::string generate_path();
    int create_jitter(bool new_flv_file);
    int on_update_duration(SharedPtrMessage *msg);

private:
    Request *request_;
    DvrPlan *plan_;
    flv::Muxer *muxer_;
    Jitter *jitter_;
    JitterAlgorithm jitter_algorithm_;
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
    virtual int Initialize(Request *request);
    virtual int OnPublish() = 0;
    virtual void OnUnpublish() = 0;
    virtual int OnMetadata(SharedPtrMessage *shared_metadata);
    virtual int OnAudio(SharedPtrMessage *shared_audio);
    virtual int OnVideo(SharedPtrMessage *shared_video);

protected:
    virtual int on_keyframe();
    virtual int on_reap_segment();
    virtual int64_t filter_timestamp(int64_t timestamp);

protected:
    Request *request_;
    bool dvr_enabled_;
    FlvSegment *segment_;
};

class DvrSegmentPlan : public DvrPlan
{
public:
    DvrSegmentPlan();
    virtual ~DvrSegmentPlan();

public:
    virtual int Initialize(Request *request) override;
    virtual int OnPublish() override;
    virtual void OnUnpublish() override;
    virtual int OnMetadata(SharedPtrMessage *shared_metadata) override;
    virtual int OnAudio(SharedPtrMessage *shared_audio) override;
    virtual int OnVideo(SharedPtrMessage *shared_video) override;

private:
    int update_duration(SharedPtrMessage *msg);

private:
    int segment_duration_;
    SharedPtrMessage *sh_video_;
    SharedPtrMessage *sh_audio_;
    SharedPtrMessage *metadata_;
    int audio_num_before_segment_;
};

class Dvr
{
public:
    Dvr();
    virtual ~Dvr();

public:
    virtual int Initialize(Source *source, Request *request);
    virtual int OnPublish(Request *request);
    virtual void OnUnpubish();
    virtual int OnMetadata(SharedPtrMessage *shared_metadata);
    virtual int OnAudio(SharedPtrMessage *shared_audio);
    virtual int OnVideo(SharedPtrMessage *shared_video);

private:
    Source *source_;
    DvrPlan *plan_;
};

} // namespace rtmp

#endif