#ifndef RS_IO_HPP
#define RS_IO_HPP

#include <core.hpp>

/**
+---------------+     +--------------------+      +---------------+
| IBufferReader |     |    IStatistic      |      | IBufferWriter |
+---------------+     +--------------------+      +---------------+
| + read()      |     | + get_recv_bytes() |      | + write()     |
+------+--------+     | + get_recv_bytes() |      | + writev()    |
      / \             +---+--------------+-+      +-------+-------+
       |                 / \            / \              / \
       |                  |              |                |
+------+------------------+-+      +-----+----------------+--+
| IProtocolReader           |      | IProtocolWriter         |
+---------------------------+      +-------------------------+
| + readfully()             |      | + set_send_timeout()    |
| + set_recv_timeout()      |      +-------+-----------------+
+------------+--------------+             / \     
            / \                            |   
             |                             | 
          +--+-----------------------------+-+
          |       IProtocolReaderWriter      |
          +----------------------------------+
          | + is_never_timeout()             |
          +----------------------------------+
*/

class IBufferReader
{
public:
    IBufferReader();
    virtual ~IBufferReader();

public:
    virtual void read(void *buf, size_t size, ssize_t *nread) = 0;
};

class IBufferWriter
{
public:
    IBufferWriter();
    virtual ~IBufferWriter();

public:
    virtual int write(void *buf, size_t size, ssize_t *nwrite) = 0;
    virtual int writev(const iovec *iov, int iov_size, ssize_t *nwrite) = 0;
};

class IStatistic
{
public:
    IStatistic();
    virtual ~IStatistic();

public:
    virtual int64_t get_recv_bytes() = 0;
    virtual int64_t get_write_bytes() = 0;
};

class IProtocolReader : public virtual IBufferReader, public virtual IStatistic
{
public:
    IProtocolReader();
    virtual ~IProtocolReader();

public:
    virtual void set_recv_timeout(int64_t timeout_us) = 0;
    virtual int64_t get_recv_timeout() = 0;

public:
    virtual int read_fully(void *buf, size_t size, ssize_t *nread) = 0;
};

class IProtocolWriter : public virtual IBufferWriter, public virtual IStatistic
{
public:
    IProtocolWriter();
    virtual ~IProtocolWriter();

public:
    virtual void set_send_timeout(int64_t timeout_us) = 0;
    virtual int64_t get_send_timeout() = 0;
};

class IProtocolReaderWriter : public virtual IProtocolReader, public virtual IProtocolWriter
{
public:
    IProtocolReaderWriter();
    virtual ~IProtocolReaderWriter();

public:
    virtual bool is_never_timeout(int64_t timeout_us) = 0;
};
#endif