/*
 * @Author: linmin
 * @Date: 2020-02-08 13:11:52
 * @LastEditTime : 2020-02-08 21:49:39
 */
#include <common/config.hpp>

Config::Config()
{
}

Config::~Config()
{
}

int32_t Config::GetChunkSize(const std::string &vhost)
{
    return 128;
}

void Config::Subscribe(IReloadHandler *handler)
{
}
void Config::UnSubscribe(IReloadHandler *handler)
{
}
int32_t Config::Reload()
{
    return 0;
}

bool Config::GetATC(const std::string &vhost)
{
    return false;
}

bool Config::GetMREnabled(const std::string &vhost)
{
    return true;
}

int Config::GetMRSleepMS(const std::string &vhost)
{
    return 350;
}

bool Config::GetRealTimeEnabled(const std::string &vhost)
{
    return false;
}

bool Config::GetReduceSequenceHeader(const std::string &vhost)
{
    return true;
}

bool Config::GetVhostIsEdge(const std::string &vhost)
{
    return true;
}

int Config::GetPublishFirstPktTimeout(const std::string &vhost)
{
    return 20000;
}

int Config::GetPublishNormalPktTimeout(const std::string &vhost)
{
    return 5000;
}

bool Config::GetTCPNoDelay(const std::string &vhost)
{
    return true;
}

int Config::GetDvrTimeJitter(const std::string &vhost)
{
    return 1;
}

std::string Config::GetDvrPath(const std::string &vhost)
{
    return "/win/flv_test";
}

bool Config::GetUTCTime()
{
    return false;
}