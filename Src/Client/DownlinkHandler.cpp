
#include <lz4.h>

#include "DownlinkHandler.h"
#include "ClientHandler.h"

DownlinkHandler::DownlinkHandler(ClientHandler& clientHandler, Server::Client& client) :
  clientHandler(clientHandler), client(client), authed(false)
{
  client.setListener(this);
}

DownlinkHandler::~DownlinkHandler()
{
  client.setListener(0);
  client.close();
}

bool_t DownlinkHandler::sendDisconnect(uint32_t connectionId)
{
  if(!authed)
    return false;

  Protocol::DisconnectMessage disconnectMessage;
  disconnectMessage.size = sizeof(disconnectMessage);
  disconnectMessage.messageType = Protocol::disconnect;
  disconnectMessage.connectionId = connectionId;
  client.send((const byte_t*)&disconnectMessage, sizeof(disconnectMessage));
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
    client.close();
    return false;
  }

  Protocol::DataMessage* dataMessage = (Protocol::DataMessage*)(byte_t*)lz4Buffer;
  dataMessage->size = sizeof(Protocol::DataMessage) + compressedSize;
  dataMessage->messageType = Protocol::data;
  dataMessage->connectionId = connectionId;
  dataMessage->originalSize = size;
  //Console::printf("sendData: compressedSize=%d, originalSize=%d, size=%llu\n", compressedSize, (int)dataMessage.originalSize, (uint64_t)size);
  if(!client.send((const byte_t*)dataMessage, dataMessage->size))
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
  client.send((const byte_t*)&suspendMessage, sizeof(suspendMessage));
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
  client.send((const byte_t*)&resumeMessage, sizeof(resumeMessage));
  return true;
}

void_t DownlinkHandler::establish()
{
  Protocol::AuthMessage authMessage;
  authMessage.size = sizeof(authMessage);
  authMessage.messageType = Protocol::auth;
  Protocol::setString(authMessage.secret, clientHandler.getSecret());
  client.send((const byte_t*)&authMessage, sizeof(authMessage));
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
  if(!clientHandler.createConnection(connect.connectionId, connect.port))
    sendDisconnect(connect.connectionId);
}

void_t DownlinkHandler::handleDisconnectMessage(const Protocol::DisconnectMessage& disconnect)
{
  clientHandler.removeConnection(disconnect.connectionId);
}

void_t DownlinkHandler::handleDataMessage(const Protocol::DataMessage& message, byte_t* data, size_t size)
{
  //Console::printf("recvData: compressedSize=%d, originalSize=%d\n", (int)size, (int)message.originalSize);

  int originalSize = message.originalSize;
  lz4Buffer.resize(originalSize);
  if(LZ4_decompress_safe((const char*)data,(char*)(byte_t*)lz4Buffer, size, originalSize) != originalSize)
  {
    client.close();
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

size_t DownlinkHandler::handle(byte_t* data, size_t size)
{
  byte_t* pos = data;
  while(size > 0)
  {
    if(size < sizeof(Protocol::Header))
      break;
    Protocol::Header* header = (Protocol::Header*)pos;
    if(header->size < sizeof(Protocol::Header))
    {
      client.close();
      return 0;
    }
    if(size < header->size)
      break;
    handleMessage((Protocol::MessageType)header->messageType, pos, header->size);
    pos += header->size;
    size -= header->size;
  }
  if(size > LZ4_compressBound(RECV_BUFFER_SIZE) + sizeof(Protocol::DataMessage))
  {
    client.close();
    return 0;
  }
  return pos - data;
}

void_t DownlinkHandler::write()
{
  clientHandler.resumeAllEndpoints();
}
