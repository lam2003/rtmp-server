#include <protocol/amf/amf0.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>
#include <common/error.hpp>

//amf0 marker
#define AMF0_NUMBER 0x00
#define AMF0_BOOLEAN 0x01
#define AMF0_STRING 0x02
#define AMF0_OBJECT 0x03
#define AMF0_MOVIE_CLIP 0x04
#define AMF0_NULL 0x05
#define AMF0_UNDEFINED 0x06
#define AMF0_REFENERCE 0x07
#define AMF0_ECMA_ARRAY 0x08
#define AMF0_ObJECT_END 0x09
#define AMF0_STRICT_ARRAY 0x0a
#define AMF0_DATE 0x0b
#define AMF0_LONG_STRING 0x0c
#define AMF0_UNSUPPORTED 0x0d
#define AMF0_RECORD_SET 0x0e
#define AMF0_XML_DOCUMENT 0x0f
#define AMF0_TYPED_OBJECT 0x10
//may be is amf3
#define AMF0_AVM_PLUS_OBJECT 0x11
#define AMF0_ORIGIN_STRICT_ARRAY 0x20
#define AMF0_INVALID 0x3f

static bool amf0_is_object_eof(BufferManager *manager)
{
    if (manager->Require(3))
    {
        int32_t flag = manager->Read3Bytes();
        manager->Skip(-3);

        return AMF0_ObJECT_END == flag;
    }
    return false;
}

static int amf0_write_utf8(BufferManager *manager, const std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write string length failed. ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(value.length());

    if (value.length() <= 0)
    {
        return ret;
    }

    if (!manager->Require(value.length()))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write string data failed. ret=%d", ret);
        return ret;
    }

    manager->WriteString(value);

    return ret;
}

static int amf0_read_utf8(BufferManager *manager, std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read string length failed. ret=%d", ret);
        return ret;
    }

    int64_t len = manager->Read2Bytes();

    if (len <= 0)
    {
        return ret;
    }

    if (!manager->Require(len))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read string data failed. ret=%d", ret);
        return ret;
    }

    std::string str = manager->ReadString(len);
    value = str;

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

void UnsortHashTable::Remove(const std::string &key)
{
    std::vector<AMF0ObjectPropertyType>::iterator it;
    for (it = properties_.begin(); it != properties_.end();)
    {
        std::string name = it->first;
        AMF0Any *any = it->second;

        if (key == name)
        {
            rs_freep(any);
            it = properties_.erase(it);
        }
        else
        {
            it++;
        }
    }
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
    return marker == AMF0_OBJECT;
}

std::string AMF0Any::ToString()
{
    AMF0String *p = dynamic_cast<AMF0String *>(this);
    return p->value;
}
bool AMF0Any::IsString()
{
    return marker == AMF0_STRING;
}

double AMF0Any::ToNumber()
{
    AMF0Number *p = dynamic_cast<AMF0Number *>(this);
    return p->value;
}

bool AMF0Any::IsNumber()
{
    return marker == AMF0_NUMBER;
}

AMF0EcmaArray *AMF0Any::ToEcmaArray()
{
    return dynamic_cast<AMF0EcmaArray *>(this);
}

bool AMF0Any::IsEcmaArray()
{
    return marker == AMF0_ECMA_ARRAY;
}

bool AMF0Any::ToBoolean()
{
    AMF0Boolean *p = dynamic_cast<AMF0Boolean *>(this);
    return p->value;
}

bool AMF0Any::IsBoolean()
{
    return marker == AMF0_BOOLEAN;
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
    marker = AMF0_ObJECT_END;
}

AMF0ObjectEOF::~AMF0ObjectEOF()
{
}

