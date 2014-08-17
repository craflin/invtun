
#pragma once

#include <nstd/Base.h>

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

#pragma pack(push, 4)
  struct Header
  {
    uint32_t size;
    uint16_t messageType; // MessageType
  };

  struct AuthMessage : public Header
  {
    char_t secret[33];
  };

  struct ConnectMessage : public Header
  {
    uint32_t connectionId;
    uint16_t port;
  };

  struct DisconnectMessage : public Header
  {
    uint32_t connectionId;
  };

  struct DataMessage : public Header
  {
    uint32_t connectionId;
  };

  struct SuspendMessage : public Header
  {
    uint32_t connectionId;
  };

  struct ResumeMessage : public Header
  {
    uint32_t connectionId;
  };
#pragma pack(pop)

  template<size_t N> static void_t setString(char_t(&str)[N], const String& value)
  {
    size_t size = value.length() + 1;
    if(size > N - 1)
      size = N - 1;
    Memory::copy(str, (const char_t*)value, size);
    str[N - 1] = '\0';
  }
  
  template<size_t N> static String getString(char_t(&str)[N])
  {
    str[N - 1] = '\0';
    String result;
    result.attach(str, String::length(str));
    return result;
  }
};
