#ifndef RS_ST_HPP
#define RS_ST_HPP

#include <common/core.hpp>

#include <st.h>

extern int StInit();
extern void StCloseFd(st_netfd_t &stfd);

#endif