#include <common/error.hpp>

bool IsClientGracefullyClose(int32_t err_code)
{
    return err_code == ERROR_SOCKET_READ ||
           err_code == ERROR_SOCKET_READ_FULLY ||
           err_code == ERROR_SOCKET_WRITE ||
           err_code == ERROR_SOCKET_TIMEOUT;
}