
#include <nstd/Console.h>
#include <nstd/Log.h>
#include <nstd/Process.h>
#include <nstd/List.h>
#include <nstd/Error.h>
#include <nstd/Socket/Socket.h>

#include "ServerHandler.h"

int_t main(int_t argc, char_t* argv[])
{
  String listenAddr;
  uint16_t uplinkPort = 1231;
  String secret("Gr33nshoes");
  List<String> ports;
  String logFile;

  // parse parameters
  {
    Process::Option options[] = {
        {'b', "daemon", Process::argumentFlag | Process::optionalFlag},
        {'l', "listen", Process::argumentFlag},
        {'s', "secret", Process::argumentFlag},
        {'h', "help", Process::optionFlag},
    };
    Process::Arguments arguments(argc, argv, options);
    int_t character;
    String argument;
    while(arguments.read(character, argument))
      switch(character)
      {
      case 'b':
        logFile = argument.isEmpty() ? String("invtuns.log") : argument;
        break;
      case 'l':
        listenAddr = argument;
        break;
      case 's':
        secret = argument;
        break;
      case 0:
        ports.append(argument);
        break;
      case '?':
        Console::errorf("Unknown option: %s.\n", (const char_t*)argument);
        return -1;
      case ':':
        Console::errorf("Option %s required an argument.\n", (const char_t*)argument);
        return -1;
      default:
        Console::errorf("Usage: %s [-b] [<addr>:]<port>[-<mapped port>] [...]\n\
  -b, --daemon[=<file>]       Detach from calling shell and write output to <file>.\n\
  -l, --listen=<addr>:<port>  Set listen interface IP address and port.\n\
  -s, --secret=<secret>       Set passphrase to protect the uplink connection.\n", argv[0]);
        return -1;
      }
  }

#ifndef _WIN32
  if(!logFile.isEmpty())
  {
    Log::infof("Starting as daemon...");
    if(!Process::daemonize(logFile))
    {
      Log::errorf("Could not daemonize process: %s", (const char_t*)Error::getErrorString());
      return -1;
    }
  }
#endif

  // start server
  Server server;
  server.setKeepAlive(true);
  server.setNoDelay(true);
  server.setSendBufferSize(SEND_BUFFER_SIZE);
  server.setReceiveBufferSize(RECV_BUFFER_SIZE);
  ServerHandler serverHandler(server);
  uint32_t ip = Socket::inetAddr(listenAddr, &uplinkPort);
  if(!serverHandler.listen(ip, uplinkPort, secret))
  {
    Log::errorf("Could not listen on port %hu: %s", uplinkPort, (const char_t*)Socket::getErrorString());
    return -1;
  }
  Log::infof("Listening for uplink on port %hu...", uplinkPort);
  for (List<String>::Iterator i = ports.begin(), end = ports.end(); i != end; ++i)
  {
    String argument = *i;
    const char_t* sep = argument.find('-');
    uint16_t mappedPort = 0;
    if(sep)
    {
      mappedPort = String::toUInt(sep + 1);
      argument.resize(sep - (const char_t*)argument);
    }
    uint16_t port = 0;
    uint32_t addr = Socket::inetAddr(argument, &port);
    if(!sep)
      mappedPort = port;
    if(!serverHandler.listen(addr, port, mappedPort))
    {
      Log::errorf("Could not listen on port %hu: %s", port, (const char_t*)Socket::getErrorString());
      return -1;
    }
    Log::infof("Listening for entries on port %hu...", port);
  }

  for(Server::Event event; server.poll(event);)
  {
    Callback* callback = (Callback*)event.userData;
    switch(event.type)
    {
    case Server::Event::failType:
      callback->abolishedClient();
      break;
    case Server::Event::openType:
      callback->openedClient();
      break;
    case Server::Event::readType:
      callback->readClient();
      break;
    case Server::Event::writeType:
      callback->writeClient();
      break;
    case Server::Event::closeType:
      callback->closedClient();
      break;
    case Server::Event::acceptType:
      callback->acceptClient(*event.handle);
      break;
    case Server::Event::timerType:
      callback->executeTimer();
      break;
    }
  }
  Log::errorf("Could not run poll loop: %s", (const char_t*)Socket::getErrorString());
  return 0;
}
