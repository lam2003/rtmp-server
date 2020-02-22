#include <common/buffer.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

//default recv buffer size 128KB
#define RS_DEFAULT_RECV_BUFFER_SIZE 131072
//socket max buffer size 256KB
#define RS_MAX_SOCKER_BUFFER_SIZE 262144

BufferManager::BufferManager() : buf_(nullptr),
                                 ptr_(nullptr),
                                 size_(0)
{
}

BufferManager::~BufferManager()
{
}

int BufferManager::Initialize(char *b, int32_t nb)
{
    int ret = ERROR_SUCCESS;
    if (!b)
    {
        ret = ERROR_KERNEL_STREAM_INIT;
        rs_error("buffer initialize with null b. ret=%d", ret);
        return ret;
    }

    if (nb <= 0)
    {
        ret = ERROR_KERNEL_STREAM_INIT;
        rs_error("buffer initialize with nb <= 0. ret=%d", ret);
        return ret;
    }

    buf_ = ptr_ = b;
    size_ = nb;
    return ret;
}

char *BufferManager::Data()
{
    return buf_;
}

int32_t BufferManager::Size()
{
    return size_;
}

int32_t BufferManager::Pos()
{
    return int32_t(ptr_ - buf_);
}

bool BufferManager::Empty()
{
    return !buf_ || (ptr_ >= buf_ + size_);
}

bool BufferManager::Require(int32_t required_size)
{
    rs_assert(required_size >= 0);
    return required_size <= size_ - (ptr_ - buf_);
}

void BufferManager::Skip(int32_t size)
{
    rs_assert(ptr_);
    ptr_ += size;
}

int8_t BufferManager::Read1Bytes()
{
    rs_assert(Require(1));
    return (int8_t)*ptr_++;
}

int16_t BufferManager::Read2Bytes()
{
    rs_assert(Require(2));
    int16_t value;
    char *pp = (char *)&value;

    pp[1] = *ptr_++;
    pp[0] = *ptr_++;

    return value;
}

int32_t BufferManager::Read3Bytes()
{
    rs_assert(Require(3));
    int32_t value = 0;
    char *pp = (char *)&value;

    pp[2] = *ptr_++;
    pp[1] = *ptr_++;
    pp[0] = *ptr_++;

    return value;
}

int32_t BufferManager::Read4Bytes()
{
    rs_assert(Require(4));
    int32_t value;
    char *pp = (char *)&value;

    pp[3] = *ptr_++;
    pp[2] = *ptr_++;
    pp[1] = *ptr_++;
    pp[0] = *ptr_++;

    return value;
}

int64_t BufferManager::Read8Bytes()
{
    rs_assert(Require(8));
    int64_t value;
    char *pp = (char *)&value;

    pp[7] = *ptr_++;
    pp[6] = *ptr_++;
    pp[5] = *ptr_++;
    pp[4] = *ptr_++;
    pp[3] = *ptr_++;
    pp[2] = *ptr_++;
    pp[1] = *ptr_++;
    pp[0] = *ptr_++;

    return value;
}

std::string BufferManager::ReadString(int32_t len)
{
    rs_assert(Require(len));
    std::string value;
    value.append(ptr_, len);

    ptr_ += len;

    return value;
}

void BufferManager::ReadBytes(char *data, int32_t size)
{
    rs_assert(Require(size));
    memcpy(data, ptr_, size);
    ptr_ += size;
}

void BufferManager::Write1Bytes(int8_t value)
{
    rs_assert(Require(1));
    *ptr_++ = value;
}

void BufferManager::Write2Bytes(int16_t value)
{
    rs_assert(Require(2));
    char *pp = (char *)&value;
    *ptr_++ = pp[1];
    *ptr_++ = pp[0];
}

void BufferManager::Write3Bytes(int32_t value)
{
    rs_assert(Require(3));
    char *pp = (char *)&value;
    *ptr_++ = pp[2];
    *ptr_++ = pp[1];
    *ptr_++ = pp[0];
}

void BufferManager::Write4Bytes(int32_t value)
{
    rs_assert(Require(4));
    char *pp = (char *)&value;
    *ptr_++ = pp[3];
    *ptr_++ = pp[2];
    *ptr_++ = pp[1];
    *ptr_++ = pp[0];
}

void BufferManager::Write8Bytes(int64_t value)
{
    rs_assert(Require(8));
    char *pp = (char *)&value;
    *ptr_++ = pp[7];
    *ptr_++ = pp[6];
    *ptr_++ = pp[5];
    *ptr_++ = pp[4];
    *ptr_++ = pp[3];
    *ptr_++ = pp[2];
    *ptr_++ = pp[1];
    *ptr_++ = pp[0];
}

