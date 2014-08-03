
#pragma once

#include "Tools/Server.h"

class ServerHandler;

class ClientHandler : public Server::Client::Listener
{
public:
  ClientHandler(uint32_t connectionId, ServerHandler& serverHandler, Server::Client& client);

  uint32_t getConnectionId() const {return connectionId;}

  void_t close() {client.close();}
  void_t sendData(const byte_t* data, size_t size) {client.send(data, size);}

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  uint32_t connectionId;

private: // Server::Client::Listener
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write() {};
};
