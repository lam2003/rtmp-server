#include <app/connection.hpp>
#include <common/utils/utils.hpp>
#include <common/io/st.hpp>
#include <common/error.hpp>
#include <common/log/log.hpp>



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

int Connection::Start()
{
    return thread_->Start();
}

int Connection::Cycle()
{
    int ret = ERROR_SUCCESS;
    id_ = _context->GetID();

    // ip_ = 
}