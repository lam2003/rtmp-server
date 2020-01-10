#include <protocol/rtmp_amf0.hpp>
#include <protocol/rtmp_consts.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>

namespace rtmp
{

UnsortHashTable::UnsortHashTable()
{
}

UnsortHashTable::~UnsortHashTable()
{
}

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

AMF0Boolean *AMF0Any::Boolean(bool value)
{
    return new AMF0Boolean(value);
}
AMF0String *AMF0Any::String(const std::string &value)
{
    return new AMF0String(value);
}
AMF0Number *AMF0Any::Number(double value)
{
    return new AMF0Number(value);
}
AMF0Null *AMF0Any::Null()
{
    return new AMF0Null;
}
AMF0Undefined *AMF0Any::Undefined()
{
    return new AMF0Undefined;
}
AMF0EcmaArray *AMF0Any::EcmaArray()
{
    return new AMF0EcmaArray;
}
AMF0Date *AMF0Any::Date(int64_t value)
{
    return new AMF0Date(value);
}
AMF0StrictArray *AMF0Any::StrictArray()
{
    return new AMF0StrictArray();
}

static bool amf0_is_object_eof(BufferManager *manager)
{
    if (manager->Require(3))
    {
        int32_t flag = manager->Read3Bytes();
        manager->Skip(-3);

        return RTMP_AMF0_ObJECT_END == flag;
    }
    return false;
}

int AMF0Any::Discovery(BufferManager *manager, AMF0Any **ppvalue)
{
    int ret = ERROR_SUCCESS;

    if (amf0_is_object_eof(manager))
    {
        *ppvalue = new AMF0ObjectEOF;
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read any marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    rs_verbose("amf0 read any marker success,marker=%#x", marker);

    manager->Skip(-1);

    switch (marker)
    {
    case RTMP_AMF0_STRING:
    {
        *ppvalue = AMF0Any::String();
        return ret;
    }
    case RTMP_AMF0_BOOLEAN:
    {
        *ppvalue = AMF0Any::Boolean();
        return ret;
    }
    case RTMP_AMF0_NUMBER:
    {
        *ppvalue = AMF0Any::Number();
        return ret;
    }
    case RTMP_AMF0_NULL:
    {
        *ppvalue = AMF0Any::Null();
        return ret;
    }
    case RTMP_AMF0_UNDEFINED:
    {
        *ppvalue = AMF0Any::Undefined();
        return ret;
    }
    case RTMP_AMF0_OBJECT:
    {
        *ppvalue = AMF0Any::Object();
        return ret;
    }
    case RTMP_AMF0_ECMA_ARRAY:
    {
        *ppvalue = AMF0Any::EcmaArray();
        return ret;
    }
    case RTMP_AMF0_STRICT_ARRAY:
    {
        *ppvalue = AMF0Any::StrictArray();
        return ret;
    }
    case RTMP_AMF0_DATE:
    {
        *ppvalue = AMF0Any::Date();
        return ret;
    }
    default:
    {
        //  case RTMP_AMF0_INVALID
        ret = ERROR_RTMP_AMF0_INVALID;
        rs_error("invalid amf0 message type,marker=%#x,ret=%d", marker, ret);
        return ret;
    }
    }
}

//AMFObjectEOF
AMF0ObjectEOF::AMF0ObjectEOF()
{
    marker = RTMP_AMF0_ObJECT_END;
}

AMF0ObjectEOF::~AMF0ObjectEOF()
{
}

int AMF0ObjectEOF::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object eof value failed,ret=%d", ret);
        return ret;
    }

    int16_t temp_value = manager->Read2Bytes();
    if (temp_value != 0x00)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object eof value check failed,must be 0x00,actual is %#x,ret=%d", temp_value, ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object eof marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_ObJECT_END)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object eof marker check failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_ObJECT_END, ret);
        return ret;
    }

    rs_verbose("amf0 read object eof success");

    return ret;
}

