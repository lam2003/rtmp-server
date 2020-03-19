/*
 * @Author: linmin
 * @Date: 2020-02-08 13:11:52
 * @LastEditTime: 2020-03-18 18:44:11
 */
#include <common/config.hpp>

Config::Config() {}

Config::~Config() {}

int32_t Config::GetChunkSize(const std::string& vhost)
{
    return 15000;
}

void    Config::Subscribe(IReloadHandler* handler) {}
void    Config::UnSubscribe(IReloadHandler* handler) {}
int32_t Config::Reload()
{
    return 0;
}

bool Config::GetATC(const std::string& vhost)
{
    return false;
}

bool Config::GetATCAuto(const std::string& vhost)
{
    return false;
}

bool Config::GetMREnabled(const std::string& vhost)
{
    return true;
}

int Config::GetMRSleepMS(const std::string& vhost)
{
    return 350;
}

bool Config::GetRealTimeEnabled(const std::string& vhost)
{
    return false;
}

bool Config::GetReduceSequenceHeader(const std::string& vhost)
{
    return true;
}

bool Config::GetVhostIsEdge(const std::string& vhost)
{
    return true;
}

int Config::GetPublishFirstPktTimeout(const std::string& vhost)
{
    return 20000;
}

int Config::GetPublishNormalPktTimeout(const std::string& vhost)
{
    return 5000;
}

bool Config::GetTCPNoDelay(const std::string& vhost)
{
    return true;
}

int Config::GetDvrTimeJitter(const std::string& vhost)
{
    return 1;
}

std::string Config::GetDvrPath(const std::string& vhost)
{
    return "/home/linmin/flv_test";
}

bool Config::GetUTCTime()
{
    return false;
}

bool Config::GetDvrWaitKeyFrame(const std::string& vhost)
{
    return true;
}

std::string Config::GetDvrPlan(const std::string& vhost)
{
    return RS_CONFIG_NVR_PLAN_SEGMENT;
}

int Config::GetDrvDuration(const std::string& vhost)
{
    // seconds
    return 10;
}

bool Config::GetDvrEnbaled(const std::string& vhost)
{
    return true;
}

bool Config::GetParseSPS(const std::string& vhost)
{
    return true;
}

int Config::GetQueueSize(const std::string& vhost)
{
    return 5;
}