#ifndef RS_AAC_HPP
#define RS_AAC_HPP

#include <common/core.hpp>
#include <codec/codec.hpp>

#include <string>

namespace aac
{
enum class ObjectType
{
    UNKNOW = 0,
    MAIN = 1,
    LC = 2,
    SSR = 3,
    HE = 4,
    HEV2 = 5
};

extern std::string object_type_to_str(aac::ObjectType object_type);
extern std::string sample_rate_to_str(int sample_rate_idx);
extern std::string channel_to_str(int channel);

} // namespace aac

class AACCodec : public ACodec
{
public:
    AACCodec();
    virtual ~AACCodec();

public:
    virtual bool HasSequenceHeader() override;
    virtual int DecodeSequenceHeader() override;
    virtual int DecodeRawData(BufferManager *manager) override;

public:
    int extradata_size;
    char *extradata;
    uint8_t channels;
    uint8_t sample_rate;
    aac::ObjectType object_type;
};

#endif