int AMF0ObjectEOF::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write object eof value failed,ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(0x00);
    rs_verbose("amf0 write object eof value success");

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write object eof marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_ObJECT_END);
    rs_verbose("amf0 write object eof marker success");

    rs_verbose("amf0 write object eof success");

    return ret;
}

int AMF0ObjectEOF::TotalSize()
{
    //2 bytes zero value + 1 bytes marker
    return 2 + 1;
}

AMF0Any *AMF0ObjectEOF::Copy()
{
    return new AMF0ObjectEOF();
}

//AMF0String
AMF0String::AMF0String(const std::string &v) : value(v)
{
    marker = RTMP_AMF0_STRING;
}

AMF0String::~AMF0String()
{
}

int AMF0String::Read(BufferManager *manager)
{
    return AMF0ReadString(manager, value);
}

int AMF0String::Write(BufferManager *manager)
{
    return AMF0WriteString(manager, value);
}

int AMF0String::TotalSize()
{
    //1 bytes marker + 2 bytes utf-8 length + n bytes string length
    return 1 + 2 + value.length();
}
AMF0Any *AMF0String::Copy()
{
    return new AMF0String(value);
}

//AMFBoolean
AMF0Boolean::AMF0Boolean(bool v) : value(v)
{
    marker = RTMP_AMF0_BOOLEAN;
}

AMF0Boolean::~AMF0Boolean()
{
}

int AMF0Boolean::Read(BufferManager *manager)
{
    return AMF0ReadBoolean(manager, value);
}

int AMF0Boolean::Write(BufferManager *manager)
{
    return AMF0WriteBoolean(manager, value);
}

int AMF0Boolean::TotalSize()
{
    //1 bytes marker + 1 bytes boolean length
    return 1 + 1;
}
AMF0Any *AMF0Boolean::Copy()
{
    return new AMF0Boolean(value);
}

//AMFNumber
AMF0Number::AMF0Number(double v) : value(v)
{
    marker = RTMP_AMF0_NUMBER;
}

AMF0Number::~AMF0Number()
{
}

int AMF0Number::Read(BufferManager *manager)
{
    return AMF0ReadNumber(manager, value);
}

int AMF0Number::Write(BufferManager *manager)
{
    return AMF0WriteNumber(manager, value);
}
int AMF0Number::TotalSize()
{
    //1 bytes marker + 8 bytes number value
    return 1 + 8;
}
AMF0Any *AMF0Number::Copy()
{
    return new AMF0Number(value);
}

//AMFNull
AMF0Null::AMF0Null()
{
    marker = RTMP_AMF0_NULL;
}

AMF0Null::~AMF0Null()
{
}
int AMF0Null::Read(BufferManager *manager)
{
    return AMF0ReadNull(manager);
}
int AMF0Null::Write(BufferManager *manager)
{
    return AMF0WriteNull(manager);
}
int AMF0Null::TotalSize()
{
    //1 bytes marker
    return 1;
}
AMF0Any *AMF0Null::Copy()
{
    return new AMF0Null;
}

//AMF0Undefined
AMF0Undefined::AMF0Undefined()
{
    marker = RTMP_AMF0_UNDEFINED;
}

AMF0Undefined::~AMF0Undefined()
{
}
int AMF0Undefined::Read(BufferManager *manager)
{
    return AMF0ReadUndefined(manager);
}
int AMF0Undefined::Write(BufferManager *manager)
{
    return AMF0WriteUndefined(manager);
}
int AMF0Undefined::TotalSize()
{
    //1 bytes marker
    return 1;
}
AMF0Any *AMF0Undefined::Copy()
{
    return new AMF0Undefined;
}

