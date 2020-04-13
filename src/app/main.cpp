/*
 * @Author: linmin
 * @Date: 2020-02-06 15:01:40
 * @LastEditTime : 2020-02-06 17:59:41
 */

#include <app/server.hpp>
#include <common/config.hpp>
#include <common/error.hpp>
#include <common/listener.hpp>
#include <common/log.hpp>
#include <common/thread.hpp>
#include <protocol/rtmp/source.hpp>
#include <repo_version.h>

#include <gperftools/profiler.h>

#include <signal.h>

ILog*           _log     = new FastLog;
IThreadContext* _context = new ThreadContext;
StreamServer*   _server  = new StreamServer;
Config*         _config  = new Config;

int32_t RunMaster()
{
    int32_t ret = ERROR_SUCCESS;
    if ((ret = _server->InitializeST()) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

void signal_handler(int signo)
{
    if (signo == SIGPIPE) {
        return;
    }

    exit(0);
}

int32_t main(int32_t argc, char* argv[])
{
    rs_info("##################################################");
    rs_info("repo_version:%s", REPO_VERSION);
    rs_info("repo_date:%s", REPO_DATE);
    rs_info("repo_hash:%s", REPO_HASH);
    rs_info("##################################################");

    signal(SIGPIPE, signal_handler);
    signal(SIGINT, signal_handler);

    ProfilerStart("gperf.srs.gcp");

    RunMaster();

    RTMPStreamListener listener(_server, ListenerType::RTMP);

    listener.Listen("0.0.0.0", 1935);

    // while (1)
    // st_usleep(100000000);
    while (true) {
        rtmp::Source::CycleAll();
        st_usleep(1000 * 1000);
    }
    return 0;
}