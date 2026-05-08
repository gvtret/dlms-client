#include "dlms/client/client_options.hpp"

#include <cstddef>

namespace dlms {
namespace client {
namespace {

template <std::size_t N>
bool IsAllZero(const std::uint8_t (&bytes)[N])
{
  for (std::size_t i = 0u; i < N; ++i) {
    if (bytes[i] != 0u) {
      return false;
    }
  }
  return true;
}

} // namespace

DlmsClientOptions DefaultDlmsClientOptions()
{
  DlmsClientOptions options;
  options.profile = ClientProfile::WrapperTcp;
  options.securityMode = ClientSecurityMode::None;
  options.wrapperTcp.host = "127.0.0.1";
  options.wrapperTcp.port = 4059u;
  options.wrapperTcp.sourceWPort = 16u;
  options.wrapperTcp.destinationWPort = 1u;
  options.clientSap = 16u;
  options.serverSap = 1u;
  options.connectTimeoutMs = 5000u;
  options.requestTimeoutMs = 5000u;
  options.security.invocationCounter = 0u;
  for (std::size_t i = 0u; i < 8u; ++i) {
    options.security.clientSystemTitle[i] = 0u;
    options.security.serverSystemTitle[i] = 0u;
  }
  for (std::size_t i = 0u; i < 16u; ++i) {
    options.security.globalUnicastEncryptionKey[i] = 0u;
    options.security.authenticationKey[i] = 0u;
  }
  return options;
}

ClientStatus ValidateDlmsClientOptions(const DlmsClientOptions& options)
{
  if (options.profile != ClientProfile::WrapperTcp) {
    return ClientStatus::UnsupportedFeature;
  }

  if (options.securityMode != ClientSecurityMode::None &&
      options.securityMode != ClientSecurityMode::AuthenticatedAndEncrypted) {
    return ClientStatus::UnsupportedFeature;
  }

  if (options.wrapperTcp.host == 0 || options.wrapperTcp.host[0] == '\0') {
    return ClientStatus::InvalidArgument;
  }

  if (options.wrapperTcp.port == 0u ||
      options.wrapperTcp.sourceWPort == 0u ||
      options.wrapperTcp.destinationWPort == 0u ||
      options.clientSap == 0u ||
      options.serverSap == 0u ||
      options.connectTimeoutMs == 0u ||
      options.requestTimeoutMs == 0u) {
    return ClientStatus::InvalidArgument;
  }

  if (options.securityMode == ClientSecurityMode::AuthenticatedAndEncrypted &&
      (IsAllZero(options.security.clientSystemTitle) ||
       IsAllZero(options.security.serverSystemTitle) ||
       IsAllZero(options.security.globalUnicastEncryptionKey) ||
       IsAllZero(options.security.authenticationKey))) {
    return ClientStatus::InvalidArgument;
  }

  return ClientStatus::Ok;
}

} // namespace client
} // namespace dlms
