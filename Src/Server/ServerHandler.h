
#pragma once

#include <nstd/HashMap.h>
#include <nstd/String.h>
#include <nstd/Socket/Server.h>

#include "Callback.h"

#define SEND_BUFFER_SIZE (256 * 1024)
#define RECV_BUFFER_SIZE (256 * 1024)

class EntryHandler;
class UplinkHandler;

class ServerHandler : public Callback
{
public:
  ServerHandler(Server& server);
  ~ServerHandler();

  const String& getSecret() const {return secret;}

  bool listen(uint32 addr, uint16 port, const String& secret);
  bool listen(uint32 addr, uint16 port, uint16 mappedPort);

  bool removeUplink();
  bool removeEntry(uint32 connectionId);
  bool sendDataToEntry(uint32 connectionId, byte* data, size_t size);
  bool sendDataToUplink(uint32 connectionId, byte* data, size_t size);
  void sendSuspendEndpoint(uint32 connectionId);
  void sendResumeEndpoint(uint32 connectionId);
  void suspendEntry(uint32 connectionId);
  void resumeEntry(uint32 connectionId);
  void suspendAllEntries();
  void resumeAllEntries();

private:
  Server& server;
  Server::Handle* tunnelListener;
  HashMap<Server::Handle*, uint16> inboundListeners;
  String secret;
  UplinkHandler* uplink;
  HashMap<uint32, EntryHandler*> entries;
  uint32 nextConnectionId;
  bool suspendedAllEntries;

private:
  uint16 mapPort(Server::Handle& handle) {return *inboundListeners.find(&handle);}

private: // Callback
  virtual void acceptClient(Server::Handle& handle);
};
