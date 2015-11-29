
#include <nstd/Log.h>
#include <nstd/Socket/Socket.h>

#include "EndpointHandler.h"
#include "ClientHandler.h"

EndpointHandler::EndpointHandler(ClientHandler& clientHandler, Server& server, Server::Handle& handle, uint32_t connectionId, uint16_t port) :
  clientHandler(clientHandler), server(server), handle(handle), connectionId(connectionId), port(port), connected(false), suspended(false), suspendedByDownlink(false)
{
  server.setUserData(handle, this);
}

EndpointHandler::~EndpointHandler()
{
  server.close(handle);
}

void_t EndpointHandler::sendData(byte_t* data, size_t size)
{
  if(connected)
  {
    size_t postponed;
    if(server.write(handle, data, size, &postponed) && postponed)
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
  server.suspend(handle);
}

void_t EndpointHandler::resume()
{
  suspended = false;
  if(!suspendedByDownlink)
    server.resume(handle);
}

void_t EndpointHandler::suspendByDownlink()
{
  suspendedByDownlink = true;
  server.suspend(handle);
}

void_t EndpointHandler::resumeByDownlink()
{
  suspendedByDownlink = false;
  if(!suspended)
    server.resume(handle);
}

void_t EndpointHandler::openedClient()
{
  Log::infof("Established endpoint connection with %s:%hu", (const char_t*)Socket::inetNtoA(Socket::loopbackAddr), port);

  connected = true;
  if(!sendBuffer.isEmpty())
  {
    server.write(handle, sendBuffer, sendBuffer.size());
    sendBuffer.free();
    clientHandler.sendResumeEntry(connectionId);
  }
}

void_t EndpointHandler::abolishedClient()
{
  Log::infof("Could not establish endpoint connection with %s:%hu: %s", (const char_t*)Socket::inetNtoA(Socket::loopbackAddr), port, (const char_t*)Socket::getErrorString());

  clientHandler.removeEndpoint(connectionId);
}

void_t EndpointHandler::closedClient()
{
  clientHandler.removeEndpoint(connectionId);
}

void_t EndpointHandler::readClient()
{
  byte_t buffer[RECV_BUFFER_SIZE];
  size_t size;
  if(server.read(handle, buffer, sizeof(buffer), size))
    clientHandler.sendDataToDownlink(connectionId, buffer, size);
}

void_t EndpointHandler::writeClient()
{
  clientHandler.sendResumeEntry(connectionId);
}
