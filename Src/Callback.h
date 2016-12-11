
#pragma once

#include <nstd/Socket/Server.h>

class Callback
{
public:
  virtual void openedClient() {}
  virtual void readClient() {}
  virtual void writeClient() {}
  virtual void closedClient() {}
  virtual void abolishedClient() {}
  virtual void acceptClient(Server::Handle& handle) {}
  virtual void executeTimer() {}
};
