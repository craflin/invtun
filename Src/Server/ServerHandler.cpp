
#include <nstd/Console.h>

#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "EntryHandler.h"

ServerHandler::ServerHandler(uint16_t uplinkPort, const String& secret, const HashMap<uint16_t, uint16_t>& ports) : uplinkPort(uplinkPort), secret(secret), uplink(0), nextConnectionId(1)
{
  for(HashMap<uint16_t, uint16_t>::Iterator i = ports.begin(), end = ports.end(); i != end; ++i)
    if(i.key() != *i)
      portMapping.append(i.key(), *i);
}

ServerHandler::~ServerHandler()
{
  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  delete uplink;
}

bool_t ServerHandler::removeClient(uint32_t connectionId)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  EntryHandler* entry = *it;
  entries.remove(it);
  delete entry;
  return true;
}

bool_t ServerHandler::sendDataToEntry(uint32_t connectionId, byte_t* data, size_t size)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  EntryHandler* entry = *it;
  entry->sendData(data, size);
  return true;
}

bool_t ServerHandler::sendDataToUplink(uint32_t connectionId, byte_t* data, size_t size)
{
  if(!uplink)
    return false;
  return uplink->sendData(connectionId, data, size);
}

uint16_t ServerHandler::mapPort(uint16_t port)
{
  HashMap<uint16_t, uint16_t>::Iterator it = portMapping.find(port);
  if(it == portMapping.end())
    return port;
  return *it;
}

void_t ServerHandler::acceptedClient(Server::Client& client, uint16_t localPort)
{
  if(localPort == uplinkPort)
  {
    Console::printf("Accepted uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    delete uplink;
    uplink = new UplinkHandler(*this, client);
  }
  else
  {
    if(!uplink)
    {
      client.close();
      return;
    }
    uint32_t connectionId = nextConnectionId++;
    uint16_t mappedPort = mapPort(localPort);
    if(!uplink->sendConnect(connectionId, mappedPort))
    {
      client.close();
      return;
    }
    Console::printf("Accepted entry connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());
    EntryHandler* entry = new EntryHandler(*this, client, connectionId);
    entries.append(connectionId, entry);
  }
}

void_t ServerHandler::closedClient(Server::Client& client)
{
  if(client.getListener() == uplink)
  {
    Console::printf("Closed uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());

    delete uplink;
    uplink = 0;

    for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
      delete *i;
    entries.clear();
  }
  else
  {
    Console::printf("Closed entry connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(client.getAddr()), client.getPort());

    EntryHandler* entry = (EntryHandler*)client.getListener();
    uint32_t connectionId = entry->getConnectionId();
    entries.remove(connectionId);
    if(uplink)
      uplink->sendDisconnect(connectionId);
    delete entry;
  }
}