int AMF0ObjectEOF::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object EOF value failed. ret=%d", ret);
        return ret;
    }

    int16_t temp_value = manager->Read2Bytes();
    if (temp_value != 0x00)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object EOF value failed. required=0x00, actual=%#x, ret=%d", temp_value, ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object EOF marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_ObJECT_END)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object EOF marker failed. required=%#x, actual=%#x, ret=%d", AMF0_ObJECT_END, marker, ret);
        return ret;
    }

    return ret;
}

int AMF0ObjectEOF::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write object EOF value failed. ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(0x00);

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write object EOF marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_ObJECT_END);

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
    marker = AMF0_STRING;
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
    marker = AMF0_BOOLEAN;
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
    marker = AMF0_NUMBER;
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
    marker = AMF0_NULL;
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
    marker = AMF0_UNDEFINED;
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
    marker = AMF0_ECMA_ARRAY;
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

int AMF0EcmaArray::Count()
{
    return properties_->Count();
}

int AMF0EcmaArray::Read(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read ecma array marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_ECMA_ARRAY)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read ecma array marker failed. required=%#x, actual=%#x, ret=%d", AMF0_ECMA_ARRAY, marker, ret);
        return ret;
    }

    if (!manager->Require(4))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read ecma array count failed. ret=%d", ret);
        return ret;
    }

    int32_t count = manager->Read4Bytes();

    count_ = count;

    while (!manager->Empty())
    {
        if (amf0_is_object_eof(manager) != ERROR_SUCCESS)
        {
            AMF0ObjectEOF eof;
            if ((ret = eof.Read(manager)) != ERROR_SUCCESS)
            {
                rs_error("amf0 read ecma array EOF failed. ret=%d", ret);
                return ret;
            }
            break;
        }

        std::string property_name;
        if ((ret = amf0_read_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read ecma array property name failed. ret=%d", ret);
            return ret;
        }

        AMF0Any *property_value = nullptr;
        if ((ret = AMF0ReadAny(manager, &property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read ecma array property value failed. ret=%d", ret);
            return ret;
        }

        Set(property_name, property_value);
    }

    return ret;
}

int AMF0EcmaArray::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write ecma array marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_ECMA_ARRAY);

    if (!manager->Require(4))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write ecma array count failed. ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(count_);

    for (int i = 0; i < properties_->Count(); i++)
    {
        std::string property_name = KeyAt(i);
        AMF0Any *property_value = ValueAt(i);

        if ((ret = amf0_write_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write ecma array property name failed. ret=%d", ret);
            return ret;
        }

        if ((ret = AMF0WriteAny(manager, property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write ecma array property value failed. ret=%d", ret);
            return ret;
        }
    }

    AMF0ObjectEOF eof;
    if ((ret = eof.Write(manager)) != ERROR_SUCCESS)
    {
        rs_error("amf0 write ecma array EOF failed. ret=%d", ret);
        return ret;
    }

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
    marker = AMF0_STRICT_ARRAY;
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

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read strict array marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_STRICT_ARRAY)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read strict array marker failed. required=%#x, actual=%#x, ret=%d", AMF0_STRICT_ARRAY, marker, ret);
        return ret;
    }

    if (!manager->Require(4))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read strict array count failed. ret=%d", ret);
        return ret;
    }

    int32_t count = manager->Read4Bytes();
    count_ = count;

    for (int i = 0; i < count && !manager->Empty(); i++)
    {
        AMF0Any *elem = nullptr;
        if ((ret = AMF0ReadAny(manager, &elem)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read strict array value failed. ret=%d", ret);
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
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write strict array marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_STRICT_ARRAY);

    if (!manager->Require(4))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write strict array count failed. ret=%d", ret);
        return ret;
    }

    manager->Write4Bytes(count_);

    for (int i = 0; i < (int)properties_.size(); i++)
    {
        AMF0Any *any = properties_[i];
        if ((ret = AMF0WriteAny(manager, any)) != ERROR_SUCCESS)
        {
            rs_error("amf0 write strict array value failed. ret=%d", ret);
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
    marker = AMF0_DATE;
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
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read date marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_DATE)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read date marker failed. required=%#x, actual=%#x, ret=%d", AMF0_DATE, marker, ret);
        return ret;
    }

    if (!manager->Require(8))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read date value failed. ret=%d", ret);
        return ret;
    }

    date_value_ = manager->Read8Bytes();

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read date time_zone failed. ret=%d", ret);
        return ret;
    }

    time_zone_ = manager->Read2Bytes();

    return ret;
}

