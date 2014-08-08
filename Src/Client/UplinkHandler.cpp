
#include "UplinkHandler.h"
#include "ServerHandler.h"

UplinkHandler::UplinkHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId, uint16_t port) : serverHandler(serverHandler), client(client), connectionId(connectionId), port(port), connected(false)
{
  client.setListener(this);
}

UplinkHandler::~UplinkHandler()
{
  client.setListener(0);
  client.close();
}

void_t UplinkHandler::sendData(byte_t* data, size_t size)
{
  if(connected)
    client.send(data, size);
  else
    sendBuffer.append(data, size);
}

void_t UplinkHandler::establish()
{
  client.send(sendBuffer, sendBuffer.size());
  sendBuffer.free();
  connected = true;
}

size_t UplinkHandler::handle(byte_t* data, size_t size)
{
  if(!serverHandler.sendDataToDownlink(connectionId, data, size))
    client.close();
  return size;
}
