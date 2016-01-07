
#include <nstd/Log.h>
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

#include "EntryHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

EntryHandler::EntryHandler(ServerHandler& serverHandler, Server& server, uint32_t connectionId)
  : serverHandler(serverHandler), server(server), handle(0), connectionId(connectionId), suspended(false), suspendedByUplink(false) {}

EntryHandler::~EntryHandler()
{
  if(handle)
  {
    Log::infof("Closed entry connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
    server.close(*handle);
  }
}

bool_t EntryHandler::accept(Server::Handle& listenerHandle)
{
  ASSERT(!handle);
  handle = server.accept(listenerHandle, this, &addr, &port);
  if(!handle)
    return false;
  Log::infof("Accepted entry connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
  return true;
}

void_t EntryHandler::sendData(const byte_t* data, size_t size)
{
  size_t postponed;
  if(server.write(*handle, data, size, &postponed) && postponed)
    serverHandler.sendSuspendEndpoint(connectionId);
}
void_t EntryHandler::suspend()
{
  suspended = true;
  server.suspend(*handle);
}

void_t EntryHandler::resume()
{
  suspended = false;
  if(!suspendedByUplink)
    server.resume(*handle);
}

void_t EntryHandler::suspendByUplink()
{
  suspendedByUplink = true;
  server.suspend(*handle);
}

void_t EntryHandler::resumeByUplink()
{
  suspendedByUplink = false;
  if(!suspended)
    server.resume(*handle);
}

void_t EntryHandler::closedClient()
{
  serverHandler.removeEntry(connectionId);
}

void_t EntryHandler::readClient()
{
  byte_t buffer[RECV_BUFFER_SIZE];
  size_t size;
  if(server.read(*handle, buffer, sizeof(buffer), size))
    serverHandler.sendDataToUplink(connectionId, buffer, size);
}

void_t EntryHandler::writeClient()
{
  serverHandler.sendResumeEndpoint(connectionId);
}
