
#pragma once

#include "Tools/Server.h"
#include "Protocol.h"

class ServerHandler;

class UplinkHandler : public Server::Client::Listener
{
public:
  UplinkHandler(ServerHandler& serverHandler, Server::Client& client);
  ~UplinkHandler();

  bool_t sendConnect(uint32_t connectionId, uint16_t port);
  bool_t sendDisconnect(uint32_t connectionId);
  bool_t sendData(uint32_t connectionId, const byte_t* data, size_t size);

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  bool_t authed;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);
  void_t handleAuthMessage(Protocol::AuthMessage& authMessage);
  void_t handleDisconnectMessag(Protocol::DisconnectMessage& disconnectMessage);
  void_t handleDataMessag(Protocol::DataMessage& dataMessage, byte_t* data, size_t size);

private: // Server::Client::Listener
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t close() {}
};
