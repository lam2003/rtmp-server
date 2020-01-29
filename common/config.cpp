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
    return false;
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