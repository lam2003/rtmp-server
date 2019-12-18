
#include <repo_version.h>
#include <stdio.h>
#include <common/log/fast_log.hpp>
#include <common/thread/thread.hpp>
#include <common/io/st.hpp>
ILog *_log = new FastLog;
IThreadContext *_context = new ThreadContext;

int main(int argc, char *argv[])
{
    StInit();
    internal::IThreadHandler handler;
    internal::Thread thread("test_thread", &handler, 0, false);

    thread.Start();
    st_usleep(100000000000);
    thread.Stop();
}