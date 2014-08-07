
#pragma once

#include "Tools/Server.h"
#include "Protocol.h"

class ServerHandler;

class DownlinkHandler : public Server::Client::Listener
{
public:
  DownlinkHandler(ServerHandler& serverHandler, Server::Client& client);
  ~DownlinkHandler();

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  bool authed;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);

  void_t handleConnectMessage(Protocol::ConnectMessage& connect);

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write() {};
  virtual void_t close();
  virtual void_t abolish();
};
