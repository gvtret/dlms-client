#include "dlms/client/client_options.hpp"

namespace dlms {
namespace client {

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
  return options;
}

ClientStatus ValidateDlmsClientOptions(const DlmsClientOptions& options)
{
  if (options.profile != ClientProfile::WrapperTcp) {
    return ClientStatus::UnsupportedFeature;
  }

  if (options.securityMode != ClientSecurityMode::None) {
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

  return ClientStatus::Ok;
}

} // namespace client
} // namespace dlms
