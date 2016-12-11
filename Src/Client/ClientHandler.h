
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

  bool connect(uint32 addr, uint16 port, const String& secret);

  bool removeDownlink();
  bool createEndpoint(uint32 connectionId, uint16 port);
  bool removeEndpoint(uint32 connectionId);
  bool sendDataToDownlink(uint32 connectionId, byte* data, size_t size);
  bool sendDataToEndpoint(uint32 connectionId, byte* data, size_t size);
  void sendSuspendEntry(uint32 connectionId);
  void sendResumeEntry(uint32 connectionId);
  void suspendEndpoint(uint32 connectionId);
  void resumeEndpoint(uint32 connectionId);
  void suspendAllEndpoints();
  void resumeAllEndpoints();

private:
  Server& server;
  uint32 addr;
  uint16 port;
  String secret;
  DownlinkHandler* downlink;
  HashMap<uint32, EndpointHandler*> endpoints;
  bool suspendedAlldEnpoints;
  Server::Handle* reconnectTimer;

private: // Callback
  virtual void executeTimer();
};
