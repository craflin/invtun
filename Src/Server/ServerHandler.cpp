
#include <nstd/Console.h>
#include <nstd/Socket/Socket.h>

#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "EntryHandler.h"

ServerHandler::ServerHandler(Server& server, const String& secret) :
  server(server), tunnelListener(0), secret(secret), uplink(0), nextConnectionId(1), suspendedAllEntries(false) {}

ServerHandler::~ServerHandler()
{
  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  delete uplink;
  if(tunnelListener)
    server.close(*tunnelListener);
  for(HashMap<Server::Handle*, uint16_t>::Iterator i = inboundListeners.begin(), end = inboundListeners.end(); i != end; ++i)
    server.close(*i.key());
}

bool_t ServerHandler::listen(uint16_t port)
{
  if(tunnelListener)
    return false;
  tunnelListener = server.listen(port, this);
  if(!tunnelListener)
    return false;
  return true;
}

bool_t ServerHandler::listen(uint16_t port, uint16_t mappedPort)
{
  Server::Handle* handle = server.listen(port, this);
  if(!handle)
    return false;
  inboundListeners.append(handle, mappedPort);
  return true;
}

bool_t ServerHandler::removeUplink()
{
  if(!uplink)
    return false;
  Console::printf("Closed uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(uplink->getAddr()), uplink->getPort());
  delete uplink;
  uplink= 0;

  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  entries.clear();
}

bool_t ServerHandler::removeEntry(uint32_t connectionId)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  EntryHandler* entry = *it;

  Console::printf("Closed entry connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(entry->getAddr()), entry->getPort());

  delete entry;
  entries.remove(it);

  if(uplink)
    uplink->sendDisconnect(connectionId);
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

void_t ServerHandler::sendSuspendEndpoint(uint32_t connectionId)
{
  if(!uplink)
    return;
  uplink->sendSuspend(connectionId);
}

void_t ServerHandler::sendResumeEndpoint(uint32_t connectionId)
{
  if(!uplink)
    return;
  uplink->sendResume(connectionId);
}

void_t ServerHandler::suspendEntry(uint32_t connectionId)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return;
  EntryHandler* entry = *it;
  entry->suspendByUplink();
}

void_t ServerHandler::resumeEntry(uint32_t connectionId)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return;
  EntryHandler* entry = *it;
  entry->resumeByUplink();
}

void_t ServerHandler::suspendAllEntries()
{
  if(suspendedAllEntries)
    return;
  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    (*i)->suspend();
  suspendedAllEntries = true;
}

void_t ServerHandler::resumeAllEntries()
{
  if(!suspendedAllEntries)
    return;
  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    (*i)->resume();
  suspendedAllEntries = false;
}

void_t ServerHandler::acceptClient(Server::Handle& listenerHandle)
{
  uint32_t addr;
  uint16_t port;
  Server::Handle* handle = server.accept(listenerHandle, 0, &addr, &port);
  if(!handle)
    return;

  if(&listenerHandle == tunnelListener)
  {
    if(uplink)
    {
      server.close(*handle);
      return;
    }
    Console::printf("Accepted uplink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);
    uplink = new UplinkHandler(*this, server, *handle, addr, port);
  }
  else
  {
    if(!uplink)
    {
      server.close(*handle);
      return;
    }
    uint32_t connectionId = nextConnectionId++;
    uint16_t mappedPort = mapPort(listenerHandle);
    if(!uplink->sendConnect(connectionId, mappedPort))
    {
      server.close(*handle);
      return;
    }
    Console::printf("Accepted entry connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);
    EntryHandler* entry = new EntryHandler(*this, server, *handle, connectionId, addr, port);
    entries.append(connectionId, entry);
  }
}
