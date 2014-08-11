
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#endif

#include <nstd/String.h>
#include <nstd/Console.h>

#include "Tools/Server.h"
#include "ClientHandler.h"

int_t main(int_t argc, char_t* argv[])
{
  uint16_t uplinkPort = 1231;
  String secret("Gr33nshoes");
  bool_t background = true;
  String address;

  // parse parameters
  for(int i = 1; i < argc; ++i)
  {
    String arg;
    arg.attach(argv[i], String::length(argv[i]));
    if(arg == "-f")
      background = false;
    else if(!arg.startsWith("-") && address.isEmpty())
      address = arg;
    else
    {
      Console::errorf("Usage: %s <port>[:<mapped port>] [<port>[:<mapped port>] ... ] [-f]\n", argv[0]);
      return -1;
    }
  }

#ifndef _WIN32
  // daemonize process
  if(background)
  {
    const char* logFile = "intunc.log";

    Console::printf("Starting as daemon...\n");

    int fd = open(logFile, O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if(fd == -1)
    {
      Console::errorf("error: Could not open file %s: %s\n", logFile, strerror(errno));
      return -1;
    }
    if(dup2(fd, STDOUT_FILENO) == -1)
    {
      Console::errorf("error: Could not reopen stdout: %s\n", strerror(errno));
      return 0;
    }
    if(dup2(fd, STDERR_FILENO) == -1)
    {
      Console::errorf("error: Could not reopen stdout: %s\n", strerror(errno));
      return 0;
    }
    close(fd);

    pid_t childPid = fork();
    if(childPid == -1)
      return -1;
    if(childPid != 0)
      return 0;
  }
#endif

  // start select loop
  Server server;
  ClientHandler clientHandler(server, Socket::inetAddr(address), uplinkPort, secret);
  if(!clientHandler.connect())
  {
    Console::errorf("error: Could not connect to %s:%hu: %s\n", (const char_t*)address, uplinkPort, (const char_t*)Socket::getErrorString());
    return -1;
  }
  if(!server.process())
  {
    Console::errorf("error: Could not run select loop: %s\n", (const char_t*)Socket::getErrorString());
    return -1;
  }
  return 0;
}
