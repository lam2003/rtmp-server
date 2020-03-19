#include <common/error.hpp>
#include <common/log.hpp>
#include <common/socket.hpp>
#include <common/utils.hpp>

StSocket::StSocket(st_netfd_t stfd)
    : stfd_(stfd), send_timeout_(ST_UTIME_NO_TIMEOUT),
      recv_timeout_(ST_UTIME_NO_TIMEOUT), send_bytes_(0), recv_bytes_(0)
{
}
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

int32_t StSocket::Read(void* buf, size_t size, ssize_t* nread)
{
    ssize_t nb_read = st_read(stfd_, buf, size, recv_timeout_);

    if (nread) {
        *nread = nb_read;
    }
    if (nb_read <= 0) {
        if (nb_read < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        else if (nb_read == 0) {
            errno = ECONNRESET;
        }
        return ERROR_SOCKET_READ;
    }

    recv_bytes_ += nb_read;
    return ERROR_SUCCESS;
}

int32_t StSocket::ReadFully(void* buf, size_t size, ssize_t* nread)
{
    ssize_t nb_read = st_read_fully(stfd_, buf, size, recv_timeout_);

    if (nread) {
        *nread = nb_read;
    }
    if (nb_read != (ssize_t)size) {
        if (nb_read < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        else if (nb_read >= 0) {
            errno = ECONNRESET;
        }
        return ERROR_SOCKET_READ;
    }
    recv_bytes_ += nb_read;
    return ERROR_SUCCESS;
}

int32_t StSocket::Write(void* buf, size_t size, ssize_t* nwrite)
{
    ssize_t nb_write = st_write(stfd_, buf, size, send_timeout_);
    if (nwrite) {
        *nwrite = nb_write;
    }
    if (nb_write <= 0) {
        if (nb_write < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        return ERROR_SOCKET_WRITE;
    }

    send_bytes_ += nb_write;

    return ERROR_SUCCESS;
}

int32_t StSocket::WriteEv(const iovec* iov, int32_t iov_size, ssize_t* nwrite)
{
    ssize_t nb_write = st_writev(stfd_, iov, iov_size, send_timeout_);

    if (nwrite) {
        *nwrite = nb_write;
    }
    if (nb_write <= 0) {
        if (nb_write < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        return ERROR_SOCKET_WRITE;
    }

    send_bytes_ += nb_write;

    return ERROR_SUCCESS;
}

int send_large_iovs(IProtocolReaderWriter* rw,
                    iovec*                 iovs,
                    int                    size,
                    ssize_t*               pnwrite)
{
    int ret = ERROR_SUCCESS;

    static int limits = 1024;

    if (size < limits) {
        if ((ret = rw->WriteEv(iovs, size, pnwrite)) != ERROR_SUCCESS) {
            if (!IsClientGracefullyClose(ret)) {
                rs_error("send with writev failed. ret=%d", ret);
            }
            return ret;
        }
        return ret;
    }

    ssize_t nwrite  = 0;
    int     cur_pos = 0;
    while (cur_pos < size) {
        int nb_send = rs_min(limits, size - cur_pos);

        if ((ret = rw->WriteEv(iovs + cur_pos, nb_send, &nwrite)) !=
            ERROR_SUCCESS) {
            if (!IsClientGracefullyClose(ret)) {
                rs_error("send with writev failed. ret=%d", ret);
            }
            return ret;
        }

        cur_pos += nb_send;
        if (pnwrite) {
            *pnwrite += nwrite;
        }
    }

    return ret;
}