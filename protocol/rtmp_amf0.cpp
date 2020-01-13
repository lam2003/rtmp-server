#include <protocol/rtmp_amf0.hpp>
#include <protocol/rtmp_consts.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>

namespace rtmp
{

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

UnsortHashTable::UnsortHashTable()
{
}

UnsortHashTable::~UnsortHashTable()
{
    Clear();
}

void UnsortHashTable::Set(const std::string &key, AMF0Any *value)
{
    std::vector<AMF0ObjectPropertyType>::iterator it;
    for (it = properties_.begin(); it != properties_.end(); it++)
    {
        AMF0ObjectPropertyType &elem = *it;
        std::string property_name = elem.first;
        AMF0Any *property_value = elem.second;

        if (property_name == key)
        {
            rs_freep(property_value);
            properties_.erase(it);
            break;
        }
    }

    if (value)
    {
        properties_.push_back(std::make_pair(key, value));
    }
}

int UnsortHashTable::Count()
{
    return (int)properties_.size();
}

void UnsortHashTable::Clear()
{
    std::vector<AMF0ObjectPropertyType>::iterator it;
    for (it = properties_.begin(); it != properties_.end(); it++)
    {
        AMF0ObjectPropertyType &elem = *it;
        AMF0Any *property_value = elem.second;
        rs_freep(property_value);
    }

    properties_.clear();
}

void UnsortHashTable::Copy(UnsortHashTable *src)
{
    std::vector<AMF0ObjectPropertyType>::iterator it;

    for (it = src->properties_.begin(); it != src->properties_.end(); it++)
    {
        AMF0ObjectPropertyType &elem = *it;
        std::string property_name = elem.first;
        AMF0Any *property_value = elem.second;
        Set(property_name, property_value->Copy());
    }
}

std::string UnsortHashTable::KeyAt(int index)
{
    AMF0ObjectPropertyType &elem = properties_[index];
    return elem.first;
}

const char *UnsortHashTable::KeyRawAt(int index)
{
    AMF0ObjectPropertyType &elem = properties_[index];
    return elem.first.data();
}

AMF0Any *UnsortHashTable::ValueAt(int index)
{
    AMF0ObjectPropertyType &elem = properties_[index];
    return elem.second;
}

AMF0Any *UnsortHashTable::GetValue(const std::string &key)
{
    std::vector<AMF0ObjectPropertyType>::iterator it;
    for (it = properties_.begin(); it != properties_.end(); it++)
    {
        AMF0ObjectPropertyType &elem = *it;
        std::string property_name = elem.first;
        AMF0Any *property_value = elem.second;

        if (property_name == key)
        {
            return property_value;
        }
    }
    return nullptr;
}

AMF0Any *UnsortHashTable::EnsurePropertyString(const std::string &key)
{
    AMF0Any *value = GetValue(key);
    if (!value)
    {
        return nullptr;
    }

    if (!value->IsString())
    {
        return nullptr;
    }

    return value;
}

AMF0Any *UnsortHashTable::EnsurePropertyNumber(const std::string &key)
{
    AMF0Any *value = GetValue(key);
    if (!value)
    {
        return nullptr;
    }
    if (!value->IsNumber())
    {
        return nullptr;
    }

    return value;
}

AMF0Any::AMF0Any()
{
}

AMF0Any::~AMF0Any()
{
}

AMF0Object *AMF0Any::ToObject()
{
    return dynamic_cast<AMF0Object *>(this);
}

bool AMF0Any::IsObject()
{
    return marker == RTMP_AMF0_OBJECT;
}

std::string AMF0Any::ToString()
{
    AMF0String *p = dynamic_cast<AMF0String *>(this);
    return p->value;
}
bool AMF0Any::IsString()
{
    return marker == RTMP_AMF0_STRING;
}

double AMF0Any::ToNumber()
{
    AMF0Number *p = dynamic_cast<AMF0Number *>(this);
    return p->value;
}

bool AMF0Any::IsNumber()
{
    return marker == RTMP_AMF0_NUMBER;
}

AMF0Object *AMF0Any::Object()
{
    return new AMF0Object;
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
    return AMF0_LEN_OBJ_EOF;
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
    return AMF0_LEN_STR(value);
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
    return AMF0_LEN_BOOLEAN;
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
    return AMF0_LEN_NUMBER;
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
    return AMF0_LEN_NULL;
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
    return AMF0_LEN_UNDEFINED;
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
}
AMF0EcmaArray::~AMF0EcmaArray()
{
    rs_freep(properties_);
}

void AMF0EcmaArray::Set(const std::string &key, AMF0Any *value)
{
    properties_->Set(key, value);
}

std::string AMF0EcmaArray::KeyAt(int index)
{
    return properties_->KeyAt(index);
}

const char *AMF0EcmaArray::KeyRawAt(int index)
{
    return properties_->KeyRawAt(index);
}

AMF0Any *AMF0EcmaArray::ValueAt(int index)
{
    return properties_->ValueAt(index);
}

void AMF0EcmaArray::Clear()
{
    properties_->Clear();
}

int AMF0EcmaArray::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = manager->Require(1)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read ecma array marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_ECMA_ARRAY)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read ecma array check marker failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("amf0 read ecma array marker success");

    if ((ret = manager->Require(4)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read ecma array count failed,ret=%d", ret);
        return ret;
    }

    int32_t count = manager->Read4Bytes();
    rs_verbose("amf0 read ecam array count success,count=%d", count);

    count_ = count;

    while (!manager->Empty())
    {
        if (amf0_is_object_eof(manager) != ERROR_SUCCESS)
        {
            AMF0ObjectEOF eof;
            if ((ret = eof.Read(manager)) != ERROR_SUCCESS)
            {
                rs_error("amf0 read ecma array eof failed,ret=%d", ret);
                return ret;
            }
            rs_verbose("amf0 read ecma array eof");
            break;
        }

        std::string property_name;
        if ((ret = amf0_read_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read ecma array property name failed,ret=%d", ret);
            return ret;
        }

        AMF0Any *property_value = nullptr;
        if ((ret = AMF0ReadAny(manager, &property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read ecma array property value failed,ret=%d", ret);
            return ret;
        }

        Set(property_name, property_value);
    }

    return ret;
}

int AMF0EcmaArray::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((manager->Require(1)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write ecma array marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_ECMA_ARRAY);
    rs_verbose("amf0 write ecma array marker success");

    if ((manager->Require(4)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write ecma array count failed,ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(count_);
    rs_verbose("amf0 write ecma array count success");

    for (int i = 0; i < properties_->Count(); i++)
    {
        std::string property_name = KeyAt(i);
        AMF0Any *property_value = ValueAt(i);

        if ((ret = amf0_write_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write ecma array property name failed,ret=%d", ret);
            return ret;
        }

        if ((ret = AMF0ReadAny(manager, &property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write ecma array property value failed,ret=%d", ret);
            return ret;
        }
    }

    AMF0ObjectEOF eof;
    if ((ret = eof.Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("amf0 write ecma array eof failed,ret=%d", ret);
        return ret;
    }

    rs_verbose("amf0 write ecma array success");
    return ret;
}

int AMF0EcmaArray::TotalSize()
{
    int size = 1 + 4;

    for (int i = 0; i < properties_->Count(); i++)
    {
        std::string key = KeyAt(i);
        AMF0Any *value = ValueAt(i);

        size += AMF0_LEN_UTF8(key);
        size += AMF0_LEN_ANY(value);
    }

    size += AMF0_LEN_OBJ_EOF;

    return size;
}

AMF0Any *AMF0EcmaArray::Copy()
{
    AMF0EcmaArray *copy = new AMF0EcmaArray;
    copy->properties_->Copy(properties_);
    copy->count_ = count_;
    return copy;
}

//AMF0StrictArray
AMF0StrictArray::AMF0StrictArray() : count_(0)
{
    marker = RTMP_AMF0_STRICT_ARRAY;
}
AMF0StrictArray::~AMF0StrictArray()
{
    Clear();
}

void AMF0StrictArray::Append(AMF0Any *any)
{
    properties_.push_back(any);
    count_ = properties_.size();
}

void AMF0StrictArray::Clear()
{
    std::vector<AMF0Any *>::iterator it;
    for (it = properties_.begin(); it != properties_.end(); it++)
    {
        AMF0Any *any = *it;
        rs_freep(any);
    }
    properties_.clear();
}

int AMF0StrictArray::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if ((ret = manager->Require(1)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read strict array marker failed,ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != RTMP_AMF0_STRICT_ARRAY)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read strict array check marker failed,ret=%d", ret);
        return ret;
    }

    if ((ret = manager->Require(4)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_DECODE;
        rs_error("amf0 read strict array count failed,ret=%d", ret);
        return ret;
    }

    int32_t count = manager->Read4Bytes();
    count_ = count;

    for (int i = 0; i < count && !manager->Empty(); i++)
    {
        AMF0Any *elem = nullptr;
        if ((ret = AMF0ReadAny(manager, &elem)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read strict array value failed,ret=%d", ret);
            return ret;
        }

        properties_.push_back(elem);
    }

    return ret;
}
int AMF0StrictArray::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write strict array marker failed,ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(RTMP_AMF0_STRICT_ARRAY);

    if ((ret = manager->Require(4)) != ERROR_SUCCESS)
    {
        ret = ERROR_RTMP_AMF0_ENCODE;
        rs_error("amf0 write strict array count failed,ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(count_);

    for (int i = 0; i < (int)properties_.size(); i++)
    {
        AMF0Any *any = properties_[i];
        if ((ret = AMF0WriteAny(manager, any)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write strict array value failed,ret=%d", ret);
            return ret;
        }
    }

    return ret;
}
int AMF0StrictArray::TotalSize()
{
    int size = 1 + 4;

    for (int i = 0; i < (int)properties_.size(); i++)
    {
        AMF0Any *any = properties_[i];
        size += AMF0_LEN_ANY(any);
    }
    return size;
}
AMF0Any *AMF0StrictArray::Copy()
{

    AMF0StrictArray *copy = new AMF0StrictArray;

    std::vector<AMF0Any *>::iterator it;
    for (it = properties_.begin(); it != properties_.end(); it++)
    {
        AMF0Any *any = *it;
        copy->Append(any);
    }
    copy->count_ = count_;
    return copy;
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
    return AMF0_LEN_DATE;
}

AMF0Any *AMF0Date::Copy()
{
    AMF0Date *copy = new AMF0Date(0);
    copy->date_value_ = date_value_;
    copy->time_zone_ = time_zone_;
    return copy;
}

int64_t AMF0Date::Date()
{
    return date_value_;
}

int16_t AMF0Date::TimeZone()
{
    return time_zone_;
}

//AMF0Object
AMF0Object::AMF0Object()
{
    marker = RTMP_AMF0_OBJECT;
    properties_ = new UnsortHashTable;
}

AMF0Object::~AMF0Object()
{
    rs_freep(properties_);
}

int AMF0Object::Read(BufferManager *manager)
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
        if (amf0_is_object_eof(manager))
        {
            AMF0ObjectEOF eof;
            if ((ret = eof.Read(manager)) != ERROR_SUCCESS)
            {
                rs_error("amf0 read object eof failed,ret=%d", ret);
                return ret;
            }
            rs_verbose("amf0  read object eof");
            break;
        }

        std::string property_name;
        if ((ret = amf0_read_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read object property name failed,ret=%d", ret);
            return ret;
        }

        AMF0Any *property_value = nullptr;

        if ((ret = AMF0ReadAny(manager, &property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read object property value failed,ret=%d", ret);
            rs_freep(property_value);
            return ret;
        }

        Set(property_name, property_value);
    }

    return ret;
}

std::string AMF0Object::KeyAt(int index)
{
    return properties_->KeyAt(index);
}

int AMF0Object::Count()
{
    return properties_->Count();
}

const char *AMF0Object::KeyRawAt(int index)
{
    return properties_->KeyRawAt(index);
}

AMF0Any *AMF0Object::ValueAt(int index)
{
    return properties_->ValueAt(index);
}

void AMF0Object::Set(const std::string &key, AMF0Any *value)
{
    properties_->Set(key, value);
}

AMF0Any *AMF0Object::EnsurePropertyString(const std::string &key)
{
    return properties_->EnsurePropertyString(key);
}

AMF0Any *AMF0Object::EnsurePropertyNumber(const std::string &key)
{
    return properties_->EnsurePropertyNumber(key);
}

AMF0Any *AMF0Object::GetValue(const std::string &key)
{
    return properties_->GetValue(key);
}

int AMF0Object::Write(BufferManager *manager)
{
    return 0;
}

int AMF0Object::TotalSize()
{
    int size = 1;
    for (int i = 0; i < properties_->Count(); i++)
    {
        std::string property_name = KeyAt(i);
        AMF0Any *property_value = ValueAt(i);

        size += AMF0_LEN_UTF8(property_name);
        size += AMF0_LEN_ANY(property_value);
    }

    size += AMF0_LEN_OBJ_EOF;

    return size;
}

void AMF0Object::Clear()
{
    properties_->Clear();
}

AMF0Any *AMF0Object::Copy()
{
    AMF0Object *copy = new AMF0Object;
    copy->properties_->Copy(properties_);
    return copy;
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

    if ((ret = AMF0Any::Discovery(manager, ppvalue)) != ERROR_SUCCESS)
    {
        rs_error("amf0 discovery any elem failed,ret=%d", ret);
        return ret;
    }

    if ((ret = (*ppvalue)->Read(manager)) != ERROR_SUCCESS)
    {
        rs_error("amf0 parse elem failed,ret=%d", ret);
        rs_freep(ppvalue);
        return ret;
    }

    return ret;
}

int AMF0WriteAny(BufferManager *manager, AMF0Any *value)
{
    return value->Write(manager);
}

} // namespace rtmp