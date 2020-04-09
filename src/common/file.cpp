/*
 * @Author: linmin
 * @Date: 2020-02-06 19:13:46
 * @LastEditTime : 2020-02-06 20:37:12
 */

#include <common/file.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// FileReader::FileReader() : fd_(-1)
// {
// }

// FileReader::~FileReader()
// {
//     Close();
// }

// int32_t FileReader::Open(const std::string &p)
// {
//     int32_t ret = ERROR_SUCCESS;

//     if (fd_ > 0)
//     {
//         return ERROR_SYSTEM_FILE_ALREADY_OPENED;
//     }

//     return ret;
// }

FileWriter::FileWriter()
{
    stfd_ = nullptr;
}

FileWriter::~FileWriter()
{
    Close();
}

void FileWriter::Close()
{
    int ret = ERROR_SUCCESS;

    if (st_netfd_close(stfd_) < 0)
    {
        ret = ERROR_SYSTEM_FILE_CLOSE;
        rs_error("close file %s failed. ret=%d", path_.c_str(), ret);
    }

    stfd_ = nullptr;
}

int FileWriter::Open(const std::string &path, bool append)
{
    int ret = ERROR_SUCCESS;

    if (stfd_)
    {
        ret = ERROR_SYSTEM_FILE_ALREADY_OPENED;
        rs_error("file %s already opened. ret=%d", path.c_str(), ret);
        return ret;
    }

    int flags = O_CREAT | O_WRONLY | O_TRUNC;
    if (append)
    {
        flags = O_APPEND | O_WRONLY;
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    st_netfd_t stfd = st_open(path.c_str(), flags, mode);

    if (!stfd)
    {
        ret = ERROR_SYSTEM_FILE_OPENE;
        rs_error("open file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }

    path_ = path;
    stfd_ = stfd;
    
    return ret;
}

bool FileWriter::IsOpen()
{
    return stfd_ != nullptr;
}

void FileWriter::Lseek(int64_t offset)
{
    ::lseek(st_netfd_fileno(stfd_), (off_t)offset, SEEK_SET);
}

int64_t FileWriter::Tellg()
{
    return ::lseek(st_netfd_fileno(stfd_), 0, SEEK_CUR);
}

int FileWriter::Write(void *buf, size_t count, ssize_t *pnwrite)
{
    int ret = ERROR_SUCCESS;
    ssize_t nwrite = 0;

    if ((nwrite = st_write(stfd_, buf, count, ST_UTIME_NO_TIMEOUT)) < 0)
    {
        ret = ERROR_SYSTEM_FILE_WRITE;
        rs_error("write to file %s failed. ret=%d", ret);
        return ret;
    }

    if (pnwrite)
    {
        *pnwrite = nwrite;
    }
    return ret;
}

int FileWriter::Writev(iovec *iov, int iovcnt, ssize_t *pnwrite)
{
    int ret = ERROR_SUCCESS;
    ssize_t nwrite = 0;

    if ((nwrite = st_writev(stfd_, iov, iovcnt, ST_UTIME_NO_TIMEOUT)) < 0)
    {
        ret = ERROR_SYSTEM_FILE_WRITE;
        rs_error("writev to file %s failed. ret=%d", ret);
        return ret;
    }

    if (pnwrite)
    {
        *pnwrite = nwrite;
    }

    return ret;
}
