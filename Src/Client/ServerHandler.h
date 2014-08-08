
#pragma once

#include <nstd/HashMap.h>

#include "Tools/Server.h"

class DownlinkHandler;
class UplinkHandler;

class ServerHandler : public Server::Listener
{
public:
  ServerHandler(Server& server, uint32_t addr, uint16_t port, const String& secret);
  ~ServerHandler();

  const String& getSecret() const {return secret;}

  bool_t connect();
  
  bool_t createConnection(uint32_t connectionId, uint16_t port);
  bool_t removeConnection(uint32_t connectionId);
  bool_t sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendDataToUplink(uint32_t connectionId, byte_t* data, size_t size);

private:
  Server& server;
  uint32_t addr;
  uint16_t port;
  String secret;
  DownlinkHandler* downlink;
  HashMap<uint32_t, UplinkHandler*> uplinks;

private: // Server::Listener
  virtual void_t establishedClient(Server::Client& client);
  virtual void_t closedClient(Server::Client& client);
  virtual void_t abolishedClient(Server::Client& client);
  virtual void_t executedTimer(Server::Timer& timer);
};
