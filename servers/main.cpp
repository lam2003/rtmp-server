
#include <repo_version.h>
#include <stdio.h>
#include <common/log/fast_log.hpp>

ILog *_log = new FastLog;
IThreadContext *_context = new ThreadContext;
int main(int argc, char *argv[])
{
    
    printf("REPO_VERSION:%s\n", REPO_VERSION);
    printf("REPO_DATE:%s\n", REPO_DATE);
    printf("REPO_HASH:%s\n", REPO_HASH);
}