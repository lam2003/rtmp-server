#include <common/utils.hpp>

#include <arpa/inet.h>
#include <string.h>

std::string Utils::GetPeerIP(int32_t fd)
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

void Utils::RandomGenerate(char *bytes, int32_t size)
{
    static bool rand_initialized = false;
    if (!rand_initialized)
    {
        rand_initialized = true;
        srand(time(nullptr));
    }

    for (int32_t i = 0; i < size; i++)
    {
        //the common value in [0x0f, 0xf0]
        bytes[i] = 0x0f + (rand() % (0xff - 0x0f - 0x0f));
    }
}