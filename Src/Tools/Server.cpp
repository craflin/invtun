
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
  selector.set(*socket, Socket::Selector::readEvent);
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
  selector.set(*socket, /*Socket::Selector::readEvent | */Socket::Selector::writeEvent);
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
        if(selectEvent & Socket::Selector::writeEvent)
          ((CallbackSocket*)socket)->write();
        if(selectEvent & Socket::Selector::readEvent)
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
