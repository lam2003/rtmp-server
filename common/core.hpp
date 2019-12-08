#ifndef RS_CORE_HPP
#define RS_CORE_HPP

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
#include <assert.h>

#define rs_assert(expression) assert(expression)

#define rs_freep(p)  \
    if (p)           \
    {                \
        delete p;    \
        p = nullptr; \
    }                \
    (void)0

#define rs_freepa(pa) \
    if (pa)           \
    {                 \
        delete[] pa;  \
        pa = nullptr; \
    }                 \
    (void)0

#endif