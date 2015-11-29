
#pragma once

#include <nstd/Buffer.h>
#include <nstd/Socket/Server.h>

#include "Callback.h"

class ClientHandler;

class EndpointHandler : public Callback
{
public:
  EndpointHandler(ClientHandler& clientHandler, Server& server, uint32_t connectionId);
  ~EndpointHandler();

  uint32_t getConnectionId() const {return connectionId;}

  bool_t connect(uint16_t port);

  void_t sendData(byte_t* data, size_t size);
  void_t suspend();
  void_t resume();
  void_t suspendByDownlink();
  void_t resumeByDownlink();

private:
  ClientHandler& clientHandler;
  Server& server;
  Server::Handle* handle;
  uint32_t connectionId;
  uint16_t port;
  bool_t connected;
  Buffer sendBuffer;
  bool_t suspended;
  bool_t suspendedByDownlink;

private: // Callback
  virtual void_t openedClient();
  virtual void_t abolishedClient();
  virtual void_t closedClient();
  virtual void_t readClient();
  virtual void_t writeClient();
};
