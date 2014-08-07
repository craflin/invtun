
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

  bool_t connect();
  void_t startReconnectTimer();

private:
  Server& server;
  uint32_t addr;
  uint16_t port;
  String secret;
  DownlinkHandler* downlink;
  HashMap<uint32_t, UplinkHandler*> uplinks;

private:

  //bool_t createConnection(uint32_t connectionId, uint32_t addr, uint16_t port);

private: // Server::Listener
  virtual void_t establishedClient(Server::Client& client, uint32_t addr, uint16_t port);
  virtual void_t closedClient(Server::Client& client);
  virtual void_t abolishedClient(uint32_t addr, uint16_t port);
  virtual void_t executedTimer(Server::Timer& timer);
};
