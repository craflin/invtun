
#pragma once

#include <nstd/Socket/Server.h>
#include <nstd/Buffer.h>

#include "Protocol.h"
#include "Callback.h"

class ClientHandler;

class DownlinkHandler : public Callback
{
public:
  DownlinkHandler(ClientHandler& clientHandler, Server& server, Server::Handle& handle, uint32_t addr, uint16_t port);
  ~DownlinkHandler();

  bool_t isConnected() const {return connected;}
  uint32_t getAddr() const {return addr;}
  uint16_t getPort() const {return port;}

  bool_t sendDisconnect(uint32_t connectionId);
  bool_t sendData(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendSuspend(uint32_t connectionId);
  bool_t sendResume(uint32_t connectionId);

private:
  ClientHandler& clientHandler;
  Server& server;
  Server::Handle& handle;
  uint32_t addr;
  uint16_t port;
  bool_t connected;
  bool_t authed;
  Buffer lz4Buffer;
  Buffer readBuffer;

private:
  void_t handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size);
  void_t handleConnectMessage(const Protocol::ConnectMessage& connect);
  void_t handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect);
  void_t handleDataMessage(const Protocol::DataMessage& message, byte_t* data, size_t size);
  void_t handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage);
  void_t handleResumeMessage(const Protocol::ResumeMessage& resumeMessage);

private: // Callback
  virtual void_t openedClient();
  virtual void_t abolishedClient();
  virtual void_t readClient();
  virtual void_t writeClient();
  virtual void_t closedClient();
};
