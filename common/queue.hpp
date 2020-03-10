#ifndef RS_QUEUE_HPP
#define RS_QUEUE_HPP

#include <common/core.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

#define FAST_VEC_DEFAULT_SIZE 1024
#define MIX_CORRECT_PURE_AV 10

template <typename T>
class FastVector
{
public:
    FastVector(int size = FAST_VEC_DEFAULT_SIZE);
    virtual ~FastVector();

public:
    virtual int Size();
    virtual int Begin();
    virtual int End();
    virtual T *Data();
    virtual T At(int index);
    virtual void Clear();
    virtual void Free();
    virtual void Erase(int begin, int end);
    virtual void PushBack(T msg);

private:
    T *msgs_;
    int nb_msgs_;
    int count_;
};

template <typename T>
FastVector<T>::FastVector(int size)
{
    msgs_ = new T[size];
    nb_msgs_ = FAST_VEC_DEFAULT_SIZE;
    count_ = 0;
}

template <typename T>
FastVector<T>::~FastVector<T>()
{
    Free();
    rs_freepa(msgs_);
}

template <typename T>
int FastVector<T>::Size()
{
    return count_;
}

template <typename T>
int FastVector<T>::Begin()
{
    return 0;
}

template <typename T>
int FastVector<T>::End()
{
    return count_;
}

template <typename T>
T *FastVector<T>::Data()
{
    return msgs_;
}

template <typename T>
T FastVector<T>::At(int index)
{
    return msgs_[index];
}

template <typename T>
void FastVector<T>::Free()
{
    for (int i = 0; i < count_; i++)
    {
        T msg = msgs_[i];
        rs_freep(msg);
    }
    count_ = 0;
}

template <typename T>
void FastVector<T>::Clear()
{
    count_ = 0;
}

template <typename T>
void FastVector<T>::Erase(int begin, int end)
{
    for (int i = begin; i < end; i++)
    {
        T msg = msgs_[i];
        rs_freep(msg);
    }

    for (int i = 0; i < count_ - end; i++)
    {
        msgs_[begin + i] = msgs_[end + i];
    }

    count_ -= (end - begin);
}

template <typename T>
void FastVector<T>::PushBack(T msg)
{
    if (count_ >= nb_msgs_)
    {
        int size = nb_msgs_ * 2;

        T *buf = new T[size];
        for (int i = 0; i < nb_msgs_; i++)
        {
            buf[i] = msgs_[i];
        }

        rs_warn("fast vector incrase %d=>%d", nb_msgs_, size);
        rs_freepa(msgs_);
        msgs_ = buf;
        nb_msgs_ = size;
    }
    msgs_[count_++] = msg;
}

template <typename T>
class MixQueue
{
public:
    MixQueue();
    virtual ~MixQueue();

public:
    virtual void Clear();
    virtual void Push(T *msg);
    virtual T *Pop();

private:
    uint32_t nb_videos_;
    uint32_t nb_audios_;
    std::multimap<int64_t, T *> msgs_;
};

template <typename T>
MixQueue<T>::MixQueue()
{
    nb_videos_ = 0;
    nb_audios_ = 0;
}

template <typename T>
MixQueue<T>::~MixQueue()
{
    Clear();
}

template <typename T>
void MixQueue<T>::Clear()
{
    typename std::multimap<int64_t, T *>::iterator it;
    for (it = msgs_.begin(); it != msgs_.end(); it++)
    {
        T *msg = it->second;
        rs_freep(msg);
    }

    msgs_.clear();
    nb_videos_ = 0;
    nb_audios_ = 0;
}

template <typename T>
void MixQueue<T>::Push(T *msg)
{
    if (msg->IsVideo())
    {
        nb_videos_++;
    }
    else
    {
        nb_audios_++;
    }

    msgs_.insert(std::make_pair(msg->timestamp, msg));
}

template <typename T>
T *MixQueue<T>::Pop()
{
    bool mix_ok = false;

    if (nb_videos_ >= MIX_CORRECT_PURE_AV && nb_audios_ == 0)
    {
        mix_ok = true;
    }
    if (nb_audios_ >= MIX_CORRECT_PURE_AV && nb_videos_ == 0)
    {
        mix_ok = true;
    }
    if (nb_videos_ > 0 && nb_audios_ > 0)
    {
        mix_ok = true;
    }

    if (!mix_ok)
    {
        return nullptr;
    }

    typename std::multimap<int64_t, T *>::iterator it = msgs_.begin();
    T *msg = it->second;
    msgs_.erase(it);

    if (msg->IsVideo())
    {
        nb_videos_--;
    }
    else
    {
        nb_audios_--;
    }

    return msg;
}

#endif