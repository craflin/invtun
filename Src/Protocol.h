
#pragma once

#include <nstd/String.h>

class Protocol
{
public:
  enum MessageType
  {
    error,
    auth,
    authResponse,
    connect,
    disconnect,
    data,
    suspend,
    resume,
  };

#pragma pack(push, 1)
  struct Header
  {
    uint32 size:24;
    uint8 messageType; // MessageType
  };

  struct AuthMessage : public Header
  {
    char secret[33];
  };

  struct ConnectMessage : public Header
  {
    uint32 connectionId;
    uint16 port;
  };

  struct DisconnectMessage : public Header
  {
    uint32 connectionId;
  };

  struct DataMessage : public Header
  {
    uint32 connectionId;
    uint32 originalSize:24;
  };

  struct SuspendMessage : public Header
  {
    uint32 connectionId;
  };

  struct ResumeMessage : public Header
  {
    uint32 connectionId;
  };
#pragma pack(pop)

  template<size_t N> static void setString(char(&str)[N], const String& value)
  {
    size_t size = value.length() + 1;
    if(size > N - 1)
      size = N - 1;
    Memory::copy(str, (const char*)value, size);
    str[N - 1] = '\0';
  }

  template<size_t N> static String getString(char(&str)[N])
  {
    str[N - 1] = '\0';
    String result;
    result.attach(str, String::length(str));
    return result;
  }
};
