
#pragma once

#include <nstd/HashSet.h>
#include <nstd/HashMap.h>

#include "Tools/Server.h"

class UplinkHandler;
class ClientHandler;

class ServerHandler : public Server::Listener
{
public:
  ServerHandler(uint16_t uplinkPort, const String& secret, const HashMap<uint16_t, uint16_t>& ports);
  ~ServerHandler();

  const String& getSecret() const {return secret;}
  UplinkHandler* getUplink() {return uplinkHandler;}
  ClientHandler* getClient(uint32_t connectionId);

private:
  uint16_t uplinkPort;
  String secret;
  UplinkHandler* uplinkHandler;
  HashSet<ClientHandler*> clients;
  HashMap<uint32_t, ClientHandler*> clientsById;
  HashMap<uint16_t, uint16_t> portMapping;
  uint32_t nextConnectionId;

private:
  uint16_t mapPort(uint16_t port);

private: // Server::Listener
  virtual void_t acceptedClient(Server::Client& client, uint32_t addr, uint16_t port);
  virtual void_t closedClient(Server::Client& client);
};
