
#pragma once

#include <nstd/Socket/Server.h>

class Callback
{
public:
  virtual void_t openedClient() {}
  virtual void_t readClient() {}
  virtual void_t writeClient() {}
  virtual void_t closedClient() {}
  virtual void_t abolishedClient() {}
  virtual void_t acceptClient(Server::Handle& handle) {}
  virtual void_t executeTimer() {}
};
