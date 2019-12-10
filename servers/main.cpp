
#include <repo_version.h>
#include <stdio.h>
#include <common/log/fast_log.hpp>

ILog *_log = new FastLog;
IThreadContext *_context = new ThreadContext;
int main(int argc, char *argv[])
{

    rs_verbose("REPO_VERSION:%s", REPO_VERSION);
    rs_info("REPO_DATE:%s", REPO_DATE);
    rs_warn("REPO_DATE:%s", REPO_DATE);
    rs_error("REPO_DATE:%s", REPO_DATE);
    rs_trace("REPO_DATE:%s", REPO_DATE);
    printf("REPO_HASH:%s\n", REPO_HASH);
}