#ifndef RS_RTMP_CONNECTION_HPP
#define RS_RTMP_CONNECTION_HPP

#include <common/core.hpp>
#include <common/socket.hpp>
#include <common/connection.hpp>
#include <app/rtmp_server.hpp>
#include <app/server.hpp>

class RTMPConnection : virtual public Connection
{
public:
    RTMPConnection(Server *server, st_netfd_t stfd);
    virtual ~RTMPConnection();

public:
    //IKbpsDelta
    virtual void Resample() override;
    virtual int64_t GetSendBytesDelta() override;
    virtual int64_t GetRecvBytesDelta() override;
    virtual void CleanUp() override;

protected:
    virtual int32_t StreamServiceCycle();
    virtual int32_t ServiceCycle();
    //Connection
    virtual int32_t DoCycle() override;

private:
    StSocket *socket_;
    RTMPServer *rtmp_;
    rtmp::Request *request_;
};

#endif