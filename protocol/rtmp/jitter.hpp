/*
 * @Author: linmin
 * @Date: 2020-03-10 10:29:49
 * @LastEditTime: 2020-03-10 16:39:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \rtmp_server\protocol\rtmp\jitter.hpp
 */
#ifndef RS_RTMP_JITTER_HPP
#define RS_RTMP_JITTER_HPP

#include <common/core.hpp>

namespace rtmp
{

class SharedPtrMessage;

enum class JitterAlgorithm
{
    FULL = 1,
    ZERO,
    OFF
};

class Jitter
{
public:
    Jitter();
    virtual ~Jitter();

public:
    virtual int Correct(SharedPtrMessage *msg, JitterAlgorithm ag);
    virtual int GetTime();

private:
    int64_t last_pkt_time_;
    int64_t last_pkt_correct_time_;
};

} // namespace rtmp

#endif