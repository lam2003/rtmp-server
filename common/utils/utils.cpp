#include <common/utils/utils.hpp>

#include <arpa/inet.h>
#include <string.h>


std::string Utils::GetPeerIP(int fd)
{
    std::string ip = "";

    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getpeername(fd, (sockaddr *)&addr, &addrlen) == -1)
    {
        return ip;
    }

    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof(buf));

    if (inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf)) == nullptr)
    {
        return ip;
    }

    ip = buf;
    return ip;
}