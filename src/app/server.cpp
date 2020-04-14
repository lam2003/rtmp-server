/*
 * @Author: linmin
 * @LastEditTime: 2020-03-17 18:46:42
 * @LastEditors: linmin
 */
#include <app/server.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/st.hpp>
#include <common/utils.hpp>
#include <protocol/rtmp/connection.hpp>

#include <algorithm>

#include <fcntl.h>
#include <unistd.h>

IServerListener::IServerListener(StreamServer* server, ListenerType type)
    : server_(server), type_(type), ip_(""), port_(-1)
{
}

IServerListener::~IServerListener() {}

ListenerType IServerListener::GetType()
{
    return type_;
}

RTMPStreamListener::RTMPStreamListener(StreamServer* server, ListenerType type)
    : IServerListener(server, type), listener_(nullptr)
{
}

RTMPStreamListener::~RTMPStreamListener()
{
    rs_freep(listener_);
}

int32_t RTMPStreamListener::Listen(const std::string& ip, int32_t port)
{
    int32_t ret = ERROR_SUCCESS;

    ip_   = ip;
    port_ = port;

    rs_freep(listener_);
    listener_ = new TCPListener(this, ip, port);

    if ((ret = listener_->Listen()) != ERROR_SUCCESS) {
        rs_error("tcp listen failed,ep=[%s:%d],ret=%d", ip.c_str(), port, ret);
        return ret;
    }

    rs_info("RTMP streamer listen on [%s:%d]", ip.c_str(), port);

    return ret;
}

int32_t RTMPStreamListener::OnTCPClient(st_netfd_t stfd)
{
    int ret = ERROR_SUCCESS;
    if ((ret = server_->AcceptClient(ListenerType::RTMP, stfd)) !=
        ERROR_SUCCESS) {
        rs_error("accpet client failed,ret=%d", ret);
        return ret;
    }
    return ret;
}

StreamServer::StreamServer() {}

StreamServer::~StreamServer() {}

int32_t StreamServer::InitializeST()
{
    int32_t ret = ERROR_SUCCESS;

    if ((ret = STInit()) != ERROR_SUCCESS) {
        rs_error("STInit failed,ret=%d", ret);
        return ret;
    }

    _context->GenerateID();

    rs_verbose("rtmp server main cid=%d,pid=%d", _context->GetID(), ::getpid());

    return ret;
}

int32_t StreamServer::Initilaize()
{
    return 0;
}

int32_t StreamServer::AcceptClient(ListenerType type, st_netfd_t stfd)
{
    int32_t ret = ERROR_SUCCESS;

    int32_t fd = st_netfd_fileno(stfd);

    int32_t val;
    if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
        ret = ERROR_SYSTEM_PID_GET_FILE_INFO;
        rs_error("fcntl F_GETFD failed,fd=%d,ret=%d", fd, ret);
        return ret;
    }
    val |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, val) < 0) {
        ret = ERROR_SYSTEM_PID_SET_FILE_INFO;
        rs_error("fcntl F_SETFD failed,fd=%d,ret=%d", fd, ret);
        return ret;
    }

    IConnection* conn = nullptr;
    if (type == ListenerType::RTMP) {
        conn = new rtmp::Connection(this, stfd);
    }

    conns_.push_back(conn);

    if ((ret = conn->Start()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int32_t StreamServer::Listen()
{
    int ret = ERROR_SUCCESS;

    if ((ret = listen_rtmp()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int32_t StreamServer::listen_rtmp()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

void StreamServer::OnRemove(IConnection* conn)
{
    std::vector<IConnection*>::iterator it =
        std::find(conns_.begin(), conns_.end(), conn);

    if (it == conns_.end()) {
        rs_warn("connection has been removed. ignore it");
        return;
    }

    conns_.erase(it);

    rs_info("connection removed. now total=%d", conns_.size());

    rs_freep(conn);
}

int StreamServer::OnPublish(rtmp::Source* s, rtmp::Request* r)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int StreamServer::OnUnPublish(rtmp::Source* s, rtmp::Request* r)
{
    int ret = ERROR_SUCCESS;
    return ret;
}