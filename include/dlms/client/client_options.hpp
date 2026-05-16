#pragma once

#include "dlms/association/association_types.hpp"
#include "dlms/client/client_status.hpp"
#include "dlms/profile/profile_types.hpp"

#include <cstddef>
#include <cstdint>

namespace dlms {
namespace client {

enum class ClientProfile
{
  WrapperTcp,
  HdlcTcp
};

enum class ClientSecurityMode
{
  None,
  AuthenticatedAndEncrypted
};

enum class ClientAuthenticationMode
{
  None,
  LowLevelSecurity,
  HighLevelSecurity,
  HighLevelSecurityGmac
};

struct ClientLowLevelSecurityOptions
{
  const std::uint8_t* credential;
  std::size_t credentialSize;
};

struct ClientHighLevelSecurityOptions
{
  const std::uint8_t* password;
  std::size_t passwordSize;
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

struct HdlcTcpEndpoint
{
  const char* host;
  std::uint16_t port;
  std::uint8_t clientAddress;
  std::uint16_t logicalDeviceAddress;
  std::uint16_t physicalDeviceAddress;
  std::size_t maxInfoTx;
  std::size_t maxInfoRx;
  std::uint8_t windowSizeTx;
  std::uint8_t windowSizeRx;
  std::uint8_t retryCount;
  std::uint32_t retryDelayMs;
};

struct DlmsClientOptions
{
  ClientProfile profile;
  ClientAuthenticationMode authenticationMode;
  ClientSecurityMode securityMode;
  WrapperTcpEndpoint wrapperTcp;
  HdlcTcpEndpoint hdlcTcp;
  ClientLowLevelSecurityOptions lowLevelSecurity;
  ClientHighLevelSecurityOptions highLevelSecurity;
  ClientSecurityOptions security;
  dlms::profile::IWrapperTcpTraceSink* wrapperTcpTraceSink;
  dlms::association::IAssociationTraceSink* associationTraceSink;
  std::uint16_t associationClientMaxReceivePduSize;
  std::uint16_t clientSap;
  std::uint16_t serverSap;
  std::uint32_t connectTimeoutMs;
  std::uint32_t requestTimeoutMs;
};

DlmsClientOptions DefaultDlmsClientOptions();
ClientStatus ValidateDlmsClientOptions(const DlmsClientOptions& options);

} // namespace client
} // namespace dlms
