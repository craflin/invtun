
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
  ASSERT(!downlink);
  Console::printf("Establishing downlink connection with %s:%hu...\n", (const char_t*)Socket::inetNtoA(addr), port);
  Server::Client* client = server.connect(addr, port);
  if(!client)
    return false;
  downlink = new DownlinkHandler(*this, *client);
  return true;
}

void_t ServerHandler::establishedClient(Server::Client& client)
{
  if(client.getListener() == downlink)
    Console::printf("Established downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);
  else
  {
    UplinkHandler* uplink = (UplinkHandler*)client.getListener();
    Console::printf("Established downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(Socket::loopbackAddr), uplink->getPort());
  }
}

void_t ServerHandler::closedClient(Server::Client& client)
{
  if(client.getListener() == downlink)
  {
    Console::printf("Closed downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);
    delete downlink;
    downlink = 0;

    server.addTimer(10 * 1000); // start reconnect timer

    for(HashMap<uint32_t, UplinkHandler*>::Iterator i = uplinks.begin(), end = uplinks.end(); i != end; ++i)
      delete *i;
    uplinks.clear();
  }
  else
  {
    UplinkHandler* uplink = (UplinkHandler*)client.getListener();
    Console::printf("Closed uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(Socket::loopbackAddr), uplink->getPort());
    uint32_t connectionId = uplink->getConnectionId();
    if(downlink)
      downlink->sendDisconnect(connectionId);
    uplinks.remove(connectionId);
    delete uplink;
  }
}

void_t ServerHandler::abolishedClient(Server::Client& client)
{
  if(client.getListener() == downlink)
  {
    Console::printf("Could not establish downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);
    delete downlink;
    downlink = 0;

    server.addTimer(10 * 1000); // start reconnect timer
  }
  else
  {
    UplinkHandler* uplink = (UplinkHandler*)client.getListener();
    Console::printf("Could not establish uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(Socket::loopbackAddr), uplink->getPort());
    uint32_t connectionId = uplink->getConnectionId();
    if(downlink)
      downlink->sendDisconnect(connectionId);
    uplinks.remove(connectionId);
    delete uplink;
  }
}

void_t ServerHandler::executedTimer(Server::Timer& timer)
{
  Console::printf("Executed timer\n");
  ASSERT(!downlink);
  if(!connect())
    server.addTimer(10 * 1000);
}

bool_t ServerHandler::createConnection(uint32_t connectionId, uint16_t port)
{
  if(!downlink)
    return false;
  Server::Client* client = server.connect(Socket::loopbackAddr, port);
  if(!client)
    return false;
  UplinkHandler* uplinkHandler = new UplinkHandler(*this, *client, connectionId, port);
  uplinks.append(connectionId, uplinkHandler);
  return true;
}

bool_t ServerHandler::removeConnection(uint32_t connectionId)
{
  HashMap<uint32_t, UplinkHandler*>::Iterator it = uplinks.find(connectionId);
  if(it == uplinks.end())
    return false;
  UplinkHandler* uplink = *it;
  uplinks.remove(it);
  delete uplink;
  return true;
}

bool_t ServerHandler::sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size)
{
  if(!downlink)
    return false;
  downlink->sendData(connectionId, data, size);
  return true;
}

bool_t ServerHandler::sendDataToUplink(uint32_t connectionId, byte_t* data, size_t size)
{
  HashMap<uint32_t, UplinkHandler*>::Iterator it = uplinks.find(connectionId);
  if(it == uplinks.end())
    return false;
  UplinkHandler* uplink = *it;
  uplink->sendData(data, size);
  return true;
}
