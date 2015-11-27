
#include "EntryHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

EntryHandler::EntryHandler(ServerHandler& serverHandler, Server& server, Server::Handle& handle, uint32_t connectionId, uint32_t addr, uint16_t port) :
  serverHandler(serverHandler), server(server), handle(handle), connectionId(connectionId), addr(addr), port(port), suspended(false), suspendedByUplink(false)
{
  server.setUserData(handle, this);
}

EntryHandler::~EntryHandler()
{
  server.close(handle);
}

void_t EntryHandler::sendData(const byte_t* data, size_t size)
{
  if(!server.write(handle, data, size))
    serverHandler.sendSuspendEndpoint(connectionId);
}
void_t EntryHandler::suspend()
{
  suspended = true;
  server.suspend(handle);
}

void_t EntryHandler::resume()
{
  suspended = false;
  if(!suspendedByUplink)
    server.resume(handle);
}

void_t EntryHandler::suspendByUplink()
{
  suspendedByUplink = true;
  server.suspend(handle);
}

void_t EntryHandler::resumeByUplink()
{
  suspendedByUplink = false;
  if(!suspended)
    server.resume(handle);
}

void_t EntryHandler::closedClient()
{
  serverHandler.removeEntry(connectionId);
}

void_t EntryHandler::readClient()
{
  byte_t buffer[RECV_BUFFER_SIZE];
  size_t size;
  if (server.read(handle, buffer, sizeof(buffer), size))
    serverHandler.sendDataToUplink(connectionId, buffer, size);
}

void_t EntryHandler::writeClient()
{
  serverHandler.sendResumeEndpoint(connectionId);
}