int AMF0Date::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write date marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_DATE);

    if (!manager->Require(8))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write date value failed. ret=%d", ret);
        return ret;
    }

    manager->Write8Bytes(date_value_);

    if (!manager->Require(2))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write date time_zone failed. ret=%d", ret);
        return ret;
    }

    manager->Write2Bytes(time_zone_);

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
    marker = AMF0_OBJECT;
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
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_OBJECT)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read object marker failed. required=%#x, actual=%#x, ret=%d", AMF0_OBJECT, marker, ret);
        return ret;
    }

    while (!manager->Empty())
    {
        if (amf0_is_object_eof(manager))
        {
            AMF0ObjectEOF eof;
            if ((ret = eof.Read(manager)) != ERROR_SUCCESS)
            {
                rs_error("amf0 read object EOF failed. ret=%d", ret);
                return ret;
            }
            break;
        }

        std::string property_name;
        if ((ret = amf0_read_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read object property name failed. ret=%d", ret);
            return ret;
        }

        AMF0Any *property_value = nullptr;

        if ((ret = AMF0ReadAny(manager, &property_value)) != ERROR_SUCCESS)
        {
            rs_error("amf0 read object property value failed. ret=%d", ret);
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

void AMF0Object::Remove(const std::string &key)
{
    properties_->Remove(key);
}

int AMF0Object::Write(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 encode object marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_OBJECT);

    for (int i = 0; i < Count(); i++)
    {
        std::string property_name = KeyAt(i);
        AMF0Any *property_value = ValueAt(i);

        if ((ret = amf0_write_utf8(manager, property_name)) != ERROR_SUCCESS)
        {
            ret = ERROR_PROTOCOL_AMF0_ENCODE;
            rs_error("amf0 encode object property_name failed. ret=%d", ret);
            return ret;
        }

        if ((ret = property_value->Write(manager)) != ERROR_SUCCESS)
        {
            ret = ERROR_PROTOCOL_AMF0_ENCODE;
            rs_error("amf0 encode object property_value failed. ret=%d", ret);
            return ret;
        }
    }

    AMF0ObjectEOF eof;
    if ((ret = eof.Write(manager)) != ERROR_SUCCESS)
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 encode object EOF failed. ret=%d", ret);
        return ret;
    }

    return ret;
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
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read any marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();

    manager->Skip(-1);

    switch (marker)
    {
    case AMF0_STRING:
    {
        *ppvalue = AMF0Any::String();
        return ret;
    }
    case AMF0_BOOLEAN:
    {
        *ppvalue = AMF0Any::Boolean();
        return ret;
    }
    case AMF0_NUMBER:
    {
        *ppvalue = AMF0Any::Number();
        return ret;
    }
    case AMF0_NULL:
    {
        *ppvalue = AMF0Any::Null();
        return ret;
    }
    case AMF0_UNDEFINED:
    {
        *ppvalue = AMF0Any::Undefined();
        return ret;
    }
    case AMF0_OBJECT:
    {
        *ppvalue = AMF0Any::Object();
        return ret;
    }
    case AMF0_ECMA_ARRAY:
    {
        *ppvalue = AMF0Any::EcmaArray();
        return ret;
    }
    case AMF0_STRICT_ARRAY:
    {
        *ppvalue = AMF0Any::StrictArray();
        return ret;
    }
    case AMF0_DATE:
    {
        *ppvalue = AMF0Any::Date();
        return ret;
    }
    default:
    {
        ret = ERROR_RTMP_AMF0_INVALID;
        rs_error("invalid amf0 message type. marker=%#x, ret=%d", marker, ret);
        return ret;
    }
    }
}

int AMF0ReadString(BufferManager *manager, std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read string marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_STRING)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read string marker failed. required=%#x, actual=%#x, ret=%d", AMF0_STRING, marker, ret);
        return ret;
    }

    return amf0_read_utf8(manager, value);
}

