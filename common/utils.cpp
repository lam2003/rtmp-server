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

std::string Utils::StringReplace(const std::string &str, const std::string &oldstr, const std::string &newstr)
{
    std::string retstr = str;

    if (oldstr == newstr)
    {
        return retstr;
    }

    size_t pos = 0;

    while ((pos = retstr.find(oldstr, pos)) != std::string::npos)
    {
        retstr = retstr.replace(pos, oldstr.length(), newstr);
    }

    return retstr;
}

bool Utils::StringEndsWith(const std::string &str, const std::string &flag)
{
    return str.rfind(flag) == (str.length() - flag.length());
}

std::string Utils::StringEraseLastSubstr(const std::string &str, const std::string &erase_str)
{
    std::string retstr = str;

    size_t pos = retstr.rfind(erase_str);
    if (pos != std::string::npos)
    {
        retstr = retstr.substr(0, pos);
    }

    return retstr;
}

std::string Utils::StringTrimStart(const std::string &str, const std::string &trim_chars)
{
    std::string retstr = str;

    for (int i = 0; i < (int)trim_chars.length(); i++)
    {
        char ch = trim_chars.at(i);
        while (!retstr.empty() && retstr.at(0) == ch)
        {
            retstr.erase(retstr.begin());
            //reset
            i = -1;
        }
    }
    return retstr;
}

std::string Utils::StringTrimEnd(const std::string &str, const std::string &trim_chars)
{
    std::string retstr = str;

    for (int i = 0; i < (int)trim_chars.length(); i++)
    {
        char ch = trim_chars.at(i);
        while (!retstr.empty() && retstr.at(retstr.length() - 1) == ch)
        {
            retstr.erase(retstr.end() - 1);
            //reset
            i = -1;
        }
    }

    return retstr;
}

std::string Utils::StringRemove(const std::string &str, const std::string &remove_chars)
{
    std::string retstr = str;

    for (int i = 0; i < (int)remove_chars.length(); i++)
    {
        char ch = remove_chars.at(i);
        for (std::string::iterator it = retstr.begin(); it != retstr.end();)
        {
            if (*it == ch)
            {
                it = retstr.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    return retstr;
}