#ifndef RS_RELOAD_HPP
#define RS_RELOAD_HPP

#include <common/core.hpp>

#include <string>

class IReloadHandler
{
public:
    IReloadHandler();
    virtual ~IReloadHandler();

public:
    virtual int OnReloadUTCTime();
    virtual int OnReloadLogTank();
    virtual int OnReloadLogLevel();
    virtual int OnReloadLogFile();
};
#endif