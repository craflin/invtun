
#include "EndpointHandler.h"
#include "ServerHandler.h"

EndpointHandler::EndpointHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId, uint16_t port) : serverHandler(serverHandler), client(client), connectionId(connectionId), port(port), connected(false)
{
  client.setListener(this);
}

EndpointHandler::~EndpointHandler()
{
  client.setListener(0);
  client.close();
}

void_t EndpointHandler::sendData(byte_t* data, size_t size)
{
  if(connected)
    client.send(data, size);
  else
    sendBuffer.append(data, size);
}

void_t EndpointHandler::establish()
{
  client.send(sendBuffer, sendBuffer.size());
  sendBuffer.free();
  connected = true;
}

size_t EndpointHandler::handle(byte_t* data, size_t size)
{
  if(!serverHandler.sendDataToDownlink(connectionId, data, size))
    client.close();
  return size;
}
