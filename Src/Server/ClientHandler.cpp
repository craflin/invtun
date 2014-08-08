
#include "ClientHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

ClientHandler::ClientHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId) :
  serverHandler(serverHandler), client(client), connectionId(connectionId)
{
  client.setListener(this);
}

ClientHandler::~ClientHandler()
{
  client.setListener(0);
  client.close();
}

size_t ClientHandler::handle(byte_t* data, size_t size)
{
  if(!serverHandler.sendDataToUplink(connectionId, data, size))
    client.close();
  return size;
}
