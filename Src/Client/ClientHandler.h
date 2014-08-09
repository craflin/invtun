
#pragma once

#include <nstd/HashMap.h>

#include "Tools/Server.h"

class DownlinkHandler;
class EndpointHandler;

class ClientHandler : public Server::Listener
{
public:
  ClientHandler(Server& server, uint32_t addr, uint16_t port, const String& secret);
  ~ClientHandler();

  const String& getSecret() const {return secret;}

  bool_t connect();
  
  bool_t createConnection(uint32_t connectionId, uint16_t port);
  bool_t removeConnection(uint32_t connectionId);
  bool_t sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendDataToEndpoint(uint32_t connectionId, byte_t* data, size_t size);

private:
  Server& server;
  uint32_t addr;
  uint16_t port;
  String secret;
  DownlinkHandler* downlink;
  HashMap<uint32_t, EndpointHandler*> endpoints;

private: // Server::Listener
  virtual void_t establishedClient(Server::Client& client);
  virtual void_t closedClient(Server::Client& client);
  virtual void_t abolishedClient(Server::Client& client);
  virtual void_t executedTimer(Server::Timer& timer);
};
