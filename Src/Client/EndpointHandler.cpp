
#include "EndpointHandler.h"
#include "ClientHandler.h"

EndpointHandler::EndpointHandler(ClientHandler& clientHandler, Server::Client& client, uint32_t connectionId) :
  clientHandler(clientHandler), client(client), connectionId(connectionId), connected(false), suspended(false), suspendedByDownlink(false)
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
  {
    if(!client.send(data, size))
      clientHandler.sendSuspendEntry(connectionId);
  }
  else
  {
    sendBuffer.append(data, size);
    clientHandler.sendSuspendEntry(connectionId);
  }
}

void_t EndpointHandler::suspend()
{
  suspended = true;
  client.suspend();
}

void_t EndpointHandler::resume()
{
  suspended = false;
  if(!suspendedByDownlink)
    client.resume();
}

void_t EndpointHandler::suspendByDownlink()
{
  suspendedByDownlink = true;
  client.suspend();
}

void_t EndpointHandler::resumeByDownlink()
{
  suspendedByDownlink = false;
  if(!suspended)
    client.resume();
}

void_t EndpointHandler::establish()
{
  connected = true;
  if(!sendBuffer.isEmpty())
  {
    client.send(sendBuffer, sendBuffer.size());
    sendBuffer.free();
    clientHandler.sendResumeEntry(connectionId);
  }
}

size_t EndpointHandler::handle(byte_t* data, size_t size)
{
  if(!clientHandler.sendDataToDownlink(connectionId, data, size))
    client.close();
  return size;
}

void_t EndpointHandler::write()
{
  clientHandler.sendResumeEntry(connectionId);
}
