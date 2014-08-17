
#pragma once

#include <nstd/HashSet.h>
#include <nstd/HashMap.h>

#include "Tools/Server.h"

class EntryHandler;
class UplinkHandler;

class ServerHandler : public Server::Listener
{
public:
  ServerHandler(uint16_t uplinkPort, const String& secret, const HashMap<uint16_t, uint16_t>& ports);
  ~ServerHandler();

  const String& getSecret() const {return secret;}

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
  uint16_t uplinkPort;
  String secret;
  UplinkHandler* uplink;
  HashMap<uint32_t, EntryHandler*> entries;
  HashMap<uint16_t, uint16_t> portMapping;
  uint32_t nextConnectionId;
  bool_t suspendedAllEntries;

private:
  uint16_t mapPort(uint16_t port);

private: // Server::Listener
  virtual void_t acceptedClient(Server::Client& client, uint16_t localPort);
  virtual void_t closedClient(Server::Client& client);
};
