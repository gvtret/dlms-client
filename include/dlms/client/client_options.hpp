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
  None
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
  std::uint16_t clientSap;
  std::uint16_t serverSap;
  std::uint32_t connectTimeoutMs;
  std::uint32_t requestTimeoutMs;
};

DlmsClientOptions DefaultDlmsClientOptions();
ClientStatus ValidateDlmsClientOptions(const DlmsClientOptions& options);

} // namespace client
} // namespace dlms
