
#pragma once

#include "Tools/Server.h"
#include "Protocol.h"

class ClientHandler;

class DownlinkHandler : public Server::Client::Listener
{
public:
  DownlinkHandler(ClientHandler& clientHandler, Server::Client& client);
  ~DownlinkHandler();

  bool_t sendDisconnect(uint32_t connectionId);
  bool_t sendData(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendSuspend(uint32_t connectionId);
  bool_t sendResume(uint32_t connectionId);

private:
  ClientHandler& clientHandler;
  Server::Client& client;
  bool_t authed;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);
  void_t handleConnectMessage(const Protocol::ConnectMessage& connect);
  void_t handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect);
  void_t handleDataMessage(const Protocol::DataMessage& message, byte_t* data, size_t size);
  void_t handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void_t handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Server::Client::Listener
  virtual void_t establish();
  virtual size_t handle(byte_t* data, size_t size);
  virtual void_t write();
};
