#ifndef RS_RTMP_EDGE_HPP
#define RS_RTMP_EDGE_HPP

#include <common/core.hpp>
#include <common/thread.hpp>
#include <common/io.hpp>
#include <protocol/rtmp_source.hpp>
#include <protocol/rtmp_stack.hpp>

namespace rtmp
{

enum EdgeState
{
    INIT = 0,
    PLAY = 100,
    INGEST_CONNECTED = 201,
    PUBLISH = 200,
};

class EdgeForwarder : public internal::IThreadHandler
{
public:
    EdgeForwarder();
    virtual ~EdgeForwarder();

public:
    virtual int Initialize();
    //internal::IThreadHandler
    virtual int Cycle() override;

private:
    internal::Thread *thread_;
    IProtocolReaderWriter *rw_;
};

class PublishEdge
{
public:
    PublishEdge();
    virtual ~PublishEdge();

public:
    int Initialize(Source *source, Request *req);

private:
    EdgeState state_;
};

} // namespace rtmp

#endif