//AMF0EcmaArray
AMF0EcmaArray::AMF0EcmaArray() : count_(0)
{
    marker = RTMP_AMF0_ECMA_ARRAY;
    properties_ = new UnsortHashTable;
    eof_ = new AMF0ObjectEOF;
}
AMF0EcmaArray::~AMF0EcmaArray()
{
    rs_freep(properties_);
    rs_freep(eof_);
}
int AMF0EcmaArray::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    // if ((ret = manager->Require(1))!= ERROR_SUCCESS)
    // {
    //     ret = ERROR_RTMP_AMF0_DECODE;
    //     rs_error("amf0 read ")
    // }
    return ret;
}
int AMF0EcmaArray::Write(BufferManager *manager)
{
}
int AMF0EcmaArray::TotalSize()
{
}
AMF0Any *AMF0EcmaArray::Copy()
{
}

//AMF0Date
AMF0Date::AMF0Date(int64_t v)
{
    marker = RTMP_AMF0_DATE;
    date_value_ = v;
    time_zone_ = 0;
}

AMF0Date::~AMF0Date()
{
}

int AMF0Date::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read date marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_DATE)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read date check marker failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_DATE, ret);
        return ret;
    }

    rs_verbose("amf0 read date marker success");

    if (!manager->Require(8))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read date_value failed,ret=%d", ret);
        return ret;
    }

    date_value_ = manager->Read8Bytes();

    rs_verbose("amf0 read date_value success,date_value=%lld", date_value_);

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read time_zone failed,ret=%d", ret);
        return ret;
    }

    time_zone_ = manager->Read2Bytes();

    rs_verbose("amf0 read time_zone success,time_zone=%d", time_zone_);

    rs_verbose("amf0 read date success");

    return ret;
}

int AMF0Date::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write date marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_DATE);
    rs_verbose("amf0 write date marker success");

    if (!manager->Require(8))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write date_value failed,ret=%d", ret);
        return ret;
    }

    manager->Write8Bytes(date_value_);

    rs_verbose("amf0 write date_value success,date_value=%lld", date_value_);

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write time_zone failed,ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(time_zone_);

    rs_verbose("amf0 write time_zone success,time_zone=%d", time_zone_);

    rs_verbose("amf0 write date success");

    return ret;
}

int AMF0Date::TotalSize()
{
    //1 bytes marker + 8 bytes date_value + 2 bytes time_zone
    return 1 + 8 + 2;
}

AMF0Any *AMF0Date::Copy()
{
    AMF0Date *copy = new AMF0Date(0);
    copy->date_value_ = date_value_;
    copy->time_zone_ = time_zone_;
    return copy;
}

//AMF0Object
AMF0Ojbect::AMF0Ojbect()
{
    marker = RTMP_AMF0_OBJECT;
    hash_table_ = new UnsortHashTable;
    eof_ = new AMF0ObjectEOF;
}

AMF0Ojbect::~AMF0Ojbect()
{
    rs_freep(hash_table_);
    rs_freep(eof_);
}

int AMF0Ojbect::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_OBJECT)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read object marker check failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("amf0 read object marker success");

    while (!manager->Empty())
    {
        std::string property_name;
        if ((ret = AMF0ReadString(manager, property_name)) != ERROR_SUCCESS)
        {
            ret = ERROR_RTMP_AMF0_DECODE;
            rs_error("amf0 read object property name failed,ret=%d", ret);
            return ret;
        }
    }

    return ret;
}

int AMF0Ojbect::Write(BufferManager *manager)
{
    return 0;
}

int AMF0Ojbect::TotalSize()
{
    return 0;
}

AMF0Any *AMF0Ojbect::Copy()
{
    return nullptr;
}

static int amf0_read_utf8(BufferManager *manager, std::string &value)
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

    rs_verbose("amf0 read string data success,data=%s", str.c_str());

    rs_verbose("amf0 read string success");

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
        rs_error("amf0 read string marker check failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_STRING, ret);
        return ret;
    }

    rs_verbose("amf0 read string marker success");

    return amf0_read_utf8(manager, value);
}

