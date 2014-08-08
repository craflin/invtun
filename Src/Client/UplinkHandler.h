
#pragma once

#include "Tools/Server.h"

class ServerHandler;

class UplinkHandler : public Server::Client::Listener
{
public:
  UplinkHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId, uint16_t port);
  ~UplinkHandler();

  uint32_t getConnectionId() const {return connectionId;}
  uint16_t getPort() const {return port;}

  void_t sendData(byte_t* data, size_t size);

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  uint32_t connectionId;
  uint16_t port;
  bool_t connected;
  Buffer sendBuffer;

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t close() {}
};
