#include <servers/rtmp_server.hpp>
#include <common/log/log.hpp>
#include <common/error.hpp>
#include <common/io/st.hpp>

RTMPServer::RTMPServer()
{
}

RTMPServer::~RTMPServer()
{
}

int RTMPServer::InitializeST()
{
    int ret = ERROR_SUCCESS;

    if ((ret = STInit()) != ERROR_SUCCESS)
    {
        rs_error("STInit failed,ret=%d", ret);
        return ret;
    }

    _context->GenerateID();

    rs_trace("rtmp server main cid=%d,pid=%d", _context->GetID(), ::getpid());

    return ret;
}

int RTMPServer::Initilaize()
{
    return 0;
}