
#include <repo_version.h>
#include <common/fast_log.hpp>
#include <common/thread.hpp>
#include <app/server.hpp>
#include <common/error.hpp>

#include <app/listener.hpp>

ILog *_log = new FastLog;
IThreadContext *_context = new ThreadContext;
Server *_server = new Server;

int32_t RunMaster()
{
   int32_t ret = ERROR_SUCCESS;
   if ((ret = _server->InitializeST()) != ERROR_SUCCESS)
   {
      return ret;
   }
}

int32_t main(int32_t argc, char *argv[])
{
   RunMaster();


   RTMPStreamListener listener(_server, ListenerType::RTMP_STEAM);

   listener.Listen("0.0.0.0", 80);

   // while(1)
   st_usleep(1000000);

}