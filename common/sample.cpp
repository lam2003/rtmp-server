#include <common/sample.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

CodecSampleUnit::CodecSampleUnit()
{
    size = 0;
    bytes = nullptr;
}

CodecSampleUnit::~CodecSampleUnit()
{
}

CodecSample::CodecSample()
{
    nb_sample_units = 0;
}

CodecSample::~CodecSample()
{
}

void CodecSample::Clear()
{
    nb_sample_units = 0;
}

int CodecSample::AddSampleUnit(char *bytes, int size)
{
    int ret = ERROR_SUCCESS;

    if (nb_sample_units >= MAX_CODEC_SAMPLE)
    {
        ret = ERROR_SAMPLE_EXCEED;
        rs_error("codec sample exceed the max count: %d. ret=%d", MAX_CODEC_SAMPLE, ret);
        return ret;
    }

    CodecSampleUnit *sample_unit = &sample_units[nb_sample_units++];
    sample_unit->bytes = bytes;
    sample_unit->size = size;

    return ret;
}