int AMF0ReadNumber(BufferManager *manager, double &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read number marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_NUMBER)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read number marker check failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_NUMBER, ret);
        return ret;
    }

    rs_verbose("amf0 read number marker success");

    if (!manager->Require(8))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read number data failed,ret=%d", ret);
        return ret;
    }

    int64_t temp_value = manager->Read8Bytes();
    memcpy(&value, &temp_value, 8);

    rs_verbose("amf0 read number data success,data=%.1f", value);

    rs_verbose("amf0 read number success");

    return ret;
}

int AMF0WriteNumber(BufferManager *manager, double value)
{
    int ret = ERROR_SUCCESS;
    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write number marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_NUMBER);

    rs_verbose("amf0 write number marker success");

    if (!manager->Require(8))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write number data failed,ret=%d", ret);
        return ret;
    }

    int64_t temp_value = 0x00;
    memcpy(&temp_value, &value, 8);
    manager->Write8Bytes(temp_value);

    rs_verbose("amf0 write number data success,data=%.1f", value);

    rs_verbose("amf0 write number success");

    return ret;
}

static int amf0_write_utf8(BufferManager *manager, const std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write string length failed,ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(value.length());
    rs_verbose("amf0 write string length success,len=%d", (int)value.length());

    if (value.length() <= 0)
    {
        rs_verbose("amf0 write empty stream,ret=%d", ret);
        return ret;
    }

    if (!manager->Require(value.length()))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write string data failed,ret=%d", ret);
        return ret;
    }

    manager->WriteString(value);

    rs_verbose("amf0 write string data success");

    rs_verbose("amf0 write string success");

    return ret;
}

int AMF0WriteString(BufferManager *manager, const std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write string marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_STRING);

    rs_verbose("amf0 write string marker success");

    return amf0_write_utf8(manager, value);
}

int AMF0ReadBoolean(BufferManager *manager, bool &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read bool marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_BOOLEAN)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read bool marker check failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_BOOLEAN, ret);
        return ret;
    }

    rs_verbose("amf0 read bool marker success");

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read bool value failed,ret=%d", ret);
        return ret;
    }

    value = (manager->Read1Bytes() != 0);

    rs_verbose("amf0 read bool value success,value=%d", value);

    rs_verbose("amf0 read bool success");

    return ret;
}

int AMF0WriteBoolean(BufferManager *manager, bool value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write bool marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_BOOLEAN);
    rs_verbose("amf0 write bool marker success");

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write bool value failed,ret=%d", ret);
        return ret;
    }

    if (value)
    {
        manager->Write1Bytes(0x01);
    }
    else
    {
        manager->Write1Bytes(0x00);
    }

    rs_verbose("amf0 write bool value success,value=%d", value);

    rs_verbose("amf0 write bool success");

    return ret;
}

int AMF0ReadNull(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read null marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_NULL)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read null marker check failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_NULL, ret);
        return ret;
    }

    rs_verbose("amf0 read null marker success");

    rs_verbose("amf0 read null success");

    return ret;
}

int AMF0WriteNull(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write null marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_NULL);
    rs_verbose("amf0 write null marker success");

    rs_verbose("amf0 write null success");

    return ret;
}

int AMF0ReadUndefined(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read undefined marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_UNDEFINED)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read undefined check marker failed,marker=%#x,required=%#x,ret=%d", marker, RTMP_AMF0_UNDEFINED, ret);
        return ret;
    }
    rs_verbose("amf0 read undefined marker success");

    rs_verbose("amf0 read undefined success");

    return ret;
}

int AMF0WriteUndefined(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write undefined marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_UNDEFINED);

    rs_verbose("amf0 write undefined marker success");

    rs_verbose("amf0 write undefined sucess");

    return ret;
}

int AMF0ReadAny(BufferManager *manager, AMF0Any **ppvalue)
{
    int ret = ERROR_SUCCESS;

    return ret;
}

} // namespace rtmp