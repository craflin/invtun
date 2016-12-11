
#pragma once

#include <nstd/Buffer.h>
#include <nstd/Socket/Server.h>

#include "Callback.h"

class ClientHandler;

class EndpointHandler : public Callback
{
public:
  EndpointHandler(ClientHandler& clientHandler, Server& server, uint32 connectionId);
  ~EndpointHandler();

  uint32 getConnectionId() const {return connectionId;}

  bool connect(uint16 port);

  void sendData(byte* data, size_t size);
  void suspend();
  void resume();
  void suspendByDownlink();
  void resumeByDownlink();

private:
  ClientHandler& clientHandler;
  Server& server;
  Server::Handle* handle;
  uint32 connectionId;
  uint16 port;
  bool connected;
  Buffer sendBuffer;
  bool suspended;
  bool suspendedByDownlink;

private: // Callback
  virtual void openedClient();
  virtual void abolishedClient();
  virtual void closedClient();
  virtual void readClient();
  virtual void writeClient();
};
