/*
 * @Author: linmin
 * @Date: 2020-02-21 12:51:16
 * @LastEditTime: 2020-02-21 17:56:49
 */
#ifndef RS_BUFFER_HPP
#define RS_BUFFER_HPP

#include <common/core.hpp>
#include <common/io.hpp>

#include <string>

class BufferManager
{
public:
    BufferManager();
    virtual ~BufferManager();

public:
    virtual int Initialize(char *b, int32_t nb);
    virtual char *Data();
    virtual int32_t Size();
    virtual int32_t Pos();
    virtual bool Require(int32_t required_size);
    virtual void Skip(int32_t size);
    virtual bool Empty();

    //read
    virtual int8_t Read1Bytes();
    virtual int16_t Read2Bytes();
    virtual int32_t Read3Bytes();
    virtual int32_t Read4Bytes();
    virtual int64_t Read8Bytes();
    virtual std::string ReadString(int32_t len);
    virtual void ReadBytes(char *data, int32_t size);

    //write
    virtual void Write1Bytes(int8_t value);
    virtual void Write2Bytes(int16_t value);
    virtual void Write3Bytes(int32_t value);
    virtual void Write4Bytes(int32_t value);
    virtual void Write8Bytes(int64_t value);
    virtual void WriteString(const std::string &value);
    virtual void WriteBytes(char *data, int32_t size);

private:
    char *buf_;
    char *ptr_;
    int32_t size_;
};

class BitBufferManager
{
public:
    BitBufferManager();
    virtual ~BitBufferManager();

public:
    virtual int Initialize(BufferManager *manager);
    virtual bool Empty();
    virtual int8_t ReadBit();

private:
    int8_t cb_;
    uint8_t cb_left_;
    BufferManager *manager_;
};

class FastBuffer
{
public:
    FastBuffer();
    virtual ~FastBuffer();

public:
    virtual int Size();
    virtual char *Bytes();
    virtual void SetBuffer(int buffer_size);
    virtual char Read1Bytes();
    virtual char *ReadSlice(int size);
    virtual void Skip(int size);
    virtual int Grow(IBufferReader *r, int required_size);
    virtual void SetMergeReadHandler(bool enable, IMergeReadHandler *mr_handler);

private:
    bool merged_read_;
    IMergeReadHandler *mr_handler_;
    int32_t capacity_;
    char *buf_;
    char *start_;
    char *end_;
};
#endif