void BufferManager::WriteString(const std::string &value)
{
    rs_assert(Require(value.length()));
    memcpy(ptr_, value.data(), value.length());
    ptr_ += value.length();
}

void BufferManager::WriteBytes(char *data, int32_t size)
{
    rs_assert(Require(size));
    memcpy(ptr_, data, size);
    ptr_ += size;
}

BitBufferManager::BitBufferManager()
{
    cb_ = 0;
    cb_left_ = 0;
    manager_ = nullptr;
}

BitBufferManager::~BitBufferManager()
{
}

int BitBufferManager::Initialize(BufferManager *manager)
{
    manager_ = manager;
    return ERROR_SUCCESS;
}

bool BitBufferManager::Empty()
{
    if (cb_left_)
    {
        return false;
    }

    return manager_->Empty();
}

int8_t BitBufferManager::ReadBit()
{
    if (!cb_left_)
    {
        rs_assert(!manager_->Empty());
        cb_ = manager_->Read1Bytes();
        cb_left_ = 8;
    }

    int8_t v = (cb_ >> (cb_left_ - 1)) & 0x01;
    cb_left_--;
    return v;
}

FastBuffer::FastBuffer() : merged_read_(false),
                           mr_handler_(nullptr)
{
    capacity_ = RS_DEFAULT_RECV_BUFFER_SIZE;
    buf_ = (char *)malloc(capacity_);
    start_ = end_ = buf_;
}

FastBuffer::~FastBuffer()
{
    free(buf_);
    buf_ = nullptr;
}

int32_t FastBuffer::Size()
{
    return (int32_t)(end_ - start_);
}

char *FastBuffer::Bytes()
{
    return start_;
}

void FastBuffer::SetBuffer(int buffer_size)
{
    buffer_size = rs_min(RS_MAX_SOCKER_BUFFER_SIZE, buffer_size);

    rs_trace("user space recv buffer size set to %d", buffer_size);

    if (buffer_size < capacity_)
    {
        return;
    }

    int start_pos = start_ - buf_;
    int size = Size();

    buf_ = (char *)realloc(buf_, buffer_size);
    start_ = buf_ + start_pos;
    end_ = start_ + size;
}

char FastBuffer::Read1Bytes()
{
    rs_assert(Size() >= 1);
    return *start_++;
}

char *FastBuffer::ReadSlice(int size)
{
    rs_assert(size >= 0);
    rs_assert(Size() >= size);
    //avoid start_+size overflow
    rs_assert(start_ + size >= buf_);

    char *ptr = start_;
    start_ += size;
    return ptr;
}

int FastBuffer::Grow(IBufferReader *r, int required_size)
{
    rs_assert(required_size > 0);

    int ret = ERROR_SUCCESS;

    if (Size() >= required_size)
    {
        return ret;
    }

    int free_space = (int)(buf_ + capacity_ - end_);
    int used_space = (int)(end_ - start_);

    if (free_space < required_size - used_space)
    {
        if (!used_space)
        {
            start_ = end_ = buf_;
        }
        else if (used_space < capacity_ && start_ > buf_)
        {
            buf_ = (char *)memmove(buf_, start_, used_space);
            start_ = buf_;
            end_ = start_ + used_space;
        }

        free_space = (int)(buf_ + capacity_ - end_);

        //avoid buffer overflow,which cause function never return
        if (free_space < required_size - used_space)
        {
            ret = ERROR_READER_BUFFER_OVERFLOW;
            rs_error("buffer overflow, required=%d, max=%d, left=%d, ret=%d",
                     required_size, capacity_, free_space, ret);
            return ret;
        }
    }

    while (end_ - start_ < required_size)
    {
        ssize_t nread;
        if ((ret = r->Read(end_, free_space, &nread)) != ERROR_SUCCESS)
        {
            return ret;
        }

        if (merged_read_ && mr_handler_)
        {
            mr_handler_->OnRead(nread);
        }

        rs_assert(int(nread) > 0);
        end_ += nread;
        free_space -= nread;
    }

    return ret;
}

void FastBuffer::SetMergeReadHandler(bool enable, IMergeReadHandler *mr_handler)
{
    merged_read_ = enable;
    mr_handler_ = mr_handler;
}

void FastBuffer::Skip(int size)
{
    //allow skip right,for example:Skip(-4)

    rs_assert(Size() >= size);
    rs_assert(start_ + size >= buf_);

    start_ += size;
}