#include <common/config/reload.hpp>
#include <common/error.hpp>

IReloadHandler::IReloadHandler()
{
}

IReloadHandler::~IReloadHandler()
{
}

int IReloadHandler::OnReloadUTCTime()
{
    return ERROR_SUCCESS;
}

int IReloadHandler::OnReloadLogTank()
{
    return ERROR_SUCCESS;
}

int IReloadHandler::OnReloadLogLevel()
{
    return ERROR_SUCCESS;
}

int IReloadHandler::OnReloadLogFile()
{
    return ERROR_SUCCESS;
}