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
    virtual int32_t OnReloadUTCTime();
    virtual int32_t OnReloadLogTank();
    virtual int32_t OnReloadLogLevel();
    virtual int32_t OnReloadLogFile();
};
#endif