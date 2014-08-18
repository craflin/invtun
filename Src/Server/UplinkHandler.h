
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
  bool_t sendSuspend(uint32_t connectionId);
  bool_t sendResume(uint32_t connectionId);
  void_t disconnect() {client.close();}

private:
  ServerHandler& serverHandler;
  Server::Client& client;
  bool_t authed;
  Buffer lz4Buffer;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);
  void_t handleAuthMessage(Protocol::AuthMessage& authMessage);
  void_t handleDisconnectMessage(const Protocol::DisconnectMessage& disconnectMessage);
  void_t handleDataMessage(const Protocol::DataMessage& dataMessage, byte_t* data, size_t size);
  void_t handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void_t handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Server::Client::Listener
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write();
};
