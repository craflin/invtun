
#include "ServerHandler.h"
#include "UplinkHandler.h"
#include "ClientHandler.h"

ServerHandler::ServerHandler(uint16_t uplinkPort, const String& secret, const HashMap<uint16_t, uint16_t>& ports) : uplinkPort(uplinkPort), secret(secret), uplinkHandler(0), nextConnectionId(1)
{
  for(HashMap<uint16_t, uint16_t>::Iterator i = ports.begin(), end = ports.end(); i != end; ++i)
    if(i.key() != *i)
      portMapping.append(i.key(), *i);
}

ServerHandler::~ServerHandler()
{
  delete uplinkHandler;
  for(HashSet<ClientHandler*>::Iterator i = clients.begin(), end = clients.end(); i != end; ++i)
    delete *i;
}

ClientHandler* ServerHandler::getClient(uint32_t connectionId)
{
  HashMap<uint32_t, ClientHandler*>::Iterator it = clientsById.find(connectionId);
  if(it == clientsById.end())
    return 0;
  return *it;
}

uint16_t ServerHandler::mapPort(uint16_t port)
{
  HashMap<uint16_t, uint16_t>::Iterator it = portMapping.find(port);
  if(it == portMapping.end())
    return port;
  return *it;
}

void_t ServerHandler::acceptedClient(Server::Client& client, uint32_t addr, uint16_t port)
{
  uint32_t localIp;
  uint16_t localPort;
  if(!client.getSockName(localIp, localPort))
  {
    client.close();
    return;
  }
  if(localPort == uplinkPort)
  {
    if(uplinkHandler) // there is already an uplink
    {
      client.close();
      return;
    }
    uplinkHandler = new UplinkHandler(*this, client);
    client.setListener(uplinkHandler);
  }
  else
  {
    if(!uplinkHandler)
    {
      client.close();
      return;
    }
    uint32_t connectionId = nextConnectionId++;
    ClientHandler* clientHandler = new ClientHandler(connectionId, *this, client);
    client.setListener(clientHandler);
    clients.append(clientHandler);
    clientsById.append(connectionId, clientHandler);
    uint16_t mappedPort = mapPort(localPort);
    if(!uplinkHandler->createConnection(connectionId, mappedPort))
    {
      client.close();
      return;
    }
  }
}

void_t ServerHandler::closedClient(Server::Client& client)
{
  Server::Client::Listener* listener = client.getListener();
  if(!listener)
    return;
  if(listener == uplinkHandler)
  {
    delete uplinkHandler;
    uplinkHandler = 0;
    for(HashSet<ClientHandler*>::Iterator i = clients.begin(), end = clients.end(); i != end; ++i)
    {
      ClientHandler* clientHandler = *i;
      clientHandler->close();
    }
  }
  else
  {
    ClientHandler* clientHandler = (ClientHandler*)listener;
    clientsById.remove(clientHandler->getConnectionId());
    clients.remove(clientHandler);
    delete clientHandler;
  }
}
