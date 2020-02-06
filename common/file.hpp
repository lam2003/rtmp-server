/*
 * @Author: linmin
 * @Date: 2020-02-06 19:13:43
 * @LastEditTime : 2020-02-06 20:21:15
 */
#ifndef RS_FILE_H
#define RS_FILE_H

#include <common/core.hpp>
#include <common/reader.hpp>
#include <common/st.hpp>

#include <string>

// class FileReader : public Reader
// {
// public:
//     FileReader();
//     virtual ~FileReader();

// public:
//     virtual int32_t Open(const std::string &p) override;
//     virtual void Close() override;

// public:
//     virtual bool IsOpen() override;
//     virtual int64_t Tellg() override;
//     virtual void Skip(int64_t size) override;
//     virtual int64_t Lseek(int64_t offset) override;
//     virtual int64_t FileSize() override;
//     virtual int32_t Read(void *buf, size_t size, ssize_t *nread) override;

// private:
//     int32_t fd_;
// };

class FileWriter
{
public:
    FileWriter();
    virtual ~FileWriter();

public:
    virtual int Open(const std::string &path, bool append = false);
    virtual void Close();
    virtual bool IsOpen();
    virtual void Lseek(int64_t offset);
    virtual int64_t Tellg();
    virtual int Write(void *buf, size_t count, ssize_t *pnwrite);
    virtual int Writev(iovec *iov, int iovcnt, ssize_t *pnwrite);

private:
    std::string path_;
    st_netfd_t stfd_;
};
#endif