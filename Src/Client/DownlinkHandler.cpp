
#include "DownlinkHandler.h"
#include "ServerHandler.h"

DownlinkHandler::DownlinkHandler(ServerHandler& serverHandler, Server::Client& client) : serverHandler(serverHandler), client(client), authed(false)
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

  Protocol::DataMessage dataMessage;
  dataMessage.size = sizeof(dataMessage) + size;
  dataMessage.messageType = Protocol::data;
  dataMessage.connectionId = connectionId;
  client.reserve(dataMessage.size);
  client.send((const byte_t*)&dataMessage, sizeof(dataMessage));
  client.send(data, size);
  return true;
}

void_t DownlinkHandler::establish()
{
  Protocol::AuthMessage authMessage;
  authMessage.size = sizeof(authMessage);
  authMessage.messageType = Protocol::auth;
  Protocol::setString(authMessage.secret, serverHandler.getSecret());
  client.send((const byte_t*)&authMessage, sizeof(authMessage));
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
  if(size >= 5000)
  {
    client.close();
    return 0;
  }
  return pos - data;
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
  default:
    break;
  }
}

void_t DownlinkHandler::handleConnectMessage(Protocol::ConnectMessage& connect)
{
  if(!serverHandler.createConnection(connect.connectionId, connect.port))
    sendDisconnect(connect.connectionId);
}

void_t DownlinkHandler::handleDisconnectMessage(Protocol::DisconnectMessage& disconnect)
{
  serverHandler.removeConnection(disconnect.connectionId);
}

void_t DownlinkHandler::handleDataMessage(Protocol::DataMessage& message, byte_t* data, size_t size)
{
  serverHandler.sendDataToEndpoint(message.connectionId, data, size);
}
