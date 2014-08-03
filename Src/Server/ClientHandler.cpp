
#include "ClientHandler.h"
#include "ServerHandler.h"
#include "UplinkHandler.h"

ClientHandler::ClientHandler(uint32_t connectionId, ServerHandler& serverHandler, Server::Client& client) :
  serverHandler(serverHandler), client(client), connectionId(connectionId) {}

size_t ClientHandler::handle(byte_t* data, size_t size)
{
  UplinkHandler* uplink = serverHandler.getUplink();
  if(!uplink || !uplink->sendData(connectionId, data, size))
  {
    client.close();
    return 0;
  }
  return size;
}
