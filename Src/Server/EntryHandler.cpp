
#include "EntryHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

EntryHandler::EntryHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId) :
  serverHandler(serverHandler), client(client), connectionId(connectionId), suspended(false), suspendedByUplink(false)
{
  client.setListener(this);
}

EntryHandler::~EntryHandler()
{
  client.setListener(0);
  client.close();
}

void_t EntryHandler::sendData(const byte_t* data, size_t size)
{
  client.send(data, size);
  if(!client.flush())
    serverHandler.sendSuspendEndpoint(connectionId);
}
void_t EntryHandler::suspend()
{
  suspended = true;
  client.suspend();
}

void_t EntryHandler::resume()
{
  suspended = false;
  if(!suspendedByUplink)
    client.resume();
}

void_t EntryHandler::suspendByUplink()
{
  suspendedByUplink = true;
  client.suspend();
}

void_t EntryHandler::resumeByUplink()
{
  suspendedByUplink = false;
  if(!suspended)
    client.resume();
}

size_t EntryHandler::handle(byte_t* data, size_t size)
{
  if(!serverHandler.sendDataToUplink(connectionId, data, size))
    client.close();
  return size;
}

void_t EntryHandler::write()
{
  serverHandler.sendResumeEndpoint(connectionId);
}
