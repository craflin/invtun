
#pragma once

#include "Tools/Server.h"

class ServerHandler;

class EntryHandler : public Server::Client::Listener
{
public:
  EntryHandler(ServerHandler& serverHandler, Server::Client& client, uint32_t connectionId);
  ~EntryHandler();

  uint32_t getConnectionId() const {return connectionId;}

  void_t sendData(const byte_t* data, size_t size);
  void_t disconnect() {client.close();}
  void_t suspend();
  void_t resume();
  void_t suspendByUplink();
  void_t resumeByUplink();

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  uint32_t connectionId;
  bool_t suspended;
  bool_t suspendedByUplink;

private: // Server::Client::Listener
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write();
};
