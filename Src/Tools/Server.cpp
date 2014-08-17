
#include <nstd/Time.h>

#include "Server.h"

Server::~Server()
{
  for(List<ServerSocket*>::Iterator i = serverSockets.begin(), end = serverSockets.end(); i != end; ++i)
    delete *i;
  for(HashSet<ConnectSocket*>::Iterator i = connectSockets.begin(), end = connectSockets.end(); i != end; ++i)
    delete *i;
  for(HashSet<ClientSocket*>::Iterator i = clientSockets.begin(), end = clientSockets.end(); i != end; ++i)
    delete *i;
}

bool_t Server::listen(uint16_t port)
{
  ServerSocket* socket = new ServerSocket(*this, port);
  if(!socket->open() ||
      !socket->setReuseAddress() ||
      !socket->bind(Socket::anyAddr, port) ||
      !socket->listen())
  {
    delete socket;
    return false;
  }
  selector.set(*socket, Socket::Selector::acceptEvent);
  serverSockets.append(socket);
  return true;
}

Server::Client* Server::connect(uint32_t addr, uint16_t port)
{
  ConnectSocket* socket = new ConnectSocket(*this);
  if(!socket->open() ||
      !socket->setNonBlocking() ||
      !socket->connect(addr, port))
  {
    delete socket;
    return 0;
  }
  selector.set(*socket, Socket::Selector::connectEvent);
  connectSockets.append(socket);
  Client& client = socket->clientSocket->client;
  client.addr = addr;
  client.port = port;
  return &client;
}

Server::Timer& Server::addTimer(timestamp_t interval)
{
  timestamp_t timerTime = Time::time() + interval;
  Timer* timer = new Timer(interval);
  timers.insert(timerTime, timer);
  return *timer;
}

void_t Server::stop()
{
  stopped = true;
  for(List<ServerSocket*>::Iterator i = serverSockets.begin(), end = serverSockets.end(); i != end; ++i)
    (*i)->close();
}

bool_t Server::process()
{
  Socket* socket;
  uint_t selectEvent;
  while(!stopped)
  {
    timestamp_t timeout = timers.isEmpty() ? 10000 : (timers.begin().key() - Time::time());
    if(timeout > 0)
    {
      if(!selector.select(socket, selectEvent, timeout))
        return false;
      if(socket)
      {
        if(selectEvent & (Socket::Selector::writeEvent | Socket::Selector::connectEvent))
          ((CallbackSocket*)socket)->write();
        if(selectEvent & (Socket::Selector::readEvent | Socket::Selector::acceptEvent))
          ((CallbackSocket*)socket)->read();
      }
    }
    else
    {
      while(!timers.isEmpty())
      {
        timestamp_t timerTime = timers.begin().key();
        if(timerTime <= Time::time())
        {
          Timer* timer = timers.front();
          timers.removeFront();
          bool repeat = timer->listener && !timer->listener->execute();
          if(listener)
            listener->executedTimer(*timer);
          if(repeat)
            timers.insert(timerTime + timer->interval, timer);
          else
            delete timer;
        }
        else
          break;
      }
    }
    if(!socketsToDelete.isEmpty())
    {
      ClientSocket* clientSocket = socketsToDelete.front();
      socketsToDelete.removeFront();
      clientSockets.remove(clientSocket);
      if(clientSocket->client.listener)
        clientSocket->client.listener->close();
      if(listener)
        listener->closedClient(clientSocket->client);
      delete clientSocket;
    }
  }
  return true;
}

void_t Server::close(ClientSocket& socket)
{
  if(!socket.isOpen())
    return;
  selector.remove(socket);
  socket.close();
  socketsToDelete.append(&socket);
}

void_t Server::accept(ServerSocket& socket)
{
  ClientSocket* clientSocket = new ClientSocket(*this);
  Client& client = clientSocket->client;
  if(!clientSocket->accept(socket, client.addr, client.port) ||
      !clientSocket->setNonBlocking() ||
      !clientSocket->setKeepAlive() ||
      !clientSocket->setNoDelay())
  {
    delete clientSocket;
    return;
  }
  selector.set(*clientSocket, Socket::Selector::readEvent);
  clientSockets.append(clientSocket);
  if(listener)
    listener->acceptedClient(client, socket.port);
  if(client.listener)
    client.listener->establish();
}

void_t Server::establish(ConnectSocket& socket)
{
  ClientSocket* clientSocket = socket.clientSocket;
  socket.clientSocket = 0;
  selector.remove(socket);
  clientSocket->swap(socket);
  connectSockets.remove(&socket);
  delete &socket;
  if(!clientSocket->setKeepAlive() ||
      !clientSocket->setNoDelay())
  {
    delete clientSocket;
    return;
  }
  selector.set(*clientSocket, Socket::Selector::readEvent);
  clientSockets.append(clientSocket);
  Client& client = clientSocket->client;
  if(listener)
    listener->establishedClient(client);
  if(client.listener)
    client.listener->establish();
}

void_t Server::abolish(ConnectSocket& socket, int_t error)
{
  selector.remove(socket);
  connectSockets.remove(&socket);
  Client& client = socket.clientSocket->client;
  Socket::setLastError(error);
  if(client.listener)
    client.listener->abolish();
  Socket::setLastError(error);
  if(listener)
    listener->abolishedClient(client);
  delete &socket;
}

void_t Server::ClientSocket::reserve(size_t capacity)
{
  sendBuffer.reserve(sendBuffer.size() + capacity);
}

bool_t Server::ClientSocket::send(const byte_t* data, size_t size)
{
  if(sendBuffer.isEmpty())
    server.selector.set(*this, (suspended ? 0 : Socket::Selector::readEvent) | Socket::Selector::writeEvent);
  sendBuffer.append(data, size);
  return true;
}

void_t Server::ClientSocket::suspend()
{
  if(suspended)
    return;
  server.selector.set(*this, sendBuffer.isEmpty() ? 0 : Socket::Selector::writeEvent);
  suspended = true;
}

void_t Server::ClientSocket::resume()
{
  if(!suspended)
    return;
  server.selector.set(*this, sendBuffer.isEmpty() ? Socket::Selector::readEvent : (Socket::Selector::readEvent | Socket::Selector::writeEvent));
  suspended = false;
}

bool_t Server::ClientSocket::flush()
{
  if(sendBuffer.isEmpty())
    return true;
  ssize_t sent = Socket::send(sendBuffer, sendBuffer.size());
  switch(sent)
  {
  case -1:
    if(getLastError() == 0) // EWOULDBLOCK
      return false;
    // no break
  case 0:
    server.close(*this);
    return true;
  }
  sendBuffer.removeFront(sent);
  if(sendBuffer.isEmpty())
  {
    sendBuffer.free();
    server.selector.set(*this, suspended ? 0 : Socket::Selector::readEvent);
    if(client.listener)
      client.listener->write();
    return true;
  }
  return false;
}

void_t Server::ClientSocket::read()
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
