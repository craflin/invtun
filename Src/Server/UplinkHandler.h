
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

  bool_t accept(Server::Handle& listenerHandle);

  bool_t sendConnect(uint32_t connectionId, uint16_t port);
  bool_t sendDisconnect(uint32_t connectionId);
  bool_t sendData(uint32_t connectionId, const byte_t* data, size_t size);
  bool_t sendSuspend(uint32_t connectionId);
  bool_t sendResume(uint32_t connectionId);

private:
  ServerHandler& serverHandler;
  Server& server;
  Server::Handle* handle;
  uint32_t addr;
  uint16_t port;
  bool_t authed;
  Buffer lz4Buffer;
  Buffer readBuffer;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);
  void_t handleAuthMessage(Protocol::AuthMessage& authMessage);
  void_t handleDisconnectMessage(const Protocol::DisconnectMessage& disconnectMessage);
  void_t handleDataMessage(const Protocol::DataMessage& dataMessage, byte_t* data, size_t size);
  void_t handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void_t handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Callback
  virtual void_t readClient();
  virtual void_t writeClient();
  virtual void_t closedClient();
};
