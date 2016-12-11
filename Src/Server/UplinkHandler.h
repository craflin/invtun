
#pragma once

#include <nstd/Socket/Server.h>
#include <nstd/Buffer.h>

#include "Protocol.h"
#include "Callback.h"

class ServerHandler;

class UplinkHandler : public Callback
{
public:
  UplinkHandler(ServerHandler& serverHandler, Server& server);
  ~UplinkHandler();

  bool accept(Server::Handle& listenerHandle);

  bool sendConnect(uint32 connectionId, uint16 port);
  bool sendDisconnect(uint32 connectionId);
  bool sendData(uint32 connectionId, const byte* data, size_t size);
  bool sendSuspend(uint32 connectionId);
  bool sendResume(uint32 connectionId);

private:
  ServerHandler& serverHandler;
  Server& server;
  Server::Handle* handle;
  uint32 addr;
  uint16 port;
  bool authed;
  Buffer lz4Buffer;
  Buffer readBuffer;

private:
  void handleMessage(Protocol::MessageType messageType, byte* data, size_t size);
  void handleAuthMessage(Protocol::AuthMessage& authMessage);
  void handleDisconnectMessage(const Protocol::DisconnectMessage& disconnectMessage);
  void handleDataMessage(const Protocol::DataMessage& dataMessage, byte* data, size_t size);
  void handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Callback
  virtual void readClient();
  virtual void writeClient();
  virtual void closedClient();
};
