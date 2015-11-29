
#include <nstd/Log.h>
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

#include <lz4.h>

#include "UplinkHandler.h"
#include "ServerHandler.h"

UplinkHandler::UplinkHandler(ServerHandler& serverHandler, Server& server)
  : serverHandler(serverHandler), server(server), handle(0), addr(addr), port(port), authed(false) {}

UplinkHandler::~UplinkHandler()
{
  if(handle)
  {
    Log::infof("Closed uplink connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
    server.close(*handle);
  }
}

bool_t UplinkHandler::accept(Server::Handle& listenerHandle)
{
  ASSERT(!handle);
  handle = server.accept(listenerHandle, this, &addr, &port);
  if(!handle)
    return false;
  Log::infof("Accepted uplink connection with %s:%hu", (const char_t*)Socket::inetNtoA(addr), port);
}

bool_t UplinkHandler::sendConnect(uint32_t connectionId, uint16_t port)
{
  if(!authed)
    return false;

  Protocol::ConnectMessage connectMessage;
  connectMessage.size = sizeof(connectMessage);
  connectMessage.messageType = Protocol::connect;
  connectMessage.connectionId = connectionId;
  connectMessage.port = port;
  server.write(*handle, (const byte_t*)&connectMessage, sizeof(connectMessage));
  return true;
}

bool_t UplinkHandler::sendDisconnect(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::DisconnectMessage disconnectMessage;
  disconnectMessage.size = sizeof(disconnectMessage);
  disconnectMessage.messageType = Protocol::disconnect;
  disconnectMessage.connectionId = connectionId;
  server.write(*handle, (const byte_t*)&disconnectMessage, sizeof(disconnectMessage));
  return true;
}

bool_t UplinkHandler::sendData(uint32_t connectionId, const byte_t* data, size_t size)
{
  if(!authed)
    return false;

  lz4Buffer.resize(sizeof(Protocol::DataMessage) + LZ4_compressBound(size));
  int compressedSize = LZ4_compress((const char*)data, (char*)(byte_t*)lz4Buffer + sizeof(Protocol::DataMessage), size);
  if(compressedSize <= 0)
  {
    serverHandler.removeUplink();
    return false;
  }

  Protocol::DataMessage* dataMessage = (Protocol::DataMessage*)(byte_t*)lz4Buffer;
  dataMessage->size = sizeof(Protocol::DataMessage) + compressedSize;
  dataMessage->messageType = Protocol::data;
  dataMessage->connectionId = connectionId;
  dataMessage->originalSize = size;
  if(!server.write(*handle, (const byte_t*)dataMessage, dataMessage->size))
    serverHandler.suspendAllEntries();
  return true;
}

bool_t UplinkHandler::sendSuspend(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::SuspendMessage suspendMessage;
  suspendMessage.size = sizeof(suspendMessage);
  suspendMessage.messageType = Protocol::suspend;
  suspendMessage.connectionId = connectionId;
  server.write(*handle, (const byte_t*)&suspendMessage, sizeof(suspendMessage));
  return true;
}

bool_t UplinkHandler::sendResume(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::ResumeMessage resumeMessage;
  resumeMessage.size = sizeof(resumeMessage);
  resumeMessage.messageType = Protocol::resume;
  resumeMessage.connectionId = connectionId;
  server.write(*handle, (const byte_t*)&resumeMessage, sizeof(resumeMessage));
  return true;
}

void_t UplinkHandler::handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size)
{
  switch(messageType)
  {
  case Protocol::auth:
    if(!authed && size >= sizeof(Protocol::AuthMessage))
      handleAuthMessage(*(Protocol::AuthMessage*)data);
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

void_t UplinkHandler::handleAuthMessage(Protocol::AuthMessage& authMessage)
{
  if(Protocol::getString(authMessage.secret) != serverHandler.getSecret())
  {
    serverHandler.removeUplink();
    return; // invalid secret
  }
  authed = true;
  Protocol::Header response;
  response.size = sizeof(response);
  response.messageType = Protocol::authResponse;
  server.write(*handle, (const byte_t*)&response, sizeof(response));
}

void_t UplinkHandler::handleDisconnectMessage(const Protocol::DisconnectMessage& disconnectMessage)
{
  serverHandler.removeEntry(disconnectMessage.connectionId);
}

void_t UplinkHandler::handleDataMessage(const Protocol::DataMessage& message, byte_t* data, size_t size)
{
  //Console::printf("recvData: compressedSize=%d, originalSize=%d\n", (int)size, (int)message.originalSize);

  int originalSize = message.originalSize;
  lz4Buffer.resize(originalSize);
  if(LZ4_decompress_safe((const char*)data,(char*)(byte_t*)lz4Buffer, size, originalSize) != originalSize)
  {
    serverHandler.removeUplink();
    return;
  }

  serverHandler.sendDataToEntry(message.connectionId, lz4Buffer, originalSize);
}

void_t UplinkHandler::handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage)
{
  serverHandler.suspendEntry(suspendMessage.connectionId);
}

void_t UplinkHandler::handleResumeMessage(const Protocol::ResumeMessage& resumeMessage)
{
  serverHandler.resumeEntry(resumeMessage.connectionId);
}

void_t UplinkHandler::readClient()
{
  size_t size;
  size_t oldSize = readBuffer.size();
  readBuffer.resize(LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage) + 1);
  if(!server.read(*handle, (byte_t*)readBuffer + readBuffer.size(), readBuffer.capacity() - readBuffer.size(), size))
    return;
  size += oldSize;
  readBuffer.resize(size);

  byte_t* pos = readBuffer;
  while(size > 0)
  {
    if(size < sizeof(Protocol::Header))
      break;
    Protocol::Header* header = (Protocol::Header*)pos;
    if(header->size < sizeof(Protocol::Header))
    {
      serverHandler.removeUplink();
      return;
    }
    if(size < header->size)
      break;
    handleMessage((Protocol::MessageType)header->messageType, pos, header->size);
    pos += header->size;
    size -= header->size;
  }
  readBuffer.removeFront(pos - (byte_t*)readBuffer);
  if(size > LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage))
  {
    serverHandler.removeUplink();
    return;
  }
}

void_t UplinkHandler::writeClient()
{
  serverHandler.resumeAllEntries();
}

void_t UplinkHandler::closedClient()
{
  serverHandler.removeUplink();
}
