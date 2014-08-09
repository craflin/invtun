
#include <nstd/Thread.h>
#include <nstd/Console.h>

#include "ServerHandler.h"
#include "EndpointHandler.h"
#include "DownlinkHandler.h"

ServerHandler::ServerHandler(Server& server, uint32_t addr, uint16_t port, const String& secret) : server(server), addr(addr), port(port), secret(secret)
{
  server.setListener(this);
}

ServerHandler::~ServerHandler()
{
  server.setListener(0);
  for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
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
    Console::printf("Established downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
  else
  {
    Console::printf("Established endpoint connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
  }
}

void_t ServerHandler::closedClient(Server::Client& client)
{
  if(client.getListener() == downlink)
  {
    Console::printf("Closed downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    delete downlink;
    downlink = 0;

    server.addTimer(10 * 1000); // start reconnect timer

    for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
      delete *i;
    endpoints.clear();
  }
  else
  {
    Console::printf("Closed endpoint connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    EndpointHandler* endpoint = (EndpointHandler*)client.getListener();
    uint32_t connectionId = endpoint->getConnectionId();
    if(downlink)
      downlink->sendDisconnect(connectionId);
    endpoints.remove(connectionId);
    delete endpoint;
  }
}

void_t ServerHandler::abolishedClient(Server::Client& client)
{
  if(client.getListener() == downlink)
  {
    Console::printf("Could not establish downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    delete downlink;
    downlink = 0;

    server.addTimer(10 * 1000); // start reconnect timer
  }
  else
  {
    EndpointHandler* endpoint = (EndpointHandler*)client.getListener();
    Console::printf("Could not establish endpoint connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    uint32_t connectionId = endpoint->getConnectionId();
    if(downlink)
      downlink->sendDisconnect(connectionId);
    endpoints.remove(connectionId);
    delete endpoint;
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
  EndpointHandler* endpoint = new EndpointHandler(*this, *client, connectionId);
  endpoints.append(connectionId, endpoint);
  return true;
}

bool_t ServerHandler::removeConnection(uint32_t connectionId)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  EndpointHandler* endpoint = *it;
  endpoints.remove(it);
  delete endpoint;
  return true;
}

bool_t ServerHandler::sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size)
{
  if(!downlink)
    return false;
  downlink->sendData(connectionId, data, size);
  return true;
}

bool_t ServerHandler::sendDataToEndpoint(uint32_t connectionId, byte_t* data, size_t size)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  EndpointHandler* endpoint = *it;
  endpoint->sendData(data, size);
  return true;
}
