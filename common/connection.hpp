#ifndef RS_CONNECTION_HPP
#define RS_CONNECTION_HPP

#include <common/core.hpp>
#include <common/thread.hpp>
#include <common/kbps.hpp>

#include <string>

class IConnection;

class IConnectionManager
{
public:
    IConnectionManager();
    virtual ~IConnectionManager();

public:
    virtual void OnRemove(IConnection *conn) = 0;
};

class IConnection : public virtual IKbpsDelta,
                   public virtual internal::IThreadHandler
{
public:
    IConnection(IConnectionManager *conn_manager, st_netfd_t client_stfd);
    virtual ~IConnection();

public:
    virtual void Dispose();
    virtual int32_t Start();
    virtual int32_t Cycle() override;
    virtual void OnThreadStop() override;
    virtual int32_t GetID();
    virtual void SetExpired(bool expired = true);

protected:
    virtual int32_t DoCycle() = 0;

protected:
    IConnectionManager *conn_manager_;
    st_netfd_t client_stfd_;
    std::string client_ip_;
    bool disposed_;
    bool expired_;

private:
    int32_t id_;
    internal::Thread *thread_;
};
#endif
