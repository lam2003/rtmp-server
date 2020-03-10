#ifndef RS_AVC_HPP
#define RS_AVC_HPP

#include <common/core.hpp>
#include <codec/codec.hpp>
namespace avc
{
enum class Profile
{
    UNKNOW = 0,
    BASELINE = 66,
    CONSTRAINED_BASELINE = 578,
    MAIN = 77,
    EXTENDED = 88,
    HIGH = 100,
    HIGH_10 = 110,
    HIGH_10_INTRA = 2158,
    HIGH_422 = 122,
    HIGH_422_INTRA = 2170,
    HIGH_444 = 144,
    HIGH_444_PREDICTIVE = 244,
    HIGH_444_INTRA = 2192
};

enum class Level
{
    UNKNOW = 0,
    LEVEL_1 = 10,
    LEVEL_11 = 11,
    LEVEL_12 = 12,
    LEVEL_13 = 13,
    LEVEL_2 = 20,
    LEVEL_21 = 21,
    LEVEL_22 = 22,
    LEVEL_3 = 30,
    LEVEL_31 = 31,
    LEVEL_32 = 32,
    LEVEL_4 = 40,
    LEVEL_41 = 41,
    LEVEL_42 = 42,
    LEVEL_5 = 50,
    LEVEL_51 = 51,
    LEVEL_52 = 52
};

enum class PayloadFormat
{
    GUESS = 0,
    ANNEXB = 1,
    IBMF = 2
};

enum class NaluType
{
    UNKNOW = 0,
    NON_IDR = 1,
    DATA_PARTITION_A = 2,
    DATA_PARTITION_B = 3,
    DATA_PARTITION_C = 4,
    IDR = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8,
    ACCESS_UNIT_DELIMITER = 9,
    EOSEQUENCE = 10,
    EOSTREAM = 11,
    FILTER_DATA = 12,
    SPS_EXT = 13,
    PERFIX_NALU = 14,
    SUBSET_SPS = 15,
    LAYER_WITHOUT_PARTITION = 19,
    CODECD_SLICE_EXT = 20
};

extern std::string profile_to_str(Profile profile);
extern std::string level_to_str(Level level);
extern int read_bit(BitBufferManager *manager, int8_t &v);
extern int read_uev(BitBufferManager *manager, int32_t &v);

class Codec : public IVCodec
{
public:
    Codec();
    virtual ~Codec();

public:
    virtual bool HasSequenceHeader() override;
    virtual int DecodeSequenceHeader(BufferManager *manager) override;
    virtual int DecodecNalu(BufferManager *manager, ICodecSample *sample) override;

private:
    int demux_sps_rbsp(char *rbsp, int nb_rbsp);
    int demux_sps();
    int demux_annexb_format(BufferManager *manager, ICodecSample *sample);
    int demux_ibmf_format(BufferManager *manager, ICodecSample *sample);
    bool start_with_annexb(BufferManager *manager, int *pnb_start_code);

public:
    avc::Profile profile;
    avc::Level level;
    avc::PayloadFormat payload_format;
    int extradata_size;
    char *extradata;
    uint16_t sps_length;
    char *sps;
    uint16_t pps_length;
    char *pps;
    int width;
    int height;
    int8_t length_size_minus_one;
};

} // namespace avc

#endif