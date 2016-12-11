
#include <nstd/Log.h>
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

#include "EndpointHandler.h"
#include "ClientHandler.h"

EndpointHandler::EndpointHandler(ClientHandler& clientHandler, Server& server, uint32 connectionId)
  : clientHandler(clientHandler), server(server), handle(0), connectionId(connectionId), connected(false), suspended(false), suspendedByDownlink(false) {}

EndpointHandler::~EndpointHandler()
{
  if(handle)
  {
    if(connected)
      Log::infof("Closed endpoint connection with %s:%hu", (const char*)Socket::inetNtoA(Socket::loopbackAddr), port);
    server.close(*handle);
  }
}

bool EndpointHandler::connect(uint16 port)
{
  ASSERT(!handle);
  Log::infof("Establishing endpoint connection with %s:%hu...", (const char*)Socket::inetNtoA(Socket::loopbackAddr), port);
  handle = server.connect(Socket::loopbackAddr, port, this);
  if(!handle)
    return false;
  this->port = port;
  return true;
}

void EndpointHandler::sendData(byte* data, size_t size)
{
  if(connected)
  {
    size_t postponed;
    if(server.write(*handle, data, size, &postponed) && postponed)
      clientHandler.sendSuspendEntry(connectionId);
  }
  else
  {
    sendBuffer.append(data, size);
    clientHandler.sendSuspendEntry(connectionId);
  }
}

void EndpointHandler::suspend()
{
  suspended = true;
  server.suspend(*handle);
}

void EndpointHandler::resume()
{
  suspended = false;
  if(!suspendedByDownlink)
    server.resume(*handle);
}

void EndpointHandler::suspendByDownlink()
{
  suspendedByDownlink = true;
  server.suspend(*handle);
}

void EndpointHandler::resumeByDownlink()
{
  suspendedByDownlink = false;
  if(!suspended)
    server.resume(*handle);
}

void EndpointHandler::openedClient()
{
  Log::infof("Established endpoint connection with %s:%hu", (const char*)Socket::inetNtoA(Socket::loopbackAddr), port);

  connected = true;
  if(!sendBuffer.isEmpty())
  {
    server.write(*handle, sendBuffer, sendBuffer.size());
    sendBuffer.free();
    clientHandler.sendResumeEntry(connectionId);
  }
}

void EndpointHandler::abolishedClient()
{
  Log::infof("Could not establish endpoint connection with %s:%hu: %s", (const char*)Socket::inetNtoA(Socket::loopbackAddr), port, (const char*)Socket::getErrorString());

  clientHandler.removeEndpoint(connectionId);
}

void EndpointHandler::closedClient()
{
  clientHandler.removeEndpoint(connectionId);
}

void EndpointHandler::readClient()
{
  byte buffer[RECV_BUFFER_SIZE];
  size_t size;
  if(server.read(*handle, buffer, sizeof(buffer), size))
    clientHandler.sendDataToDownlink(connectionId, buffer, size);
}

void EndpointHandler::writeClient()
{
  clientHandler.sendResumeEntry(connectionId);
}
