
#pragma once

#include <nstd/Socket/Server.h>
#include <nstd/HashMap.h>
#include <nstd/String.h>

#include "Callback.h"

#define SEND_BUFFER_SIZE (256 * 1024)
#define RECV_BUFFER_SIZE (256 * 1024)

class DownlinkHandler;
class EndpointHandler;

class ClientHandler : public Callback
{
public:
  ClientHandler(Server& server);
  ~ClientHandler();

  const String& getSecret() const {return secret;}

  bool_t connect(uint32_t addr, uint16_t port, const String& secret);

  bool_t removeDownlink();
  bool_t createEndpoint(uint32_t connectionId, uint16_t port);
  bool_t removeEndpoint(uint32_t connectionId);
  bool_t sendDataToDownlink(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendDataToEndpoint(uint32_t connectionId, byte_t* data, size_t size);
  void_t sendSuspendEntry(uint32_t connectionId);
  void_t sendResumeEntry(uint32_t connectionId);
  void_t suspendEndpoint(uint32_t connectionId);
  void_t resumeEndpoint(uint32_t connectionId);
  void_t suspendAllEndpoints();
  void_t resumeAllEndpoints();

private:
  Server& server;
  uint32_t addr;
  uint16_t port;
  String secret;
  DownlinkHandler* downlink;
  HashMap<uint32_t, EndpointHandler*> endpoints;
  bool_t suspendedAlldEnpoints;
  Server::Handle* reconnectTimer;

private: // Callback
  virtual void_t executeTimer();
};
