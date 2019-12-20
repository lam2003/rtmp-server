#ifndef RS_ST_HPP
#define RS_ST_HPP

#include <common/core.hpp>

#include <st.h>

extern int STInit();

extern void STCloseFd(st_netfd_t &stfd);

#endif