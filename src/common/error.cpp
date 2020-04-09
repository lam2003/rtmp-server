#include <common/error.hpp>

bool is_client_gracefully_close(int32_t err_code)
{
    return err_code == ERROR_SOCKET_READ ||
           err_code == ERROR_SOCKET_READ_FULLY ||
           err_code == ERROR_SOCKET_WRITE ||
           err_code == ERROR_SOCKET_TIMEOUT;
}

bool is_system_control_error(int err_code)
{
    return err_code == ERROR_CONTROL_REPUBLISH ||
           err_code == ERROR_CONTROL_RTMP_CLOSE;
}