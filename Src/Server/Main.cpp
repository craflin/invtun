
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#endif

#include <nstd/Console.h>
#include <nstd/String.h>

#include "ServerHandler.h"

int_t main(int_t argc, char_t* argv[])
{
  uint16_t uplinkPort = 1231;
  String secret("Gr33nshoes");
  HashMap<uint16_t, uint16_t> ports;
  bool_t background = true;

  // parse parameters
  for(int i = 1; i < argc; ++i)
  {
    String arg;
    arg.attach(argv[i], String::length(argv[i]));
    if(arg == "-f")
      background = false;
    else if(!arg.startsWith("-"))
    {
      uint16_t port = arg.toUInt();
      uint16_t mappedPort = port;
      const char_t* colon = arg.find(':');
      if(colon)
      {
        ++colon;
        mappedPort = arg.substr(colon - (const char_t*)arg).toUInt();
      }
      ports.append(port, mappedPort);
    }
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
    const char* logFile = "intuns.log";

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

  // start server
  Server server;
  ServerHandler serverHandler(uplinkPort, secret, ports);
  server.setListener(&serverHandler);
  if(!server.listen(uplinkPort))
  {
    Console::errorf("error: Could not listen on port %hu: %s\n", uplinkPort, (const char_t*)Socket::getErrorString());
    return -1;
  }
  Console::printf("Listening for uplink on port %hu...\n", uplinkPort);
  for (HashMap<uint16_t, uint16_t>::Iterator i = ports.begin(), end = ports.end(); i != end; ++i)
  {
    uint16_t port = i.key();
    if(!server.listen(port))
    {
      Console::errorf("error: Could not listen on port %hu: %s\n", port, (const char_t*)Socket::getErrorString());
      return -1;
    }
    Console::printf("Listening for entries on port %hu...\n", port);
  }
  if(!server.process())
  {
    Console::errorf("error: Could not run select loop: %s\n", (const char_t*)Socket::getErrorString());
    return -1;
  }
  return 0;
}
