#include <common/reload.hpp>
#include <common/error.hpp>

IReloadHandler::IReloadHandler()
{
}

IReloadHandler::~IReloadHandler()
{
}

int32_t IReloadHandler::OnReloadUTCTime()
{
    return ERROR_SUCCESS;
}

int32_t IReloadHandler::OnReloadLogTank()
{
    return ERROR_SUCCESS;
}

int32_t IReloadHandler::OnReloadLogLevel()
{
    return ERROR_SUCCESS;
}

int32_t IReloadHandler::OnReloadLogFile()
{
    return ERROR_SUCCESS;
}