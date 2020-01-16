#ifndef RS_RTMP_AMF0_HPP
#define RS_RTMP_AMF0_HPP

#include <common/core.hpp>
#include <common/buffer.hpp>

#include <string>
#include <vector>

namespace rtmp
{

class AMF0Object;
class AMF0Any;
class AMF0ObjectEOF;
class AMF0String;
class AMF0Boolean;
class AMF0Number;
class AMF0Null;
class AMF0Undefined;
class AMF0EcmaArray;
class AMF0Date;
class AMF0StrictArray;

extern int AMF0ReadString(BufferManager *manager, std::string &value);
extern int AMF0ReadNumber(BufferManager *manager, double &value);
extern int AMF0ReadBoolean(BufferManager *manager, bool &value);
extern int AMF0ReadNull(BufferManager *manager);
extern int AMF0ReadUndefined(BufferManager *manager);
extern int AMF0ReadAny(BufferManager *manager, AMF0Any **ppvalue);

extern int AMF0WriteString(BufferManager *manager, const std::string &value);
extern int AMF0WriteNumber(BufferManager *manager, double value);
extern int AMF0WriteBoolean(BufferManager *manager, bool value);
extern int AMF0WriteNull(BufferManager *manager);
extern int AMF0WriteUndefined(BufferManager *manager);
extern int AMF0WriteAny(BufferManager *manager, AMF0Any *any);

class UnsortHashTable
{
public:
    UnsortHashTable();
    virtual ~UnsortHashTable();

public:
    virtual void Set(const std::string &key, AMF0Any *value);
    virtual int Count();
    virtual void Clear();
    virtual void Copy(UnsortHashTable *src);
    virtual std::string KeyAt(int index);
    virtual const char *KeyRawAt(int index);
    virtual AMF0Any *ValueAt(int index);
    virtual AMF0Any *EnsurePropertyString(const std::string &key);
    virtual AMF0Any *GetValue(const std::string &key);
    virtual AMF0Any *EnsurePropertyNumber(const std::string &key);

private:
    typedef std::pair<std::string, AMF0Any *> AMF0ObjectPropertyType;
    std::vector<AMF0ObjectPropertyType> properties_;
};

class AMF0Any
{
public:
    AMF0Any();
    virtual ~AMF0Any();

public:
    static AMF0Object *Object();
    static AMF0String *String(const std::string &value = "");
    static AMF0Boolean *Boolean(bool value = false);
    static AMF0Number *Number(double value = 0.0);
    static AMF0Null *Null();
    static AMF0Undefined *Undefined();
    static AMF0EcmaArray *EcmaArray();
    static AMF0Date *Date(int64_t value = 0);
    static AMF0StrictArray *StrictArray();
    static int Discovery(BufferManager *manager, AMF0Any **ppvalue);

    virtual int Read(BufferManager *manager) = 0;
    virtual int Write(BufferManager *manager) = 0;
    virtual int TotalSize() = 0;
    virtual AMF0Any *Copy() = 0;

    virtual AMF0Object *ToObject();
    virtual bool IsObject();
    virtual std::string ToString();
    virtual bool IsString();
    virtual double ToNumber();
    virtual bool IsNumber();

public:
    char marker;
};

class AMF0ObjectEOF : public AMF0Any
{
public:
    AMF0ObjectEOF();
    virtual ~AMF0ObjectEOF();

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;
};

class AMF0String : public AMF0Any
{
public:
    virtual ~AMF0String();

private:
    friend class AMF0Any;
    AMF0String(const std::string &v);

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

public:
    std::string value;
};

class AMF0Boolean : public AMF0Any
{
public:
    virtual ~AMF0Boolean();

private:
    friend class AMF0Any;
    AMF0Boolean(bool v);

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

public:
    bool value;
};

class AMF0Number : public AMF0Any
{
public:
    virtual ~AMF0Number();

private:
    friend class AMF0Any;
    AMF0Number(double v);

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

public:
    double value;
};

class AMF0Null : public AMF0Any
{
public:
    virtual ~AMF0Null();

private:
    friend class AMF0Any;
    AMF0Null();

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;
};

class AMF0Undefined : public AMF0Any
{
public:
    virtual ~AMF0Undefined();

private:
    friend class AMF0Any;
    AMF0Undefined();

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;
};

class AMF0EcmaArray : public AMF0Any
{
public:
    virtual ~AMF0EcmaArray();

private:
    friend class AMF0Any;
    AMF0EcmaArray();

public:
    virtual void Set(const std::string &key, AMF0Any *value);
    virtual std::string KeyAt(int index);
    virtual const char *KeyRawAt(int index);
    virtual AMF0Any *ValueAt(int index);
    virtual void Clear();
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

private:
    int count_;
    UnsortHashTable *properties_;
};

class AMF0Date : public AMF0Any
{
public:
    virtual ~AMF0Date();

private:
    friend class AMF0Any;
    AMF0Date(int64_t v);

public:
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

public:
    virtual int64_t Date();
    virtual int16_t TimeZone();

private:
    int64_t date_value_;
    int16_t time_zone_;
};

class AMF0StrictArray : public AMF0Any
{
public:
    virtual ~AMF0StrictArray();

private:
    friend class AMF0Any;
    AMF0StrictArray();

public:
    virtual void Append(AMF0Any *any);
    virtual void Clear();
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

private:
    int count_;
    std::vector<AMF0Any *> properties_;
};

class AMF0Object : public AMF0Any
{
public:
    AMF0Object();
    virtual ~AMF0Object();

public:
    virtual void Clear();
    virtual std::string KeyAt(int index);
    virtual const char *KeyRawAt(int index);
    virtual int Count();
    virtual AMF0Any *ValueAt(int index);
    virtual void Set(const std::string &key, AMF0Any *value);
    virtual AMF0Any *EnsurePropertyString(const std::string &key);
    virtual AMF0Any *GetValue(const std::string &key);
    virtual AMF0Any *EnsurePropertyNumber(const std::string &key);
    //AMF0Any
    virtual int Read(BufferManager *manager) override;
    virtual int Write(BufferManager *manager) override;
    virtual int TotalSize() override;
    virtual AMF0Any *Copy() override;

private:
    UnsortHashTable *properties_;
};

} // namespace rtmp
#endif