int AMF0ReadNumber(BufferManager *manager, double &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read number marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_NUMBER)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read number marker failed. required=%#x, actual=%#x, ret=%d", AMF0_NUMBER, marker, ret);
        return ret;
    }

    if (!manager->Require(8))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read number data failed. ret=%d", ret);
        return ret;
    }

    int64_t temp_value = manager->Read8Bytes();
    memcpy(&value, &temp_value, 8);

    return ret;
}

int AMF0WriteNumber(BufferManager *manager, double value)
{
    int ret = ERROR_SUCCESS;
    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write number marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_NUMBER);

    if (!manager->Require(8))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write number data failed. ret=%d", ret);
        return ret;
    }

    int64_t temp_value = 0x00;
    memcpy(&temp_value, &value, 8);
    manager->Write8Bytes(temp_value);

    return ret;
}

int AMF0WriteString(BufferManager *manager, const std::string &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write string marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_STRING);

    return amf0_write_utf8(manager, value);
}

int AMF0ReadBoolean(BufferManager *manager, bool &value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read bool marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_BOOLEAN)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read bool marker failed. required=%#x, actual=%#x, ret=%d", AMF0_BOOLEAN, marker, ret);
        return ret;
    }

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read bool value failed. ret=%d", ret);
        return ret;
    }

    value = (manager->Read1Bytes() != 0);

    return ret;
}

int AMF0WriteBoolean(BufferManager *manager, bool value)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write bool marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_BOOLEAN);

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write bool value failed. ret=%d", ret);
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

    return ret;
}

int AMF0ReadNull(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read null marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_NULL)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read null marker failed. required=%#x, actual=%#x, ret=%d", AMF0_NULL, marker, ret);
        return ret;
    }

    return ret;
}

int AMF0WriteNull(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write null marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_NULL);

    return ret;
}

int AMF0ReadUndefined(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read undefined marker failed. ret=%d", ret);
        return ret;
    }

    char marker = manager->Read1Bytes();
    if (marker != AMF0_UNDEFINED)
    {
        ret = ERROR_PROTOCOL_AMF0_DECODE;
        rs_error("amf0 read undefined marker failed. required=%#x,actual=%#x, ret=%d", AMF0_UNDEFINED, marker, ret);
        return ret;
    }

    return ret;
}

int AMF0WriteUndefined(BufferManager *manager)
{
    int ret = ERROR_SUCCESS;

    if (!manager->Require(1))
    {
        ret = ERROR_PROTOCOL_AMF0_ENCODE;
        rs_error("amf0 write undefined marker failed. ret=%d", ret);
        return ret;
    }

    manager->Write1Bytes(AMF0_UNDEFINED);

    return ret;
}

int AMF0ReadAny(BufferManager *manager, AMF0Any **ppvalue)
{
    int ret = ERROR_SUCCESS;

    if ((ret = AMF0Any::Discovery(manager, ppvalue)) != ERROR_SUCCESS)
    {
        rs_error("amf0 discovery any elem failed. ret=%d", ret);
        return ret;
    }

    if ((ret = (*ppvalue)->Read(manager)) != ERROR_SUCCESS)
    {
        rs_error("amf0 parse any elem failed. ret=%d", ret);
        rs_freep(ppvalue);
        return ret;
    }

    return ret;
}

int AMF0WriteAny(BufferManager *manager, AMF0Any *value)
{
    return value->Write(manager);
}
