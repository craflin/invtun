
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

void_t DownlinkHandler::close()
{
  serverHandler.startReconnectTimer();
}

void_t DownlinkHandler::abolish()
{
  serverHandler.startReconnectTimer();
}

void_t DownlinkHandler::handleMessage(Protocol::MessageType messageType, byte_t* data, size_t size)
{
  switch(messageType)
  {
  //case Protocol::auth:
  //  if(!authed && size >= sizeof(Protocol::AuthMessage))
  //    handleAuthMessage(*(Protocol::AuthMessage*)data);
  //  break;
  //case Protocol::disconnect:
  //  if(authed && size >= sizeof(Protocol::DisconnectMessage))
  //    handleDisconnectMessag(*(Protocol::DisconnectMessage*)data);
  //  break;
  //case Protocol::data:
  //  if(authed && size >= sizeof(Protocol::DataMessage))
  //    handleDataMessag(*(Protocol::DataMessage*)data, data + sizeof(Protocol::DataMessage), size - sizeof(Protocol::DataMessage));
  //  break;
  case Protocol::authResponse:
    if(!authed)
      authed = true;
    break;
  case Protocol::connect:
    if(authed && size >= sizeof(Protocol::ConnectMessage))
      handleConnectMessage(*(Protocol::ConnectMessage*)data);
    break;
  default:
    break;
  }
}

void_t DownlinkHandler::handleConnectMessage(Protocol::ConnectMessage& connect)
{
}

//bool_t DownlinkHandler::createConnection(uint32_t connectionId, uint32_t addr, uint16_t port)
//{
//  UplinkHandler* uplinkHandler = new UplinkHandler();
//  if(!server.connect(addr, port, *uplinkHandler))
//  {
//    // todo: send disconnect answer
//    delete uplinkHandler;
//    return false;
//  }
//  uplinks.append(connectionId, uplinkHandler);
//  return true;
//}
