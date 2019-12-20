#ifndef RS_RTMP_SERVER_HPP
#define RS_RTMP_SERVER_HPP

class RTMPServer
{
public:
    RTMPServer();
    virtual ~RTMPServer();

public:
    virtual int Initilaize();
    virtual int InitializeST();
};

#endif