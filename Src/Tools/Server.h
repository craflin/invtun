
#pragma once

#include <nstd/List.h>
#include <nstd/HashSet.h>
#include <nstd/MultiMap.h>
#include <nstd/Buffer.h>

#include "Socket.h"

class Server
{
private:
  class ClientSocket;

public:
  class Client
  {
  public:
    class Listener
    {
    public:
      virtual void_t establish() {}
      virtual size_t handle(byte_t* data, size_t size) = 0;
      virtual void_t write() {}
      virtual void_t close() {}
      virtual void_t abolish() {}
    };

  public:
    void_t setListener(Listener* listener) {this->listener = listener;}
    Listener* getListener() const {return listener;}
    void_t reserve(size_t capacity) {socket.reserve(capacity);}
    bool_t send(const byte_t* data, size_t size) {return socket.send(data, size);}
    void_t close() {server.close(socket);}
    uint32_t getAddr() const {return addr;}
    uint16_t getPort() const {return port;}

  private:
    Client(Server& server, Server::ClientSocket& socket) : listener(0), server(server), socket(socket) {}

    Listener* listener;
    Server& server;
    Server::ClientSocket& socket;
    uint32_t addr;
    uint16_t port;

    friend class Server;
  };

  class Timer
  {
  public:
    class Listener
    {
    public:
      virtual bool_t execute() = 0;
    };

  public:
    void_t setListener(Listener* listener) {this->listener = listener;}
    Listener* getListener() const {return listener;}

  private:
    Timer(timestamp_t interval) : interval(interval), listener(0) {}

    timestamp_t interval;
    Listener* listener;

    friend class Server;
  };

  class Listener
  {
  public:
    virtual void_t acceptedClient(Client& client, uint16_t localPort) {};
    virtual void_t establishedClient(Client& client) {};
    virtual void_t closedClient(Client& client) {};
    virtual void_t abolishedClient(Client& client) {};
    virtual void_t executedTimer(Timer& timer) {};
  };

  Server() : stopped(false), listener(0) {}
  ~Server();

  void_t setListener(Listener* listener) {this->listener = listener;}
  Listener* getListener() const {return listener;}

  bool_t listen(uint16_t port);
  Client* connect(uint32_t addr, uint16_t port);
  Timer& addTimer(timestamp_t interval);
  bool_t process();
  void_t stop();

private:
  class CallbackSocket : public Socket
  {
  public:
    Server& server;
    CallbackSocket(Server& server) : server(server) {}
    virtual void_t read() {};
    virtual void_t write() {};
    virtual void_t except() {};
  };

  class ClientSocket : public CallbackSocket
  {
  public:
    Client client;
    Buffer sendBuffer;
    Buffer recvBuffer;

    ClientSocket(Server& server) : CallbackSocket(server), client(server, *this) {}

    void_t reserve(size_t capacity)
    {
      sendBuffer.reserve(sendBuffer.size() + capacity);
    }

    bool_t send(const byte_t* data, size_t size)
    {
      if(sendBuffer.isEmpty())
        server.selector.set(*this, Socket::Selector::readEvent | Socket::Selector::writeEvent);
      sendBuffer.append(data, size);
      return true;
    }

    virtual void_t read()
    {
      size_t bufferSize = recvBuffer.size();
      recvBuffer.resize(bufferSize + 1500);
      ssize_t received = Socket::recv(recvBuffer + bufferSize, 1500);
      switch(received)
      {
      case -1:
        if(getLastError() == 0) // EWOULDBLOCK
          return;
        // no break
      case 0:
        server.close(*this);
        return;
      }
      bufferSize += received;
      recvBuffer.resize(bufferSize);
      size_t handled = client.listener ? client.listener->handle(recvBuffer, bufferSize) : bufferSize;
      recvBuffer.removeFront(handled);
    }

    virtual void_t write()
    {
      if(sendBuffer.isEmpty())
        return;
      ssize_t sent = Socket::send(sendBuffer, sendBuffer.size());
      switch(sent)
      {
      case -1:
        if(getLastError() == 0) // EWOULDBLOCK
          return;
        // no break
      case 0:
        server.close(*this);
        return;
      }
      sendBuffer.removeFront(sent);
      if(sendBuffer.isEmpty())
      {
        sendBuffer.free();
        server.selector.set(*this, Socket::Selector::readEvent);
        if(client.listener)
          client.listener->write();
      }
    }
  };

  class ServerSocket : public CallbackSocket
  {
  public:
    uint16_t port;
    ServerSocket(Server& server, uint16_t port) : CallbackSocket(server), port(port) {}
    virtual void_t read() {server.accept(*this);}
  };

  class ConnectSocket : public CallbackSocket
  {
  public:
    ClientSocket* clientSocket;
    ConnectSocket(Server& server) : CallbackSocket(server), clientSocket(new ClientSocket(server)) {}
    ~ConnectSocket() {delete clientSocket;}
    virtual void_t write()
    {
      int_t err = getAndResetErrorStatus(); // we need this on linux. on windows write does not get called on connect failure.
      if(err == 0)
        server.establish(*this);
      else
        server.abolish(*this, err);
    }
    virtual void_t except() {server.abolish(*this, getAndResetErrorStatus());} // detect connect failure on windows
  };

  void_t close(ClientSocket& socket);
  void_t accept(ServerSocket& socket);
  void_t establish(ConnectSocket& socket);
  void_t abolish(ConnectSocket& socket, int_t error);

  volatile bool stopped;
  Listener* listener;
  Socket::Selector selector;
  List<ServerSocket*> serverSockets;
  HashSet<ConnectSocket*> connectSockets;
  HashSet<ClientSocket*> clientSockets;
  List<ClientSocket*> socketsToDelete;
  MultiMap<timestamp_t, Timer*> timers;
};

