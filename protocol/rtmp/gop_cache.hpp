/*
 * @Date: 2020-03-18 11:13:26
 * @LastEditors: linmin
 * @LastEditTime: 2020-03-18 13:29:54
 */
#ifndef RS_GOP_CACHE_HPP
#define RS_GOP_CACHE_HPP

#include <common/core.hpp>

#include <vector>

namespace rtmp {

enum class JitterAlgorithm;
class SharedPtrMessage;
class Consumer;

class GopCache {
  public:
    GopCache();
    virtual ~GopCache();

  public:
    virtual void Dispose();
    virtual void Set(bool enabled);
    virtual int  Cache(SharedPtrMessage* shared_msg);
    virtual void Clear();
    virtual int  Dump(Consumer* consumer, bool atc, JitterAlgorithm jitter_ag);
    virtual bool Empty();
    virtual int64_t StartTime();
    virtual bool    PureAudio();

  private:
    int                            cached_video_count_;
    bool                           enable_gop_cache_;
    int                            audio_after_last_video_count_;
    std::vector<SharedPtrMessage*> queue_;
};
}  // namespace rtmp

#endif