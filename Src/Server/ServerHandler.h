
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
  ServerHandler(Server& server, const String& secret);
  ~ServerHandler();

  const String& getSecret() const {return secret;}

  bool_t listen(uint32_t addr, uint16_t port);
  bool_t listen(uint32_t addr, uint16_t port, uint16_t mappedPort);

  bool_t removeUplink();
  bool_t removeEntry(uint32_t connectionId);
  bool_t sendDataToEntry(uint32_t connectionId, byte_t* data, size_t size);
  bool_t sendDataToUplink(uint32_t connectionId, byte_t* data, size_t size);
  void_t sendSuspendEndpoint(uint32_t connectionId);
  void_t sendResumeEndpoint(uint32_t connectionId);
  void_t suspendEntry(uint32_t connectionId);
  void_t resumeEntry(uint32_t connectionId);
  void_t suspendAllEntries();
  void_t resumeAllEntries();

private:
  Server& server;
  Server::Handle* tunnelListener;
  HashMap<Server::Handle*, uint16_t> inboundListeners;
  String secret;
  UplinkHandler* uplink;
  HashMap<uint32_t, EntryHandler*> entries;
  uint32_t nextConnectionId;
  bool_t suspendedAllEntries;

private:
  uint16_t mapPort(Server::Handle& handle) {return *inboundListeners.find(&handle);}

private: // Callback
  virtual void_t acceptClient(Server::Handle& handle);
};
