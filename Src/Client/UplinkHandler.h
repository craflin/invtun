
#pragma once

#include "Tools/Server.h"

class ServerHandler;

class UplinkHandler : public Server::Client::Listener
{
public:
  UplinkHandler(ServerHandler& serverHandler, Server::Client& client);
  ~UplinkHandler();

private:
  ServerHandler& serverHandler;
  Server::Client& client;
};
