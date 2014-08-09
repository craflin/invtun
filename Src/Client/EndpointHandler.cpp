
#include "EndpointHandler.h"
#include "ClientHandler.h"

EndpointHandler::EndpointHandler(ClientHandler& clientHandler, Server::Client& client, uint32_t connectionId) : clientHandler(clientHandler), client(client), connectionId(connectionId), connected(false)
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
    sendBuffer.append(data, size); // todo: suspend entry socket as long as connection is not ready
}

void_t EndpointHandler::establish()
{
  client.send(sendBuffer, sendBuffer.size());
  sendBuffer.free();
  connected = true;
}

size_t EndpointHandler::handle(byte_t* data, size_t size)
{
  if(!clientHandler.sendDataToDownlink(connectionId, data, size))
    client.close();
  return size;
}
