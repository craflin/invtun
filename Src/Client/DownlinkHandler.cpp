
#include <nstd/Log.h>
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

#include <lz4.h>

#include "DownlinkHandler.h"
#include "ClientHandler.h"

DownlinkHandler::DownlinkHandler(ClientHandler& clientHandler, Server& server) :
  clientHandler(clientHandler), server(server), handle(0), connected(false), authed(false) {}

DownlinkHandler::~DownlinkHandler()
{
  if(handle)
  {
    if(connected)
      Log::infof("Closed downlink connection with %s:%hu", (const char*)Socket::inetNtoA(addr), port);
    server.close(*handle);
  }
}

bool DownlinkHandler::connect(uint32 addr, uint16 port)
{
  ASSERT(!handle);
  Log::infof("Establishing downlink connection with %s:%hu...", (const char*)Socket::inetNtoA(addr), port);
  handle = server.connect(addr, port, this);
  if(!handle)
    return false;
  this->addr = addr;
  this->port = port;
  return true;
}

bool DownlinkHandler::sendDisconnect(uint32 connectionId)
{
  if(!authed)
    return false;

  Protocol::DisconnectMessage disconnectMessage;
  disconnectMessage.size = sizeof(disconnectMessage);
  disconnectMessage.messageType = Protocol::disconnect;
  disconnectMessage.connectionId = connectionId;
  server.write(*handle, (const byte*)&disconnectMessage, sizeof(disconnectMessage));
  return true;
}

bool DownlinkHandler::sendData(uint32 connectionId, byte* data, size_t size)
{
  if(!authed)
    return false;

  lz4Buffer.resize(sizeof(Protocol::DataMessage) + LZ4_compressBound(size));
  int compressedSize = LZ4_compress((const char*)data, (char*)(byte*)lz4Buffer + sizeof(Protocol::DataMessage), size);
  if(compressedSize <= 0)
  {
    clientHandler.removeDownlink();
    return false;
  }

  Protocol::DataMessage* dataMessage = (Protocol::DataMessage*)(byte*)lz4Buffer;
  dataMessage->size = sizeof(Protocol::DataMessage) + compressedSize;
  dataMessage->messageType = Protocol::data;
  dataMessage->connectionId = connectionId;
  dataMessage->originalSize = size;
  size_t postponed;
  if(server.write(*handle, (const byte*)dataMessage, dataMessage->size, &postponed) && postponed)
    clientHandler.suspendAllEndpoints();
  return true;
}

bool DownlinkHandler::sendSuspend(uint32 connectionId)
{
  if(!authed)
    return false;

  Protocol::SuspendMessage suspendMessage;
  suspendMessage.size = sizeof(suspendMessage);
  suspendMessage.messageType = Protocol::suspend;
  suspendMessage.connectionId = connectionId;
  server.write(*handle, (const byte*)&suspendMessage, sizeof(suspendMessage));
  return true;
}

bool DownlinkHandler::sendResume(uint32 connectionId)
{
  if(!authed)
    return false;

  Protocol::ResumeMessage resumeMessage;
  resumeMessage.size = sizeof(resumeMessage);
  resumeMessage.messageType = Protocol::resume;
  resumeMessage.connectionId = connectionId;
  server.write(*handle, (const byte*)&resumeMessage, sizeof(resumeMessage));
  return true;
}

void DownlinkHandler::openedClient()
{
  Log::infof("Established downlink connection with %s:%hu", (const char*)Socket::inetNtoA(addr), port);

  connected = true;

  Protocol::AuthMessage authMessage;
  authMessage.size = sizeof(authMessage);
  authMessage.messageType = Protocol::auth;
  Protocol::setString(authMessage.secret, clientHandler.getSecret());
  server.write(*handle, (const byte*)&authMessage, sizeof(authMessage));
}

void DownlinkHandler::abolishedClient()
{
    Log::infof("Could not establish downlink connection with %s:%hu: %s", (const char*)Socket::inetNtoA(addr), port, (const char*)Socket::getErrorString());

    clientHandler.removeDownlink();
}

void DownlinkHandler::handleMessage(Protocol::MessageType messageType, byte* data, size_t size)
{
  switch(messageType)
  {
  case Protocol::authResponse:
    if(!authed)
      authed = true;
    break;
  case Protocol::connect:
    if(authed && size >= sizeof(Protocol::ConnectMessage))
      handleConnectMessage(*(Protocol::ConnectMessage*)data);
    break;
  case Protocol::disconnect:
    if(authed && size >= sizeof(Protocol::DisconnectMessage))
      handleDisconnectMessage(*(Protocol::DisconnectMessage*)data);
    break;
  case Protocol::data:
    if(authed && size >= sizeof(Protocol::DataMessage))
      handleDataMessage(*(Protocol::DataMessage*)data, data + sizeof(Protocol::DataMessage), size - sizeof(Protocol::DataMessage));
    break;
  case Protocol::suspend:
    if(authed && size >= sizeof(Protocol::SuspendMessage))
      handleSuspendMessage(*(Protocol::SuspendMessage*)data);
    break;
  case Protocol::resume:
    if(authed && size >= sizeof(Protocol::ResumeMessage))
      handleResumeMessage(*(Protocol::ResumeMessage*)data);
    break;
  default:
    break;
  }
}

void DownlinkHandler::handleConnectMessage(const Protocol::ConnectMessage& connect)
{
  if(!clientHandler.createEndpoint(connect.connectionId, connect.port))
    sendDisconnect(connect.connectionId);
}

void DownlinkHandler::handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect)
{
  clientHandler.removeEndpoint(disconnect.connectionId);
}

void DownlinkHandler::handleDataMessage(const Protocol::DataMessage& message, byte* data, size_t size)
{
  int originalSize = message.originalSize;
  lz4Buffer.resize(originalSize);
  if(LZ4_decompress_safe((const char*)data,(char*)(byte*)lz4Buffer, size, originalSize) != originalSize)
  {
    clientHandler.removeDownlink();
    return;
  }

  clientHandler.sendDataToEndpoint(message.connectionId, lz4Buffer, originalSize);
}

void DownlinkHandler::handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage)
{
  clientHandler.suspendEndpoint(suspendMessage.connectionId);
}

void DownlinkHandler::handleResumeMessage(const Protocol::ResumeMessage& resumeMessage)
{
  clientHandler.resumeEndpoint(resumeMessage.connectionId);
}

void DownlinkHandler::readClient()
{
  size_t size;
  size_t oldSize = readBuffer.size();
  readBuffer.resize(LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage) + 1);
  if(!server.read(*handle, (byte*)readBuffer + oldSize, readBuffer.capacity() - oldSize, size))
    return;
  size += oldSize;
  readBuffer.resize(size);

  byte* pos = readBuffer;
  while(size > 0)
  {
    if(size < sizeof(Protocol::Header))
      break;
    Protocol::Header* header = (Protocol::Header*)pos;
    if(header->size < sizeof(Protocol::Header))
    {
      clientHandler.removeDownlink();
      return;
    }
    if(size < header->size)
      break;
    handleMessage((Protocol::MessageType)header->messageType, pos, header->size);
    pos += header->size;
    size -= header->size;
  }
  readBuffer.removeFront(pos - (byte*)readBuffer);
  if(size > LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage))
  {
    clientHandler.removeDownlink();
    return;
  }
}

void DownlinkHandler::writeClient()
{
  clientHandler.resumeAllEndpoints();
}

void DownlinkHandler::closedClient()
{
  clientHandler.removeDownlink();
}
