
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
    void_t suspend() {socket.suspend();}
    void_t resume() {socket.resume();}
    bool_t flush() {return socket.flush();}
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
  };

  class ClientSocket : public CallbackSocket
  {
  public:
    Client client;
    Buffer sendBuffer;
    Buffer recvBuffer;
    bool_t suspended;

    ClientSocket(Server& server) : CallbackSocket(server), client(server, *this), suspended(false) {}

    void_t reserve(size_t capacity);
    bool_t send(const byte_t* data, size_t size);
    void_t suspend();
    void_t resume();
    bool_t flush();

    virtual void_t read();
    virtual void_t write() {flush();}
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
      int_t err = getAndResetErrorStatus();
      if(err == 0)
        server.establish(*this);
      else
        server.abolish(*this, err);
    }
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

