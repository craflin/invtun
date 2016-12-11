
#pragma once

#include <nstd/Socket/Server.h>
#include <nstd/Buffer.h>

#include "Protocol.h"
#include "Callback.h"

class ClientHandler;

class DownlinkHandler : public Callback
{
public:
  DownlinkHandler(ClientHandler& clientHandler, Server& server);
  ~DownlinkHandler();

  bool connect(uint32 addr, uint16 port);

  bool sendDisconnect(uint32 connectionId);
  bool sendData(uint32 connectionId, byte* data, size_t size);
  bool sendSuspend(uint32 connectionId);
  bool sendResume(uint32 connectionId);

private:
  ClientHandler& clientHandler;
  Server& server;
  Server::Handle* handle;
  uint32 addr;
  uint16 port;
  bool connected;
  bool authed;
  Buffer lz4Buffer;
  Buffer readBuffer;

private:
  void handleMessage(Protocol::MessageType messageType, byte* data, size_t size);
  void handleConnectMessage(const Protocol::ConnectMessage& connect);
  void handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect);
  void handleDataMessage(const Protocol::DataMessage& message, byte* data, size_t size);
  void handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Callback
  virtual void openedClient();
  virtual void abolishedClient();
  virtual void readClient();
  virtual void writeClient();
  virtual void closedClient();
};
