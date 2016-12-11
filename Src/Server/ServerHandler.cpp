
#include <nstd/Log.h>
#include <nstd/Socket/Socket.h>

#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "EntryHandler.h"

ServerHandler::ServerHandler(Server& server) :
  server(server), tunnelListener(0), uplink(0), nextConnectionId(1), suspendedAllEntries(false) {}

ServerHandler::~ServerHandler()
{
  for(HashMap<uint32, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  delete uplink;
  if(tunnelListener)
    server.close(*tunnelListener);
  for(HashMap<Server::Handle*, uint16>::Iterator i = inboundListeners.begin(), end = inboundListeners.end(); i != end; ++i)
    server.close(*i.key());
}

bool ServerHandler::listen(uint32 addr, uint16 port, const String& secret)
{
  if(tunnelListener)
    return false;
  this->secret = secret;
  tunnelListener = server.listen(addr, port, this);
  if(!tunnelListener)
    return false;
  return true;
}

bool ServerHandler::listen(uint32 addr, uint16 port, uint16 mappedPort)
{
  Server::Handle* handle = server.listen(addr, port, this);
  if(!handle)
    return false;
  inboundListeners.append(handle, mappedPort);
  return true;
}

bool ServerHandler::removeUplink()
{
  if(!uplink)
    return false;
  delete uplink;
  uplink= 0;

  for(HashMap<uint32, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    delete *i;
  entries.clear();
  return true;
}

bool ServerHandler::removeEntry(uint32 connectionId)
{
  HashMap<uint32, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  delete *it;
  entries.remove(it);

  if(uplink)
    uplink->sendDisconnect(connectionId);
  return true;
}

bool ServerHandler::sendDataToEntry(uint32 connectionId, byte* data, size_t size)
{
  HashMap<uint32, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return false;
  EntryHandler* entry = *it;
  entry->sendData(data, size);
  return true;
}

bool ServerHandler::sendDataToUplink(uint32 connectionId, byte* data, size_t size)
{
  if(!uplink)
    return false;
  return uplink->sendData(connectionId, data, size);
}

void ServerHandler::sendSuspendEndpoint(uint32 connectionId)
{
  if(!uplink)
    return;
  uplink->sendSuspend(connectionId);
}

void ServerHandler::sendResumeEndpoint(uint32 connectionId)
{
  if(!uplink)
    return;
  uplink->sendResume(connectionId);
}

void ServerHandler::suspendEntry(uint32 connectionId)
{
  HashMap<uint32, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return;
  EntryHandler* entry = *it;
  entry->suspendByUplink();
}

void ServerHandler::resumeEntry(uint32 connectionId)
{
  HashMap<uint32, EntryHandler*>::Iterator it = entries.find(connectionId);
  if(it == entries.end())
    return;
  EntryHandler* entry = *it;
  entry->resumeByUplink();
}

void ServerHandler::suspendAllEntries()
{
  if(suspendedAllEntries)
    return;
  for(HashMap<uint32, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    (*i)->suspend();
  suspendedAllEntries = true;
}

void ServerHandler::resumeAllEntries()
{
  if(!suspendedAllEntries)
    return;
  for(HashMap<uint32, EntryHandler*>::Iterator i = entries.begin(), end = entries.end(); i != end; ++i)
    (*i)->resume();
  suspendedAllEntries = false;
}

void ServerHandler::acceptClient(Server::Handle& listenerHandle)
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
    uint32 connectionId = nextConnectionId++;
    EntryHandler* entry = new EntryHandler(*this, server, connectionId);
    if(!entry->accept(listenerHandle))
    {
      delete entry;
      return;
    }
    uint16 mappedPort = mapPort(listenerHandle);
    uplink->sendConnect(connectionId, mappedPort);
    entries.append(connectionId, entry);
  }
}
