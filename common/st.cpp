#include <common/st.hpp>
#include <common/error.hpp>
#include <common/log.hpp>
#include <common/utils.hpp>

int32_t STInit()
{
    int32_t ret = ERROR_SUCCESS;
    if (st_set_eventsys(ST_EVENTSYS_ALT) == -1)
    {
        ret = ERROR_ST_SET_EPOLL;
        rs_error("st_set_eventsys use %s failed,ret=%d", st_get_eventsys_name(), ret);
        return ret;
    }

    if (st_init() != 0)
    {
        ret = ERROR_ST_INITIALIZE;
        rs_error("st_init failed,ret=%d", ret);
        return ret;
    }

    return ret;
}

void STCloseFd(st_netfd_t &stfd)
{
    if (stfd)
    {
        int32_t err = st_netfd_close(stfd);
        rs_assert(err != -1);
        stfd = nullptr;
    }
}