#include <protocol/rtmp_amf0.hpp>
#include <protocol/rtmp_consts.hpp>
#include <common/log.hpp>
#include <common/error.hpp>

namespace rtmp
{
AMF0Any::AMF0Any()
{
}

AMF0Any::~AMF0Any()
{
}

AMF0Ojbect *AMF0Any::Object()
{
    return new AMF0Ojbect;
}

int AMF0ReadUTF8(BufferManager *manager, std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read string length failed,ret=%d", ret);
        return ret;
    }

    int64_t len = manager->Read2Bytes();
    rs_verbose("amf0 read string length success,len=%d", len);

    if (len <= 0)
    {
        rs_verbose("amf0 read empty string.ret=%d", ret);
        return ret;
    }

    if (!manager->Require(len))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read string data failed,ret=%d", ret);
        return ret;
    }

    std::string str = manager->ReadString(len);
    value = str;
    rs_verbose("amf0 read string data success,str=%s", str.c_str());

    return ret;
}

int AMF0ReadString(BufferManager *manager, std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read string marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_STRING)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 check string marker failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_STRING, ret);
        return ret;
    }

    rs_verbose("amf0 read string marker success");

    return ret;
}

} // namespace rtmp