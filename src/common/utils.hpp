/*
 * @Author: linmin
 * @Date: 2020-02-08 11:59:08
 * @LastEditTime : 2020-02-09 16:58:37
 */
#ifndef RS_UTILS_HPP
#define RS_UTILS_HPP

#include <common/buffer.hpp>
#include <common/core.hpp>

#include <string>

#define rs_assert(expression) assert(expression)

#define rs_freep(p)                                                            \
    if (p) {                                                                   \
        delete p;                                                              \
        p = nullptr;                                                           \
    }                                                                          \
    (void)0

#define rs_freepa(pa)                                                          \
    if (pa) {                                                                  \
        delete[] pa;                                                           \
        pa = nullptr;                                                          \
    }                                                                          \
    (void)0

#define rs_min(a, b) (((a) < (b)) ? (a) : (b))
#define rs_max(a, b) (((a) < (b)) ? (b) : (a))

class Utils {
  public:
    static std::string GetPeerIP(int32_t fd);
    static std::string GetLocalIP(int32_t fd);
    static int         GetLocalPort(int32_t fd);
    static void        RandomGenerate(char* bytes, int32_t size);
    static std::string StringReplace(const std::string& str,
                                     const std::string& oldstr,
                                     const std::string& newstr);
    static bool StringEndsWith(const std::string& str, const std::string& flag);
    static std::string StringEraseLastSubstr(const std::string& str,
                                             const std::string& erase_str);
    static std::string StringTrimStart(const std::string& str,
                                       const std::string& trim_chars);
    static std::string StringTrimEnd(const std::string& str,
                                     const std::string& trim_chars);
    static std::string StringRemove(const std::string& str,
                                    const std::string& remove_chars);
    static int64_t     GetSteadyNanoSeconds();
    static int64_t     GetSteadyMicroSeconds();
    static int64_t     GetSteadyMilliSeconds();
    static std::string GetSystemTime(const std::string& format = "%H-%M-%S");

    static bool        BytesEquals(void* pa, void* pb, int size);
    static std::string BuildStreamPath(const std::string& template_path,
                                       const std::string& vhost,
                                       const std::string& app,
                                       const std::string& stream);
    static std::string BuildIndexPath(const std::string& template_path);
    static std::string
                       BuildTimestampPath(const std::string& template_path,
                                          const std::string& format = "%H-%M-%S");
    static std::string BuildIndexSuffixPath(const std::string& template_path,
                                            int                index);
    static bool        IsFileExist(const std::string& path);
    static int         CreateDirRecursively(const std::string& dir);
    static int         ReadBit(BitBufferManager* manager, int8_t& v);
    static int         ReadUEV(BitBufferManager* manager, int32_t& v);
};

#define rs_auto_free(class_name, instance)                                     \
    __impl_AutoFree<class_name> __auto_free_##instance(&instance, false)
#define rs_auto_freea(class_name, instance)                                    \
    __impl_AutoFree<class_name> __auto_free_##instance(&instance, true)

template <typename T> class __impl_AutoFree {
  public:
    __impl_AutoFree(T** p, bool is_array) : p_(p), is_array_(is_array) {}

    ~__impl_AutoFree()
    {
        if (*p_) {
            if (is_array_) {
                delete[] * p_;
            }
            else {
                delete *p_;
            }
            *p_ = nullptr;
        }
    }

  private:
    T**  p_;
    bool is_array_;
};
#endif