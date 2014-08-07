
#include <nstd/Thread.h>
#include <nstd/Console.h>

#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "DownlinkHandler.h"

ServerHandler::ServerHandler(Server& server, uint32_t addr, uint16_t port, const String& secret) : server(server), addr(addr), port(port), secret(secret)
{
  server.setListener(this);
}

ServerHandler::~ServerHandler()
{
  server.setListener(0);
  for(HashMap<uint32_t, UplinkHandler*>::Iterator i = uplinks.begin(), end = uplinks.end(); i != end; ++i)
    delete *i;
  delete downlink;
}

bool_t ServerHandler::connect()
{
  if(downlink)
    return false;
  Console::printf("Establishing downlink connection with %s:%hu...", (const char_t*)Socket::inetNtoA(addr), port);
  Server::Client* client = server.connect(addr, port);
  if(!client)
    return false;
  downlink = new DownlinkHandler(*this, *client);
  return true;
}

void_t ServerHandler::establishedClient(Server::Client& client, uint32_t addr, uint16_t port)
{
  Console::printf("Established connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
}

void_t ServerHandler::closedClient(Server::Client& client)
{
  Console::printf("Closed connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
}

void_t ServerHandler::abolishedClient(uint32_t addr, uint16_t port)
{
  Console::printf("Could not establish connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
}

void_t ServerHandler::executedTimer(Server::Timer& timer)
{
  Console::printf("Executed timer");
  ASSERT(!downlink);
  if(!connect())
    startReconnectTimer();
}

void_t ServerHandler::startReconnectTimer()
{
  delete downlink;
  downlink = 0;
  server.addTimer(10 * 1000);
}

//bool_t ServerHandler::createConnection(uint32_t connectionId, uint32_t addr, uint16_t port)
//{
//  UplinkHandler* uplinkHandler = new UplinkHandler();
//  if(!server.connect(addr, port, *uplinkHandler))
//  {
//    // todo: send disconnect answer
//    delete uplinkHandler;
//    return false;
//  }
//  uplinks.append(connectionId, uplinkHandler);
//  return true;
//}
