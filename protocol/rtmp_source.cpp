#include <protocol/rtmp_source.hpp>
#include <common/config.hpp>

namespace rtmp
{
ISourceHandler::ISourceHandler()
{
}

ISourceHandler::~ISourceHandler()
{
}

std::map<std::string, Source *> Source::pool_;

Source::Source() : request_(nullptr)
{
}

Source::~Source()
{
    rs_freep(request_);
}

int Source::FetchOrCreate(rtmp::Request *r, ISourceHandler *h, Source **pps)
{
    int ret = ERROR_SUCCESS;

    Source *source = nullptr;

    if ((source = Fetch(r)) != nullptr)
    {
        *pps = source;
        return ret;
    }

    std::string stream_url = r->GetStreamUrl();

    std::string vhost = r->vhost;

    source = new Source;

    if ((ret = source->Initialize(r, h)) != ERROR_SUCCESS)
    {
        rs_freep(source);
        return ret;
    }

    pool_[stream_url] = source;
    *pps = source;

    rs_info("create new source for url=%s,vhost=%s", stream_url.c_str(), vhost.c_str());

    return ret;
}

Source *Source::Fetch(rtmp::Request *r)
{
    Source *source = nullptr;

    std::string stream_url = r->GetStreamUrl();

    if (pool_.find(stream_url) == pool_.end())
    {
        return nullptr;
    }

    source = pool_[stream_url];

    source->request_->Update(r);

    return source;
}

int Source::Initialize(rtmp::Request *r, ISourceHandler *h)
{
    int ret = ERROR_SUCCESS;

    handler_ = h;

    request_ = r->Copy();

    atc_ = _config->GetATC(r->vhost);

    return ret;
}
} // namespace rtmp