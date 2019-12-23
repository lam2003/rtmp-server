#include <common/buffer.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

//default recv buffer size 128KB
#define RS_DEFAULT_RECV_BUFFER_SIZE 131072
//socket max buffer size 256KB
#define RS_MAX_SOCKER_BUFFER_SIZE 262144

Buffer::Buffer() : bytes_(nullptr),
                   pos_(nullptr),
                   nb_bytes_(0)
{
}

Buffer::~Buffer()
{
}

int Buffer::Initialize(char *b, int32_t nb)
{
    int ret = ERROR_SUCCESS;
    if (!b)
    {
        ret = ERROR_KERNEL_STREAM_INIT;
        rs_error("buffer initialize with null b,ret=%d", ret);
        return ret;
    }

    if (nb <= 0)
    {
        ret = ERROR_KERNEL_STREAM_INIT;
        rs_error("buffer initialize with nb <= 0,ret=%d", ret);
        return ret;
    }

    bytes_ = pos_ = b;
    nb_bytes_ = nb;
    return ret;
}

char *Buffer::Data()
{
    return bytes_;
}

int32_t Buffer::Size()
{
    return nb_bytes_;
}

int32_t Buffer::Pos()
{
    return int32_t(pos_ - bytes_);
}

bool Buffer::Empty()
{
    return !bytes_ || (pos_ >= bytes_ + nb_bytes_);
}

bool Buffer::Require(int32_t required_size)
{
    rs_assert(required_size >= 0);
    return required_size <= nb_bytes_ - (pos_ - bytes_);
}

void Buffer::Skip(int32_t size)
{
    rs_assert(pos_);
    pos_ += size;
}

int8_t Buffer::Read1Bytes()
{
    rs_assert(Require(1));
    return (int8_t)*pos_++;
}

int16_t Buffer::Read2Bytes()
{
    rs_assert(Require(2));
    int16_t value;
    char *pp = (char *)&value;

    pp[1] = *pos_++;
    pp[0] = *pos_++;

    return value;
}

int32_t Buffer::Read3Bytes()
{
    rs_assert(Require(3));
    int32_t value = 0;
    char *pp = (char *)&value;

    pp[2] = *pos_++;
    pp[1] = *pos_++;
    pp[0] = *pos_++;

    return value;
}

int32_t Buffer::Read4Bytes()
{
    rs_assert(Require(4));
    int32_t value;
    char *pp = (char *)&value;

    pp[3] = *pos_++;
    pp[2] = *pos_++;
    pp[1] = *pos_++;
    pp[0] = *pos_++;

    return value;
}

int64_t Buffer::Read8Bytes()
{
    rs_assert(Require(8));
    int64_t value;
    char *pp = (char *)&value;

    pp[7] = *pos_++;
    pp[6] = *pos_++;
    pp[5] = *pos_++;
    pp[4] = *pos_++;
    pp[3] = *pos_++;
    pp[2] = *pos_++;
    pp[1] = *pos_++;
    pp[0] = *pos_++;

    return value;
}

std::string Buffer::ReadString(int32_t len)
{
    rs_assert(Require(len));
    std::string value;
    value.append(pos_, len);

    pos_ += len;

    return value;
}

void Buffer::ReadBytes(char *data, int32_t size)
{
    rs_assert(Require(size));
    memcpy(data, pos_, size);
    pos_ += size;
}

void Buffer::Write1Bytes(int8_t value)
{
    rs_assert(Require(1));
    *pos_++ = value;
}

void Buffer::Write2Bytes(int16_t value)
{
    rs_assert(Require(2));
    char *pp = (char *)&value;
    *pos_++ = pp[1];
    *pos_++ = pp[0];
}

void Buffer::Write3Bytes(int32_t value)
{
    rs_assert(Require(3));
    char *pp = (char *)&value;
    *pos_++ = pp[2];
    *pos_++ = pp[1];
    *pos_++ = pp[0];
}

void Buffer::Write4Bytes(int32_t value)
{
    rs_assert(Require(4));
    char *pp = (char *)&value;
    *pos_++ = pp[3];
    *pos_++ = pp[2];
    *pos_++ = pp[1];
    *pos_++ = pp[0];
}

void Buffer::Write8Bytes(int64_t value)
{
    rs_assert(Require(8));
    char *pp = (char *)&value;
    *pos_++ = pp[7];
    *pos_++ = pp[6];
    *pos_++ = pp[5];
    *pos_++ = pp[4];
    *pos_++ = pp[3];
    *pos_++ = pp[2];
    *pos_++ = pp[1];
    *pos_++ = pp[0];
}

void Buffer::WriteString(const std::string &value)
{
    rs_assert(Require(value.length()));
    memcpy(pos_, value.data(), value.length());
    pos_ += value.length();
}

void Buffer::WriteBytes(char *data, int32_t size)
{
    rs_assert(Require(size));
    memcpy(pos_, data, size);
    pos_ += size;
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
    if (buffer_size > RS_MAX_SOCKER_BUFFER_SIZE)
    {
        rs_warn("limit user space buffer from %d to %d", buffer_size, RS_MAX_SOCKER_BUFFER_SIZE);
    }

    buffer_size = rs_min(RS_MAX_SOCKER_BUFFER_SIZE, buffer_size);

    if (buffer_size < capacity_)
    {
        rs_warn("only realloc when buffer changed bigger");
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
        rs_verbose("move fast buffer %d bytes", used_space);
        if (!used_space)
        {
            start_ = end_ = buf_;
            rs_verbose("all consumed,reset fast buffer");
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
            rs_error("buffer overflow,required=%d,max=%d,left=%d,ret=%d",
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
    rs_assert(Size() >= size);
    rs_assert(start_ + size >= buf_);

    start_ += size;
}