#include <common/io/socket.hpp>

#include <common/error.hpp>

StSocket::StSocket(st_netfd_t stfd) : stfd_(stfd),
                                      send_timeout_(ST_UTIME_NO_TIMEOUT),
                                      recv_timeout_(ST_UTIME_NO_TIMEOUT),
                                      send_bytes_(0),
                                      recv_bytes_(0) {}
StSocket::~StSocket() {}

bool StSocket::IsNeverTimeout(int64_t timeout_us)
{
    return timeout_us == (int64_t)ST_UTIME_NO_TIMEOUT;
}

void StSocket::SetRecvTimeout(int64_t timeout_us)
{
    recv_timeout_ = timeout_us;
}

int64_t StSocket::GetRecvTimeout()
{
    return recv_timeout_;
}

void StSocket::SetSendTimeout(int64_t timeout_us)
{
    send_timeout_ = timeout_us;
}

int64_t StSocket::GetSendTimeout()
{
    return send_timeout_;
}

int64_t StSocket::GetSendBytes()
{
    return send_bytes_;
}

int64_t StSocket::GetRecvBytes()
{
    return recv_bytes_;
}

int StSocket::Read(void *buf, size_t size, ssize_t *nread)
{
    ssize_t nb_read = st_read(stfd_, buf, size, recv_timeout_);
    *nread = nb_read;

    if (nb_read <= 0)
    {
        if (nb_read < 0 && errno == ETIME)
        {
            return ERROR_SOCKET_TIMEOUT;
        }
        else if (nb_read == 0)
        {
            errno = ECONNRESET;
        }
        return ERROR_SOCKET_READ;
    }

    recv_bytes_ += nb_read;
    return ERROR_SUCCESS;
}

int StSocket::ReadFully(void *buf, size_t size, ssize_t *nread)
{
    ssize_t nb_read = st_read_fully(stfd_, buf, size, recv_timeout_);
    *nread = nb_read;

    if (nb_read != size)
    {
        if (nb_read < 0 && errno == ETIME)
        {
            return ERROR_SOCKET_TIMEOUT;
        }
        else if (nb_read >= 0)
        {
            errno = ECONNRESET;
        }
        return ERROR_SOCKET_READ;
    }
    recv_bytes_ += nb_read;
    return ERROR_SUCCESS;
}

int StSocket::Write(void *buf, size_t size, ssize_t *nwrite)
{
    ssize_t nb_write = st_write(stfd_, buf, size, send_timeout_);
    *nwrite = nb_write;

    if (nb_write <= 0)
    {
        if (nb_write < 0 && errno == ETIME)
        {
            return ERROR_SOCKET_TIMEOUT;
        }
        return ERROR_SOCKET_WRITE;
    }

    send_bytes_ += nb_write;

    return ERROR_SUCCESS;
}

int StSocket::WriteEv(const iovec *iov, int iov_size, ssize_t *nwrite)
{
    ssize_t nb_write = st_writev(stfd_, iov, iov_size, send_timeout_);
    *nwrite = nb_write;

    if (nb_write <= 0)
    {
        if (nb_write < 0 && errno == ETIME)
        {
            return ERROR_SOCKET_TIMEOUT;
        }
        return ERROR_SOCKET_WRITE;
    }

    send_bytes_ += nb_write;

    return ERROR_SUCCESS;
}