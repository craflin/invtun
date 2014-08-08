
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

  bool_t removeClient(uint32_t connectionId);
  bool_t sendDataToClient(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendDataToUplink(uint32_t connectionId, byte_t* data, size_t size);

private:
  uint16_t uplinkPort;
  String secret;
  UplinkHandler* uplink;
  HashMap<uint32_t, ClientHandler*> clients;
  HashMap<uint16_t, uint16_t> portMapping;
  uint32_t nextConnectionId;

private:
  uint16_t mapPort(uint16_t port);

private: // Server::Listener
  virtual void_t acceptedClient(Server::Client& client);
  virtual void_t closedClient(Server::Client& client);
};
