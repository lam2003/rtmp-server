/*
 * @Author: linmin
 * @Date: 2020-02-06 19:12:24
 * @LastEditTime : 2020-02-08 17:11:57
 */
#include <common/core.hpp>
#include <common/file.hpp>
#include <protocol/rtmp_stack.hpp>

#define FLV_TAG_HEADER_SIZE 11
#define FLV_PREVIOUS_TAG_SIZE 4

namespace flv
{
enum TagType
{
    AUDIO = 8,
    VIDEO = 9,
    SCRIPT = 18,
    UNKNOW = 0
};

class Encoder
{
public:
    Encoder();
    virtual ~Encoder();

public:
    static int SizeTag(int data_size);

    virtual int Initialize(FileWriter *writer);
    virtual int WriteFlvHeader();
    virtual int WriteFlvHeader(char flv_header[9]);
    virtual int WriteMetadata(char *data, int size);
    virtual int WriteAudio(int64_t timestamp, char *data, int size);
    virtual int WriteVideo(int64_t timestamp, char *data, int size);
    virtual int WriteTags(rtmp::SharedPtrMessage **msgs, int count);

private:
    int write_tag_header_to_cache(char type, int size, int timestamp, char *cache);
    int write_previous_tag_size_to_cache(int size, char *cache);
    int write_tag(char *header, int header_size, char *tag, int tag_size);

private:
    FileWriter *writer_;
    int nb_tag_headers_;
    char *tag_headers_;
    int nb_ppts_;
    char *ppts_;
    int nb_iovss_cache_;
    iovec *iovss_cache_;
};
}; // namespace flv