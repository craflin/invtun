
#include "UplinkHandler.h"
#include "ServerHandler.h"
#include "ClientHandler.h"

UplinkHandler::UplinkHandler(ServerHandler& serverHandler, Server::Client& client) :
  serverHandler(serverHandler), client(client), authed(false) {}

bool_t UplinkHandler::createConnection(uint32_t connectionId, uint16_t port)
{
  if(!authed)
    return false;

  Protocol::ConnectMessage connectMessage;
  connectMessage.size = sizeof(connectMessage);
  connectMessage.messageType = Protocol::connect;
  connectMessage.connectionId = connectionId;
  connectMessage.port = port;
  client.send((const byte_t*)&connectMessage, sizeof(connectMessage));
  return true;
}

bool_t UplinkHandler::sendData(uint32_t connectionId, const byte_t* data, size_t size)
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

size_t UplinkHandler::handle(byte_t* data, size_t size)
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
    handleMessage((Protocol::MessageType)header->messageType, pos + sizeof(Protocol::Header), header->size - sizeof(Protocol::Header));
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
      handleDisconnectMessag(*(Protocol::DisconnectMessage*)data);
    break;
  case Protocol::data:
    if(authed && size >= sizeof(Protocol::DataMessage))
      handleDataMessag(*(Protocol::DataMessage*)data, data + sizeof(Protocol::DataMessage), size - sizeof(Protocol::DataMessage));
    break;
  default:
    break;
  }
}

void_t UplinkHandler::handleAuthMessage(Protocol::AuthMessage& authMessage)
{
  if(Protocol::getString(authMessage.secret) != serverHandler.getSecret())
  {
    client.close();
    return; // invalid secret
  }
  authed = true;
  Protocol::Header response;
  response.size = sizeof(response);
  response.messageType = Protocol::authResponse;
  client.send((const byte_t*)&response, sizeof(response));
}

void_t UplinkHandler::handleDisconnectMessag(Protocol::DisconnectMessage& disconnectMessage)
{
  ClientHandler* client = serverHandler.getClient(disconnectMessage.connectionId);
  if(!client)
    return;
  client->close();
}

void_t UplinkHandler::handleDataMessag(Protocol::DataMessage& dataMessage, byte_t* data, size_t size)
{
  ClientHandler* client = serverHandler.getClient(dataMessage.connectionId);
  if(!client)
    return;
  client->sendData(data, size);
}
