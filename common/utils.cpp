#include <common/utils.hpp>
#include <common/config.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include <string.h>

#include <chrono>
#include <sstream>

#define TIME_FORMAT_BUFLEN 255

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

std::string Utils::GetLocalIP(int32_t fd)
{
    std::string ip = "";

    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(fd, (sockaddr *)&addr, &addrlen) == -1)
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

int Utils::GetLocalPort(int32_t fd)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(fd, (sockaddr *)&addr, &addrlen) == -1)
    {
        return 0;
    }

    return ntohs(addr.sin_port);
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

int64_t Utils::GetSteadyNanoSeconds()
{
    using namespace std::chrono;
    steady_clock::time_point now = steady_clock::now();
    return duration_cast<nanoseconds>(now.time_since_epoch()).count();
}

int64_t Utils::GetSteadyMicroSeconds()
{
    using namespace std::chrono;
    steady_clock::time_point now = steady_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
}

int64_t Utils::GetSteadyMilliSeconds()
{
    using namespace std::chrono;
    steady_clock::time_point now = steady_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

bool Utils::BytesEquals(void *pa, void *pb, int size)
{
    uint8_t *a = (uint8_t *)pa;
    uint8_t *b = (uint8_t *)pb;

    if (!a && !b)
    {
        return true;
    }

    if (!a || !b)
    {
        return false;
    }

    for (int i = 0; i < size; i++)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

std::string Utils::BuildStreamPath(const std::string &template_path,
                                   const std::string &vhost,
                                   const std::string &app,
                                   const std::string &stream)
{
    std::string path = template_path;
    path = StringReplace(path, "[vhost]", vhost);
    path = StringReplace(path, "[app]", app);
    path = StringReplace(path, "[stream]", stream);
    return path;
}

std::string Utils::BuildIndexPath(const std::string &template_path)
{
    static int index = 0;
    std::ostringstream oss;
    oss << index++;
    return StringReplace(template_path, "[timestamp]", oss.str());
}

std::string Utils::BuildTimestampPath(const std::string &template_path, const std::string &format)
{
    timeval tv;
    if (gettimeofday(&tv, nullptr) < 0)
    {
        return BuildIndexPath(template_path);
    }

    struct tm *tm;
    if (_config->GetUTCTime())
    {
        if ((tm = gmtime(&tv.tv_sec)) == nullptr)
        {
            return BuildIndexPath(template_path);
        }
    }
    else
    {
        if ((tm = localtime(&tv.tv_sec)) == nullptr)
        {
            return BuildIndexPath(template_path);
        }
    }

    char temp_buf[TIME_FORMAT_BUFLEN];
    strftime(temp_buf, sizeof(temp_buf), format.c_str(), tm);

    return StringReplace(template_path, "[timestamp]", temp_buf);
}

std::string Utils::BuildIndexSuffixPath(const std::string &template_path, int index)
{
    size_t pos;
    if ((pos = template_path.find_last_of(".flv")) == std::string::npos)
    {
        return template_path;
    }

    pos -= 3;

    std::string pre = template_path.substr(0, pos);

    std::ostringstream oss;
    oss << pre << "_" << index << ".flv";
    return oss.str();
}

bool Utils::IsFileExist(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) != 0)
    {
        return false;
    }
    return true;
}

static int do_create_dir_recursively(const std::string &dir)
{
    int ret = ERROR_SUCCESS;

    if (Utils::IsFileExist(dir))
    {
        return ERROR_SYSTEM_DIR_EXISTS;
    }

    size_t pos;
    if ((pos = dir.rfind("/")) != std::string::npos)
    {
        std::string parent = dir.substr(0, pos);
        ret = do_create_dir_recursively(parent);
        if (ret != ERROR_SUCCESS && ret != ERROR_SYSTEM_DIR_EXISTS)
        {
            return ret;
        }

        ret = ERROR_SUCCESS;
    }

    mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH;
    if (::mkdir(dir.c_str(), mode) < 0)
    {
        if (errno == EEXIST)
        {
            return ERROR_SYSTEM_DIR_EXISTS;
        }

        ret = ERROR_SYSTEM_CREATE_DIR;
        rs_error("create dir %s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }

    return ret;
}

int Utils::CreateDirRecursively(const std::string &dir)
{
    int ret = ERROR_SUCCESS;
    ret = do_create_dir_recursively(dir);
    if (ret == ERROR_SYSTEM_DIR_EXISTS)
    {
        ret = ERROR_SUCCESS;
    }

    return ret;
}