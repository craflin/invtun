
#pragma once

#include "Tools/Server.h"
#include "Protocol.h"

class ServerHandler;

class DownlinkHandler : public Server::Client::Listener
{
public:
  DownlinkHandler(ServerHandler& serverHandler, Server::Client& client);
  ~DownlinkHandler();

  bool_t sendDisconnect(uint32_t connectionId);
  bool_t sendData(uint32_t connectionId, byte_t* data, size_t size);

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  bool_t authed;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);

  void_t handleConnectMessage(Protocol::ConnectMessage& connect);
  void_t handleDisconnectMessage(Protocol::DisconnectMessage& disconnect);
  void_t handleDataMessage(Protocol::DataMessage& message, byte_t* data, size_t size);

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t close() {}
};
