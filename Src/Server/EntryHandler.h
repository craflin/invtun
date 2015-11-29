
#pragma once

#include <nstd/Socket/Server.h>

#include "Callback.h"

class ServerHandler;

class EntryHandler : public Callback
{
public:
  EntryHandler(ServerHandler& serverHandler, Server& server, uint32_t connectionId);
  ~EntryHandler();

  uint32_t getConnectionId() const {return connectionId;}

  bool_t accept(Server::Handle& listenerHandle);

  void_t sendData(const byte_t* data, size_t size);
  void_t suspend();
  void_t resume();
  void_t suspendByUplink();
  void_t resumeByUplink();

private:
  ServerHandler& serverHandler;
  Server& server;
  Server::Handle* handle;
  uint32_t connectionId;
  uint32_t addr;
  uint16_t port;
  bool_t suspended;
  bool_t suspendedByUplink;

private: // Server::Client::Listener
  virtual void_t readClient();
  virtual void_t writeClient();
  virtual void_t closedClient();
};
