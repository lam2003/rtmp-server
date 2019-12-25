#include <common/connection.hpp>
#include <common/utils.hpp>
#include <common/st.hpp>
#include <common/error.hpp>
#include <common/log.hpp>

IConnectionManager::IConnectionManager()
{
}

IConnectionManager::~IConnectionManager()
{
}

Connection::Connection(IConnectionManager *conn_manager, st_netfd_t client_stfd) : conn_manager_(conn_manager),
                                                                                   client_stfd_(client_stfd),
                                                                                   client_ip_(""),
                                                                                   disposed_(false),
                                                                                   expired_(false),
                                                                                   id_(-1)
{
    thread_ = new internal::Thread("connection", this, 0, false);
}

Connection::~Connection()
{
    Dispose();
    rs_freep(thread_);
}

void Connection::Dispose()
{
    if (disposed_)
    {
        return;
    }

    disposed_ = true;
    STCloseFd(client_stfd_);
}

int32_t Connection::Start()
{
    return thread_->Start();
}

int32_t Connection::Cycle()
{
    int32_t ret = ERROR_SUCCESS;
    id_ = _context->GetID();

    client_ip_ = Utils::GetPeerIP(st_netfd_fileno(client_stfd_));

    ret = DoCycle();

    if (IsClientGracefullyClose(ret))
    {
        ret = ERROR_SOCKET_CLOSED;
        rs_warn("client %s disconnect peer,ret=%d", client_ip_.c_str(), ret);
    }

    if (ret == ERROR_SUCCESS)
    {
        rs_info("client %s finish", client_ip_.c_str());
    }

    //break cycle
    thread_->StopLoop();
    //whatevet return ok
    return ERROR_SUCCESS;
}

int32_t Connection::GetID()
{
    return id_;
}

void Connection::SetExpired(bool expired)
{
    expired_ = expired;
}

void Connection::OnThreadStop()
{
    conn_manager_->OnRemove(this);
}
