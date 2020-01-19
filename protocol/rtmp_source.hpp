#ifndef RS_RTMP_SOURCE_HPP
#define RS_RTMP_SOURCE_HPP

#include <common/core.hpp>
#include <protocol/rtmp_stack.hpp>

namespace rtmp
{

class Source;

class ISourceHandler
{
public:
    ISourceHandler();
    virtual ~ISourceHandler();

public:
    virtual int OnPublish(Source *s, Request *r) = 0;
    virtual int OnUnPublish(Source *s, Request *r) = 0;
};

class Source
{
public:
    Source();
    virtual ~Source();

public:
    static int FetchOrCreate(Request *r, ISourceHandler *h, Source **pps);
    int Initialize(Request *r, ISourceHandler *h);

protected:
    static Source *Fetch(Request *r);

private:
    static std::map<std::string, Source *> pool_;
    Request *request_;
    bool atc_;
    ISourceHandler *handler_;
};
} // namespace rtmp
#endif
