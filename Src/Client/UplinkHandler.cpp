
#include "UplinkHandler.h"

UplinkHandler::UplinkHandler(ServerHandler& serverHandler, Server::Client& client) : serverHandler(serverHandler), client(client)
{
  client.setListener(this);
}

UplinkHandler::~UplinkHandler()
{
  client.setListener(0);
  client.close();
}
