
#include <nstd/Log.h>
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

#include "ClientHandler.h"
#include "EndpointHandler.h"
#include "DownlinkHandler.h"

ClientHandler::ClientHandler(Server& server) :
  server(server), downlink(0), suspendedAlldEnpoints(false), reconnectTimer(0) {}

ClientHandler::~ClientHandler()
{
  for(HashMap<uint32, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    delete *i;
  delete downlink;
}

bool ClientHandler::connect(uint32 addr, uint16 port, const String& secret)
{
  ASSERT(!downlink);
  this->addr = addr;
  this->port = port;
  this->secret = secret;
  downlink = new DownlinkHandler(*this, server);
  if(!downlink->connect(addr, port))
  {
    delete downlink;
    downlink = 0;
    return false;
  }
  return true;
}

void ClientHandler::executeTimer()
{
  //Console::printf("Executed timer\n");
  ASSERT(!downlink);
  if(connect(addr, port, secret))
  {
    if(reconnectTimer)
    {
      server.close(*reconnectTimer);
      reconnectTimer = 0;
    }
  }
}

bool ClientHandler::removeDownlink()
{
  if(!downlink)
    return false;
  delete downlink;
  downlink = 0;

  for(HashMap<uint32, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    delete *i;
  endpoints.clear(),

  reconnectTimer = server.createTimer(10 * 1000, this); // start reconnect timer
  return true;
}

bool ClientHandler::createEndpoint(uint32 connectionId, uint16 port)
{
  EndpointHandler* endpoint = new EndpointHandler(*this, server, connectionId);
  if(!endpoint->connect(port))
  {
    delete endpoint;
    return false;
  }
  endpoints.append(connectionId, endpoint);
  return true;
}

bool ClientHandler::removeEndpoint(uint32 connectionId)
{
  HashMap<uint32, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  delete *it;
  endpoints.remove(it);

  if(downlink)
    downlink->sendDisconnect(connectionId);
  return true;
}

bool ClientHandler::sendDataToDownlink(uint32 connectionId, byte* data, size_t size)
{
  if(!downlink)
    return false;
  downlink->sendData(connectionId, data, size);
  return true;
}

bool ClientHandler::sendDataToEndpoint(uint32 connectionId, byte* data, size_t size)
{
  HashMap<uint32, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  EndpointHandler* endpoint = *it;
  endpoint->sendData(data, size);
  return true;
}

void ClientHandler::sendSuspendEntry(uint32 connectionId)
{
  if(!downlink)
    return;
  downlink->sendSuspend(connectionId);
}

void ClientHandler::sendResumeEntry(uint32 connectionId)
{
  if(!downlink)
    return;
  downlink->sendResume(connectionId);
}

void ClientHandler::suspendEndpoint(uint32 connectionId)
{
  HashMap<uint32, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return;
  EndpointHandler* endpoint = *it;
  endpoint->suspendByDownlink();
}

void ClientHandler::resumeEndpoint(uint32 connectionId)
{
  HashMap<uint32, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return;
  EndpointHandler* endpoint = *it;
  endpoint->resumeByDownlink();
}

void ClientHandler::suspendAllEndpoints()
{
  if(suspendedAlldEnpoints)
    return;
  for(HashMap<uint32, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    (*i)->suspend();
  suspendedAlldEnpoints = true;
}

void ClientHandler::resumeAllEndpoints()
{
  if(!suspendedAlldEnpoints)
    return;
  for(HashMap<uint32, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    (*i)->resume();
  suspendedAlldEnpoints = false;
}
