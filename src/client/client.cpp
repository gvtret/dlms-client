#include "dlms/client/client.hpp"

#include "dlms/association/association_types.hpp"
#include "dlms/profile/profile_types.hpp"
#include "dlms/security/ciphered_apdu_processor.hpp"
#include "dlms/security/in_memory_invocation_counter_store.hpp"
#include "dlms/security/in_memory_key_store.hpp"

#include <cstddef>
#include <memory>

namespace dlms {
namespace client {
namespace {

ClientStatus MapAssociationStatus(
  dlms::association::AssociationStatus status)
{
  switch (status) {
  case dlms::association::AssociationStatus::Ok:
  case dlms::association::AssociationStatus::AlreadyAssociated:
    return ClientStatus::Ok;
  case dlms::association::AssociationStatus::InvalidArgument:
    return ClientStatus::InvalidArgument;
  case dlms::association::AssociationStatus::InvalidState:
    return ClientStatus::InvalidState;
  case dlms::association::AssociationStatus::ChannelOpenFailed:
    return ClientStatus::ChannelOpenFailed;
  case dlms::association::AssociationStatus::SendFailed:
    return ClientStatus::SendFailed;
  case dlms::association::AssociationStatus::ReceiveFailed:
    return ClientStatus::ReceiveFailed;
  case dlms::association::AssociationStatus::UnsupportedApplicationContext:
  case dlms::association::AssociationStatus::UnsupportedAuthentication:
    return ClientStatus::UnsupportedFeature;
  case dlms::association::AssociationStatus::AssociationRejected:
  case dlms::association::AssociationStatus::NegotiationFailed:
  case dlms::association::AssociationStatus::EncodeFailed:
  case dlms::association::AssociationStatus::DecodeFailed:
    return ClientStatus::AssociationFailed;
  case dlms::association::AssociationStatus::ChannelCloseFailed:
  case dlms::association::AssociationStatus::InternalError:
    return ClientStatus::InternalError;
  }

  return ClientStatus::InternalError;
}

ClientStatus MapXdlmsStatus(dlms::xdlms::XdlmsStatus status)
{
  switch (status) {
  case dlms::xdlms::XdlmsStatus::Ok:
    return ClientStatus::Ok;
  case dlms::xdlms::XdlmsStatus::InvalidArgument:
    return ClientStatus::InvalidArgument;
  case dlms::xdlms::XdlmsStatus::InvalidState:
    return ClientStatus::InvalidState;
  case dlms::xdlms::XdlmsStatus::NotAssociated:
    return ClientStatus::NotAssociated;
  case dlms::xdlms::XdlmsStatus::SendFailed:
    return ClientStatus::SendFailed;
  case dlms::xdlms::XdlmsStatus::ReceiveFailed:
    return ClientStatus::ReceiveFailed;
  case dlms::xdlms::XdlmsStatus::ServiceRejected:
    return ClientStatus::ServiceRejected;
  case dlms::xdlms::XdlmsStatus::SecurityFailed:
    return ClientStatus::SecurityFailed;
  case dlms::xdlms::XdlmsStatus::BlockTransferRequired:
  case dlms::xdlms::XdlmsStatus::UnsupportedFeature:
    return ClientStatus::UnsupportedFeature;
  case dlms::xdlms::XdlmsStatus::EncodeFailed:
  case dlms::xdlms::XdlmsStatus::DecodeFailed:
  case dlms::xdlms::XdlmsStatus::InvokeIdMismatch:
  case dlms::xdlms::XdlmsStatus::InternalError:
    return ClientStatus::InternalError;
  }

  return ClientStatus::InternalError;
}

dlms::transport::TcpStreamTransportOptions MakeTcpOptions(
  const DlmsClientOptions& options)
{
  dlms::transport::TcpStreamTransportOptions tcp;
  tcp.host = options.wrapperTcp.host == 0 ? "" : options.wrapperTcp.host;
  tcp.port = options.wrapperTcp.port;
  tcp.connectTimeout.milliseconds = options.connectTimeoutMs;
  tcp.readTimeout.milliseconds = options.requestTimeoutMs;
  tcp.writeTimeout.milliseconds = options.requestTimeoutMs;
  return tcp;
}

dlms::profile::ApduChannelOptions MakeWrapperTcpChannelOptions(
  const DlmsClientOptions& options)
{
  dlms::profile::ApduChannelOptions channel =
    dlms::profile::DefaultApduChannelOptions();
  channel.localWrapperPort = options.wrapperTcp.sourceWPort;
  channel.remoteWrapperPort = options.wrapperTcp.destinationWPort;
  return channel;
}

dlms::association::AssociationOptions MakeAssociationOptions(
  const DlmsClientOptions& options)
{
  dlms::association::AssociationOptions association =
    dlms::association::DefaultAssociationOptions();
  if (options.authenticationMode ==
      ClientAuthenticationMode::LowLevelSecurity) {
    association.authenticationMode =
      dlms::association::AuthenticationMode::LowLevelSecurity;
    if (options.lowLevelSecurity.credential != 0 &&
        options.lowLevelSecurity.credentialSize != 0u) {
      association.lowLevelSecurityCredential.assign(
        options.lowLevelSecurity.credential,
        options.lowLevelSecurity.credential +
          options.lowLevelSecurity.credentialSize);
    }
  } else {
    association.authenticationMode =
      dlms::association::AuthenticationMode::None;
  }
  return association;
}

dlms::security::SecurityKey MakeSecurityKey(
  dlms::security::SecurityKeyRole role,
  const std::uint8_t bytes[16])
{
  dlms::security::SecurityKey key = dlms::security::EmptySecurityKey(role);
  key.size = 16u;
  for (std::size_t i = 0u; i < 16u; ++i) {
    key.bytes[i] = bytes[i];
  }
  return key;
}

std::unique_ptr<dlms::security::SecurityContext> CreateSecurityContext(
  const DlmsClientOptions& options)
{
  std::unique_ptr<dlms::security::SecurityContext> context(
    new dlms::security::SecurityContext(
      dlms::security::EmptySecurityContext()));
  context->policy =
    dlms::security::SecurityPolicy::AuthenticatedAndEncrypted;
  context->role = dlms::security::SecurityRole::Client;
  context->clientSap = options.clientSap;
  context->serverSap = options.serverSap;
  for (std::size_t i = 0u; i < 8u; ++i) {
    context->localSystemTitle[i] = options.security.clientSystemTitle[i];
    context->remoteSystemTitle[i] = options.security.serverSystemTitle[i];
  }
  return context;
}

std::unique_ptr<dlms::transport::TcpStreamTransport> CreateTcpStream(
  const DlmsClientOptions& options)
{
  return std::unique_ptr<dlms::transport::TcpStreamTransport>(
    new dlms::transport::TcpStreamTransport(MakeTcpOptions(options)));
}

std::unique_ptr<dlms::profile::WrapperTcpProfileChannel> CreateWrapperChannel(
  dlms::transport::IByteStream& stream,
  const DlmsClientOptions& options)
{
  return std::unique_ptr<dlms::profile::WrapperTcpProfileChannel>(
    new dlms::profile::WrapperTcpProfileChannel(
      stream,
      MakeWrapperTcpChannelOptions(options)));
}

std::unique_ptr<dlms::association::AssociationClient> CreateAssociation(
  dlms::profile::IApduChannel& channel,
  const DlmsClientOptions& options)
{
  return std::unique_ptr<dlms::association::AssociationClient>(
    new dlms::association::AssociationClient(
      channel,
      MakeAssociationOptions(options)));
}

} // namespace

DlmsClient::DlmsClient(const DlmsClientOptions& options)
  : ownedStream_(CreateTcpStream(options))
  , ownedChannel_(CreateWrapperChannel(*ownedStream_, options))
  , ownedAssociation_(CreateAssociation(*ownedChannel_, options))
  , ownedSecurityContext_()
  , ownedKeys_()
  , ownedCounters_()
  , ownedSecurity_()
  , association_(*ownedAssociation_)
  , xdlms_()
  , state_(ClientState::Disconnected)
  , constructionStatus_(ValidateDlmsClientOptions(options))
{
  if (constructionStatus_ != ClientStatus::Ok ||
      options.securityMode == ClientSecurityMode::None) {
    xdlms_.reset(
      new dlms::xdlms::XdlmsClient(*ownedChannel_, *ownedAssociation_));
    return;
  }

  ownedSecurityContext_ = CreateSecurityContext(options);
  ownedKeys_.reset(new dlms::security::InMemoryKeyStore());
  ownedCounters_.reset(
    new dlms::security::InMemoryInvocationCounterStore());
  ownedCounters_->SetLocalCounter(options.security.invocationCounter);

  const dlms::security::SecurityStatus encryptionKeyStatus =
    ownedKeys_->SetKey(
      MakeSecurityKey(
        dlms::security::SecurityKeyRole::GlobalUnicastEncryption,
        options.security.globalUnicastEncryptionKey));
  const dlms::security::SecurityStatus authenticationKeyStatus =
    ownedKeys_->SetKey(
      MakeSecurityKey(
        dlms::security::SecurityKeyRole::Authentication,
        options.security.authenticationKey));
  if (encryptionKeyStatus != dlms::security::SecurityStatus::Ok ||
      authenticationKeyStatus != dlms::security::SecurityStatus::Ok) {
    constructionStatus_ = ClientStatus::InvalidArgument;
    xdlms_.reset(
      new dlms::xdlms::XdlmsClient(*ownedChannel_, *ownedAssociation_));
    return;
  }

  ownedSecurity_.reset(
    new dlms::security::CipheredApduProcessor(
      *ownedSecurityContext_,
      *ownedKeys_,
      *ownedCounters_));
  xdlms_.reset(
    new dlms::xdlms::XdlmsClient(
      *ownedChannel_,
      *ownedAssociation_,
      *ownedSecurity_));
}

DlmsClient::DlmsClient(
  dlms::profile::IApduChannel& channel,
  dlms::association::AssociationClient& association)
  : ownedStream_()
  , ownedChannel_()
  , ownedAssociation_()
  , ownedSecurityContext_()
  , ownedKeys_()
  , ownedCounters_()
  , ownedSecurity_()
  , association_(association)
  , xdlms_(new dlms::xdlms::XdlmsClient(channel, association))
  , state_(ClientState::Disconnected)
  , constructionStatus_(ClientStatus::Ok)
{
}

DlmsClient::DlmsClient(
  dlms::profile::IApduChannel& channel,
  dlms::association::AssociationClient& association,
  dlms::security::CipheredApduProcessor& security)
  : ownedStream_()
  , ownedChannel_()
  , ownedAssociation_()
  , ownedSecurityContext_()
  , ownedKeys_()
  , ownedCounters_()
  , ownedSecurity_()
  , association_(association)
  , xdlms_(new dlms::xdlms::XdlmsClient(channel, association, security))
  , state_(ClientState::Disconnected)
  , constructionStatus_(ClientStatus::Ok)
{
}

DlmsClient::~DlmsClient()
{
}

ClientStatus DlmsClient::Connect()
{
  if (constructionStatus_ != ClientStatus::Ok) {
    return constructionStatus_;
  }

  if (state_ != ClientState::Disconnected) {
    return ClientStatus::Ok;
  }

  const ClientStatus status = MapAssociationStatus(association_.Open());
  if (status != ClientStatus::Ok) {
    return status;
  }

  state_ = ClientState::Connected;
  return ClientStatus::Ok;
}

ClientStatus DlmsClient::OpenAssociation()
{
  if (state_ == ClientState::Disconnected) {
    return ClientStatus::InvalidState;
  }

  if (state_ == ClientState::Associated) {
    return ClientStatus::Ok;
  }

  const ClientStatus status = MapAssociationStatus(association_.Establish());
  if (status != ClientStatus::Ok) {
    return status;
  }

  state_ = ClientState::Associated;
  return ClientStatus::Ok;
}

ClientStatus DlmsClient::ReleaseAssociation()
{
  if (state_ != ClientState::Associated) {
    return ClientStatus::Ok;
  }

  const ClientStatus status = MapAssociationStatus(association_.Release());
  if (status != ClientStatus::Ok) {
    return status;
  }

  state_ = ClientState::Disconnected;
  return ClientStatus::Ok;
}

ClientStatus DlmsClient::Close()
{
  if (state_ == ClientState::Disconnected) {
    return ClientStatus::Ok;
  }

  const ClientStatus status = MapAssociationStatus(association_.Close());
  if (status != ClientStatus::Ok) {
    return status;
  }

  state_ = ClientState::Disconnected;
  return ClientStatus::Ok;
}

ClientState DlmsClient::State() const
{
  return state_;
}

bool DlmsClient::IsConnected() const
{
  return state_ != ClientState::Disconnected;
}

bool DlmsClient::IsAssociated() const
{
  return state_ == ClientState::Associated && association_.IsAssociated();
}

ClientStatus DlmsClient::Get(
  const CosemAttributeDescriptor& descriptor,
  std::vector<std::uint8_t>& encodedData)
{
  encodedData.clear();
  if (state_ != ClientState::Associated) {
    return ClientStatus::NotAssociated;
  }

  dlms::xdlms::GetResult result = dlms::xdlms::EmptyGetResult();
  const ClientStatus status = MapXdlmsStatus(xdlms_->Get(descriptor, result));
  if (status != ClientStatus::Ok) {
    return status;
  }

  encodedData = result.data;
  return ClientStatus::Ok;
}

ClientStatus DlmsClient::Set(
  const CosemAttributeDescriptor& descriptor,
  const std::vector<std::uint8_t>& encodedData)
{
  if (state_ != ClientState::Associated) {
    return ClientStatus::NotAssociated;
  }

  dlms::xdlms::SetResult result = dlms::xdlms::EmptySetResult();
  return MapXdlmsStatus(xdlms_->Set(descriptor, encodedData, result));
}

ClientStatus DlmsClient::Action(
  const CosemMethodDescriptor& descriptor,
  bool hasParameter,
  const std::vector<std::uint8_t>& encodedParameter,
  std::vector<std::uint8_t>& encodedReturnParameter)
{
  encodedReturnParameter.clear();
  if (state_ != ClientState::Associated) {
    return ClientStatus::NotAssociated;
  }

  dlms::xdlms::ActionResult result = dlms::xdlms::EmptyActionResult();
  const ClientStatus status =
    MapXdlmsStatus(xdlms_->Action(
      descriptor,
      hasParameter,
      encodedParameter,
      result));
  if (status != ClientStatus::Ok) {
    return status;
  }

  if (result.hasData) {
    encodedReturnParameter = result.data;
  }
  return ClientStatus::Ok;
}

const char* ClientStateName(ClientState state)
{
  switch (state) {
  case ClientState::Disconnected:
    return "Disconnected";
  case ClientState::Connected:
    return "Connected";
  case ClientState::Associated:
    return "Associated";
  }

  return "Unknown";
}

} // namespace client
} // namespace dlms
