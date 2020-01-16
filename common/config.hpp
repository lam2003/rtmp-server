#ifndef RS_CONFIG_HPP
#define RS_CONFIG_HPP

#include <common/reload.hpp>

class Config
{
public:
    Config();
    virtual ~Config();

public:
    virtual void Subscribe(IReloadHandler *handler);
    virtual void UnSubscirbe(IReloadHandler *handler);
    virtual int32_t Reload();
    virtual int32_t GetChunkSize(const std::string &vhost);
};

extern Config *_config;

#endif