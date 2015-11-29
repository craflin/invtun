
#include <nstd/Console.h>
#include <nstd/Log.h>
#include <nstd/Process.h>
#include <nstd/Socket/Socket.h>

#include "ClientHandler.h"

int_t main(int_t argc, char_t* argv[])
{
  uint16_t uplinkPort = 1231;
  String secret("Gr33nshoes");
  String address;
  String logFile;

  // parse parameters
  {
    Process::Option options[] = {
        {'b', "daemon", Process::argumentFlag | Process::optionalFlag},
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
        logFile = argument.isEmpty() ? String("invtunc.log") : argument;
        break;
      case 's':
        secret = argument;
        break;
      case 0:
        address = argument;
        break;
      case '?':
        Console::errorf("Unknown option: %s.\n", (const char_t*)argument);
        return -1;
      case ':':
        Console::errorf("Option %s required an argument.\n", (const char_t*)argument);
        return -1;
      default:
        Console::errorf("Usage: %s [-b] <addr>:<port>\n\
  -b, --daemon[=<file>]   Detach from calling shell and write output to <file>.\n\
  -s, --secret=<secret>   Set passphrase to protect the uplink connection.\n", argv[0]);
        return -1;
      }
  }

#ifndef _WIN32
  if(!logFile.isEmpty())
  {
    Log::infof("Starting as daemon...");
    if(!Process::daemonize(logFile))
    {
      Log::errorf("error: Could not daemonize process: %s", (const char_t*)Error::getErrorString());
      return -1;
    }
  }
#endif

  // start select loop
  Server server;
  server.setKeepAlive(true);
  server.setNoDelay(true);
  server.setSendBufferSize(SEND_BUFFER_SIZE);
  server.setReceiveBufferSize(RECV_BUFFER_SIZE);
  uint32_t addr = Socket::inetAddr(address, &uplinkPort);
  ClientHandler clientHandler(server, addr, uplinkPort, secret);
  if(!clientHandler.connect())
  {
    Log::errorf("Could not connect to %s:%hu: %s", (const char_t*)address, uplinkPort, (const char_t*)Socket::getErrorString());
    return -1;
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
  return -1;
}
