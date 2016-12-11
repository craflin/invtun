
#pragma once

#include <nstd/Socket/Server.h>

#include "Callback.h"

class ServerHandler;

class EntryHandler : public Callback
{
public:
  EntryHandler(ServerHandler& serverHandler, Server& server, uint32 connectionId);
  ~EntryHandler();

  uint32 getConnectionId() const {return connectionId;}

  bool accept(Server::Handle& listenerHandle);

  void sendData(const byte* data, size_t size);
  void suspend();
  void resume();
  void suspendByUplink();
  void resumeByUplink();

private:
  ServerHandler& serverHandler;
  Server& server;
  Server::Handle* handle;
  uint32 connectionId;
  uint32 addr;
  uint16 port;
  bool suspended;
  bool suspendedByUplink;

private: // Server::Client::Listener
  virtual void readClient();
  virtual void writeClient();
  virtual void closedClient();
};
