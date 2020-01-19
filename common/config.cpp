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
void Config::UnSubscirbe(IReloadHandler *handler)
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