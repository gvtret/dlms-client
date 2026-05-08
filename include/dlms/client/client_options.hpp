#pragma once

#include "dlms/client/client_status.hpp"

#include <cstdint>

namespace dlms {
namespace client {

enum class ClientProfile
{
  WrapperTcp
};

enum class ClientSecurityMode
{
  None,
  AuthenticatedAndEncrypted
};

struct ClientSecurityOptions
{
  std::uint8_t clientSystemTitle[8];
  std::uint8_t serverSystemTitle[8];
  std::uint8_t globalUnicastEncryptionKey[16];
  std::uint8_t authenticationKey[16];
  std::uint32_t invocationCounter;
};

struct WrapperTcpEndpoint
{
  const char* host;
  std::uint16_t port;
  std::uint16_t sourceWPort;
  std::uint16_t destinationWPort;
};

struct DlmsClientOptions
{
  ClientProfile profile;
  ClientSecurityMode securityMode;
  WrapperTcpEndpoint wrapperTcp;
  ClientSecurityOptions security;
  std::uint16_t clientSap;
  std::uint16_t serverSap;
  std::uint32_t connectTimeoutMs;
  std::uint32_t requestTimeoutMs;
};

DlmsClientOptions DefaultDlmsClientOptions();
ClientStatus ValidateDlmsClientOptions(const DlmsClientOptions& options);

} // namespace client
} // namespace dlms
