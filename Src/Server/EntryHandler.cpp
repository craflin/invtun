
#include "EntryHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

EntryHandler::EntryHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId) :
  serverHandler(serverHandler), client(client), connectionId(connectionId)
{
  client.setListener(this);
}

EntryHandler::~EntryHandler()
{
  client.setListener(0);
  client.close();
}

size_t EntryHandler::handle(byte_t* data, size_t size)
{
  if(!serverHandler.sendDataToUplink(connectionId, data, size))
    client.close();
  return size;
}
