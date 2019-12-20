
#include <repo_version.h>
#include <common/log/fast_log.hpp>
#include <common/thread/thread.hpp>
#include <servers/rtmp_server.hpp>
#include <common/error.hpp>

#include <servers/listener.hpp>

ILog *_log = new FastLog;
IThreadContext *_context = new ThreadContext;
RTMPServer *_server = new RTMPServer;

int RunMaster()
{
   int ret = ERROR_SUCCESS;
   if ((ret = _server->InitializeST()) != ERROR_SUCCESS)
   {
      return ret;
   }
}

int main(int argc, char *argv[])
{
   RunMaster();

   TCPListener lis(nullptr, "0.0.0.0", 80);
   lis.Listen();

   // while(1)
   st_usleep(1000000);
}