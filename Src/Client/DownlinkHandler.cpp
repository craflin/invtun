
#include <nstd/Console.h>
#include <nstd/Socket/Socket.h>

#include <lz4.h>

#include "DownlinkHandler.h"
#include "ClientHandler.h"

DownlinkHandler::DownlinkHandler(ClientHandler& clientHandler, Server& server, Server::Handle& handle, uint32_t addr, uint16_t port) :
  clientHandler(clientHandler), server(server), handle(handle), addr(addr), port(port), connected(false), authed(false)
{
  server.setUserData(handle, this);
}

DownlinkHandler::~DownlinkHandler()
{
  server.close(handle);
}

bool_t DownlinkHandler::sendDisconnect(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::DisconnectMessage disconnectMessage;
  disconnectMessage.size = sizeof(disconnectMessage);
  disconnectMessage.messageType = Protocol::disconnect;
  disconnectMessage.connectionId = connectionId;
  server.write(handle, (const byte_t*)&disconnectMessage, sizeof(disconnectMessage));
  return true;
}

bool_t DownlinkHandler::sendData(uint32_t connectionId, byte_t* data, size_t size)
{
  if(!authed)
    return false;

  lz4Buffer.resize(sizeof(Protocol::DataMessage) + LZ4_compressBound(size));
  int compressedSize = LZ4_compress((const char*)data, (char*)(byte_t*)lz4Buffer + sizeof(Protocol::DataMessage), size);
  if(compressedSize <= 0)
  {
    clientHandler.removeDownlink();
    return false;
  }

  Protocol::DataMessage* dataMessage = (Protocol::DataMessage*)(byte_t*)lz4Buffer;
  dataMessage->size = sizeof(Protocol::DataMessage) + compressedSize;
  dataMessage->messageType = Protocol::data;
  dataMessage->connectionId = connectionId;
  dataMessage->originalSize = size;
  if(!server.write(handle, (const byte_t*)dataMessage, dataMessage->size))
    clientHandler.suspendAllEndpoints();
  return true;
}

bool_t DownlinkHandler::sendSuspend(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::SuspendMessage suspendMessage;
  suspendMessage.size = sizeof(suspendMessage);
  suspendMessage.messageType = Protocol::suspend;
  suspendMessage.connectionId = connectionId;
  server.write(handle, (const byte_t*)&suspendMessage, sizeof(suspendMessage));
  return true;
}

bool_t DownlinkHandler::sendResume(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::ResumeMessage resumeMessage;
  resumeMessage.size = sizeof(resumeMessage);
  resumeMessage.messageType = Protocol::resume;
  resumeMessage.connectionId = connectionId;
  server.write(handle, (const byte_t*)&resumeMessage, sizeof(resumeMessage));
  return true;
}

void_t DownlinkHandler::openedClient()
{
  Console::printf("Established downlink connection with %s:%hu\n", (const char_t*)Socket::inetNtoA(addr), port);

  connected = true;

  Protocol::AuthMessage authMessage;
  authMessage.size = sizeof(authMessage);
  authMessage.messageType = Protocol::auth;
  Protocol::setString(authMessage.secret, clientHandler.getSecret());
  server.write(handle, (const byte_t*)&authMessage, sizeof(authMessage));
}

void_t DownlinkHandler::abolishedClient()
{
    Console::printf("Could not establish downlink connection with %s:%hu: %s\n", (const char_t*)Socket::inetNtoA(addr), port, (const char_t*)Socket::getErrorString());

    clientHandler.removeDownlink();
}

void_t DownlinkHandler::handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size)
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

void_t DownlinkHandler::handleConnectMessage(const Protocol::ConnectMessage& connect)
{
  if(!clientHandler.createEndpoint(connect.connectionId, connect.port))
    sendDisconnect(connect.connectionId);
}

void_t DownlinkHandler::handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect)
{
  clientHandler.removeEndpoint(disconnect.connectionId);
}

void_t DownlinkHandler::handleDataMessage(const Protocol::DataMessage& message, byte_t* data, size_t size)
{
  int originalSize = message.originalSize;
  lz4Buffer.resize(originalSize);
  if(LZ4_decompress_safe((const char*)data,(char*)(byte_t*)lz4Buffer, size, originalSize) != originalSize)
  {
    clientHandler.removeDownlink();
    return;
  }

  clientHandler.sendDataToEndpoint(message.connectionId, lz4Buffer, originalSize);
}

void_t DownlinkHandler::handleSuspendMessage(const Protocol::SuspendMessage& suspendMessage)
{
  clientHandler.suspendEndpoint(suspendMessage.connectionId);
}

void_t DownlinkHandler::handleResumeMessage(const Protocol::ResumeMessage& resumeMessage)
{
  clientHandler.resumeEndpoint(resumeMessage.connectionId);
}

void_t DownlinkHandler::readClient()
{
  size_t size;
  size_t oldSize = readBuffer.size();
  readBuffer.resize(LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage) + 1);
  if(!server.read(handle, (byte_t*)readBuffer + readBuffer.size(), readBuffer.capacity() - readBuffer.size(), size))
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
      clientHandler.removeDownlink();
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
    clientHandler.removeDownlink();
    return;
  }
}

void_t DownlinkHandler::writeClient()
{
  clientHandler.resumeAllEndpoints();
}

void_t DownlinkHandler::closedClient()
{
  clientHandler.removeDownlink();
}
