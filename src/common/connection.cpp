/*
 * @Date: 2020-02-24 11:23:35
 * @LastEditors: linmin
 * @LastEditTime: 2020-03-17 18:47:38
 */
#include <common/connection.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/st.hpp>
#include <common/utils.hpp>

IConnectionManager::IConnectionManager() {}

IConnectionManager::~IConnectionManager() {}

IConnection::IConnection(IConnectionManager* conn_manager,
                         st_netfd_t          client_stfd)
    : conn_manager_(conn_manager), client_stfd_(client_stfd), client_ip_(""),
      disposed_(false), expired_(false), id_(-1)
{
    thread_ = new internal::Thread("connection", this, 0, false);
}

IConnection::~IConnection()
{
    Dispose();
    rs_freep(thread_);
}

void IConnection::Dispose()
{
    if (disposed_) {
        return;
    }

    disposed_ = true;
    STCloseFd(client_stfd_);
}

int32_t IConnection::Start()
{
    return thread_->Start();
}

int32_t IConnection::Cycle()
{
    int32_t ret = ERROR_SUCCESS;
    id_         = _context->GetID();

    client_ip_ = Utils::GetPeerIP(st_netfd_fileno(client_stfd_));

    ret = do_cycle();

    if (is_client_gracefully_close(ret)) {
        ret = ERROR_SOCKET_CLOSED;
        rs_warn("client %s disconnect peer,ret=%d", client_ip_.c_str(), ret);
    }

    if (ret == ERROR_SUCCESS) {
        rs_info("client %s finish", client_ip_.c_str());
    }

    // break cycle
    thread_->StopLoop();
    // whatevet return ok
    return ERROR_SUCCESS;
}

int32_t IConnection::GetID()
{
    return id_;
}

void IConnection::SetExpired(bool expired)
{
    expired_ = expired;
}

void IConnection::OnThreadStop()
{
    conn_manager_->OnRemove(this);
}
