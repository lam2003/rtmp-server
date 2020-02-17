/*
 * @Author: linmin
 * @Date: 2020-02-09 15:47:54
 * @LastEditTime : 2020-02-13 14:05:43
 */
#ifndef RS_CORE_HPP
#define RS_CORE_HPP

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
#include <assert.h>

#define RS_VERSION_MAJOR 1
#define RS_VERSION_MINOR 0
#define RS_VERSION_REVISION 0

#define RS_XSTR(v) RS_INTERNAL_STR(v)
#define RS_INTERNAL_STR(v) #v

#define RS_KEY "RS"
#define RS_CODE "ppking"

#define RS_VERSION RS_XSTR(RS_VERSION_MAJOR) "." RS_XSTR(RS_VERSION_MINOR) "." RS_XSTR(RS_VERSION_REVISION) 
#define RS_SERVER RS_KEY "/" RS_VERSION "(" RS_CODE ")"

#endif 