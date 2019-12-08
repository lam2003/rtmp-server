#ifndef RS_CONFIG_HPP
#define RS_CONFIG_HPP

#include <common/config/reload.hpp>

class IConfig
{
public:
    IConfig();
    virtual ~IConfig();

public:
    virtual void Subscribe(IReloadHandler *handler);
    virtual void UnSubscirbe(IReloadHandler *handler);
    virtual int Reload();

protected:
    
};

#endif