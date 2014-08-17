
#pragma once

#include "Tools/Server.h"

class ClientHandler;

class EndpointHandler : public Server::Client::Listener
{
public:
  EndpointHandler(ClientHandler& clientHandler, Server::Client& client, uint32_t connectionId);
  ~EndpointHandler();

  uint32_t getConnectionId() const {return connectionId;}

  void_t sendData(byte_t* data, size_t size);
  void_t disconnect() {client.close();}
  void_t suspend();
  void_t resume();
  void_t suspendByDownlink();
  void_t resumeByDownlink();

private:
  ClientHandler& clientHandler;
  Server::Client& client;
  uint32_t connectionId;
  bool_t connected;
  Buffer sendBuffer;
  bool_t suspended;
  bool_t suspendedByDownlink;

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write();
};
