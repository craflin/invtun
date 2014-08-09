
#pragma once

#include "Tools/Server.h"

class ServerHandler;

class EndpointHandler : public Server::Client::Listener
{
public:
  EndpointHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId);
  ~EndpointHandler();

  uint32_t getConnectionId() const {return connectionId;}

  void_t sendData(byte_t* data, size_t size);

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  uint32_t connectionId;
  bool_t connected;
  Buffer sendBuffer;

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t close() {}
};
