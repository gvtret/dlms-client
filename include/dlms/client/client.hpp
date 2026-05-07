#pragma once

#include "dlms/client/client_options.hpp"
#include "dlms/client/client_status.hpp"

#include "dlms/association/association_client.hpp"
#include "dlms/profile/apdu_channel.hpp"
#include "dlms/profile/wrapper_tcp_profile_channel.hpp"
#include "dlms/transport/tcp_stream_transport.hpp"
#include "dlms/xdlms/xdlms_client.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace dlms {
namespace client {

enum class ClientState
{
  Disconnected,
  Connected,
  Associated
};

using CosemAttributeDescriptor = dlms::xdlms::CosemAttributeDescriptor;
using CosemMethodDescriptor = dlms::xdlms::CosemMethodDescriptor;

class DlmsClient
{
public:
  explicit DlmsClient(const DlmsClientOptions& options);

  DlmsClient(
    dlms::profile::IApduChannel& channel,
    dlms::association::AssociationClient& association);

  ~DlmsClient();

  ClientStatus Connect();
  ClientStatus OpenAssociation();
  ClientStatus ReleaseAssociation();
  ClientStatus Close();

  ClientState State() const;
  bool IsConnected() const;
  bool IsAssociated() const;

  ClientStatus Get(
    const CosemAttributeDescriptor& descriptor,
    std::vector<std::uint8_t>& encodedData);

  ClientStatus Set(
    const CosemAttributeDescriptor& descriptor,
    const std::vector<std::uint8_t>& encodedData);

  ClientStatus Action(
    const CosemMethodDescriptor& descriptor,
    bool hasParameter,
    const std::vector<std::uint8_t>& encodedParameter,
    std::vector<std::uint8_t>& encodedReturnParameter);

private:
  DlmsClient(const DlmsClient&);
  DlmsClient& operator=(const DlmsClient&);

  std::unique_ptr<dlms::transport::TcpStreamTransport> ownedStream_;
  std::unique_ptr<dlms::profile::WrapperTcpProfileChannel> ownedChannel_;
  std::unique_ptr<dlms::association::AssociationClient> ownedAssociation_;
  dlms::association::AssociationClient& association_;
  dlms::xdlms::XdlmsClient xdlms_;
  ClientState state_;
  ClientStatus constructionStatus_;
};

const char* ClientStateName(ClientState state);

} // namespace client
} // namespace dlms
