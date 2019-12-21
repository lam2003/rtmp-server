#include <common/buffer.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

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
