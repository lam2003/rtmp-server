/*
 * @Author: linmin
 * @Date: 2020-02-08 11:59:08
 * @LastEditTime: 2020-02-21 13:38:28
 */
#ifndef RS_CONFIG_HPP
#define RS_CONFIG_HPP

#include <common/reload.hpp>

#define RS_CONFIG_NVR_PLAN_SESSION "session"
#define RS_CONFIG_NVR_PLAN_APPEND "append"
#define RS_CONFIG_NVR_PLAN_SEGMENT "segment"

inline bool rs_config_dvr_is_plan_segment(const std::string &plan)
{
    return plan == RS_CONFIG_NVR_PLAN_SEGMENT;
}

class Config
{
public:
    Config();
    virtual ~Config();

public:
    virtual void Subscribe(IReloadHandler *handler);
    virtual void UnSubscribe(IReloadHandler *handler);
    virtual int32_t Reload();
    virtual int32_t GetChunkSize(const std::string &vhost);
    virtual bool GetATC(const std::string &vhost);
    virtual bool GetMREnabled(const std::string &vhost);
    virtual int GetMRSleepMS(const std::string &vhost);
    virtual bool GetRealTimeEnabled(const std::string &vhost);
    virtual bool GetReduceSequenceHeader(const std::string &vhost);
    virtual bool GetVhostIsEdge(const std::string &vhost);
    virtual int GetPublishFirstPktTimeout(const std::string &vhost);
    virtual int GetPublishNormalPktTimeout(const std::string &vhost);
    virtual bool GetTCPNoDelay(const std::string &vhost);
    virtual int GetDvrTimeJitter(const std::string &vhost);
    virtual std::string GetDvrPath(const std::string &vhost);
    virtual bool GetUTCTime();
    virtual bool GetDvrWaitKeyFrame(const std::string &vhost);
    virtual std::string GetDvrPlan(const std::string &vhost);
    virtual int GetDrvDuration(const std::string &vhost);
    virtual bool GetDvrEnbaled(const std::string &vhost);
    virtual bool GetATCAuto(const std::string &vhost);
    virtual bool GetParseSPS(const std::string &vhost);
};

extern Config *_config;

#endif