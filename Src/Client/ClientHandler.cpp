
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
  for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    delete *i;
  delete downlink;
}

bool_t ClientHandler::connect(uint32_t addr, uint16_t port, const String& secret)
{
  ASSERT(!downlink);
  this->addr = addr;
  this->port = port;
  downlink = new DownlinkHandler(*this, server);
  if(!downlink->connect(addr, port))
  {
    delete downlink;
    downlink = 0;
    return false;
  }
  this->secret = secret;
  return true;
}

void_t ClientHandler::executeTimer()
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

bool_t ClientHandler::removeDownlink()
{
  if(!downlink)
    return false;
  delete downlink;
  downlink = 0;

  for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    delete *i;
  endpoints.clear(),

  reconnectTimer = server.createTimer(10 * 1000, this); // start reconnect timer
  return true;
}

bool_t ClientHandler::createEndpoint(uint32_t connectionId, uint16_t port)
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

bool_t ClientHandler::removeEndpoint(uint32_t connectionId)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  delete *it;
  endpoints.remove(it);

  if(downlink)
    downlink->sendDisconnect(connectionId);
  return true;
}

bool_t ClientHandler::sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size)
{
  if(!downlink)
    return false;
  downlink->sendData(connectionId, data, size);
  return true;
}

bool_t ClientHandler::sendDataToEndpoint(uint32_t connectionId, byte_t* data, size_t size)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return false;
  EndpointHandler* endpoint = *it;
  endpoint->sendData(data, size);
  return true;
}

void_t ClientHandler::sendSuspendEntry(uint32_t connectionId)
{
  if(!downlink)
    return;
  downlink->sendSuspend(connectionId);
}

void_t ClientHandler::sendResumeEntry(uint32_t connectionId)
{
  if(!downlink)
    return;
  downlink->sendResume(connectionId);
}

void_t ClientHandler::suspendEndpoint(uint32_t connectionId)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return;
  EndpointHandler* endpoint = *it;
  endpoint->suspendByDownlink();
}

void_t ClientHandler::resumeEndpoint(uint32_t connectionId)
{
  HashMap<uint32_t, EndpointHandler*>::Iterator it = endpoints.find(connectionId);
  if(it == endpoints.end())
    return;
  EndpointHandler* endpoint = *it;
  endpoint->resumeByDownlink();
}

void_t ClientHandler::suspendAllEndpoints()
{
  if(suspendedAlldEnpoints)
    return;
  for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    (*i)->suspend();
  suspendedAlldEnpoints = true;
}

void_t ClientHandler::resumeAllEndpoints()
{
  if(!suspendedAlldEnpoints)
    return;
  for(HashMap<uint32_t, EndpointHandler*>::Iterator i = endpoints.begin(), end = endpoints.end(); i != end; ++i)
    (*i)->resume();
  suspendedAlldEnpoints = false;
}
