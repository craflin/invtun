
#include <nstd/Log.h>
#include <nstd/Socket/Socket.h>

#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "EntryHandler.h"

ServerHandler::ServerHandler(Server& server) :
  server(server), tunnelListener(0), uplink(0), nextConnectionId(1), suspendedAllEntries(false) {}

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

bool_t ServerHandler::listen(uint32_t addr, uint16_t port, const String& secret)
{
  if(tunnelListener)
    return false;
  this->secret = secret;
  tunnelListener = server.listen(addr, port, this);
  if(!tunnelListener)
    return false;
  return true;
}

bool_t ServerHandler::listen(uint32_t addr, uint16_t port, uint16_t mappedPort)
{
  Server::Handle* handle = server.listen(addr, port, this);
  if(!handle)
    return false;
  inboundListeners.append(handle, mappedPort);
  return true;
}

bool_t ServerHandler::removeUplink()
{
  if(!uplink)
    return false;
  delete uplink;
  uplink= 0;

  for(HashMap<uint32_t, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  entries.clear();
  return true;
}

bool_t ServerHandler::removeEntry(uint32_t connectionId)
{
  HashMap<uint32_t, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  delete *it;
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
  if(&listenerHandle == tunnelListener)
  {
    if(uplink)
    {
      Server::Handle* handle = server.accept(listenerHandle, 0);
      if(handle)
      {
        server.close(*handle);
        return;
      }
    }

    uplink = new UplinkHandler(*this, server);
    if(!uplink->accept(listenerHandle))
    {
      delete uplink;
      uplink = 0;
      return;
    }
  }
  else
  {
    if(!uplink)
    {
      Server::Handle* handle = server.accept(listenerHandle, 0);
      if(handle)
      {
        server.close(*handle);
        return;
      }
    }
    uint32_t connectionId = nextConnectionId++;
    EntryHandler* entry = new EntryHandler(*this, server, connectionId);
    if(!entry->accept(listenerHandle))
    {
      delete entry;
      return;
    }
    uint16_t mappedPort = mapPort(listenerHandle);
    uplink->sendConnect(connectionId, mappedPort);
    entries.append(connectionId, entry);
  }
}
