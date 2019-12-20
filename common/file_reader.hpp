#ifndef RS_FILE_READER_H
#define RS_FILE_READER_H

#include <common/core.hpp>
#include <common/reader.hpp>

#include <string>

class FileReader : public Reader
{
public:
    FileReader();
    virtual ~FileReader();

public:
    virtual int Open(const std::string &p) override;
    virtual void Close() override;

public:
    virtual bool IsOpen() override;
    virtual int64_t Tellg() override;
    virtual void Skip(int64_t size) override;
    virtual int64_t Lseek(int64_t offset) override;
    virtual int64_t FileSize() override;
    virtual int Read(void *buf, size_t size, ssize_t *nread) override;

private:
    int fd_;
};

#endif