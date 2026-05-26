#include "dlms/client/client.hpp"

#include "dlms/apdu/data.hpp"
#include "dlms/association/association_types.hpp"
#include "dlms/profile/hdlc_profile_channel.hpp"
#include "dlms/profile/profile_types.hpp"
#include "dlms/profile/wrapper_tcp_profile_channel.hpp"
#include "dlms/security/ciphered_apdu_processor.hpp"
#include "dlms/security/hls_high_authenticator.hpp"
#include "dlms/security/hls_gmac_authenticator.hpp"
#include "dlms/security/in_memory_invocation_counter_store.hpp"
#include "dlms/security/in_memory_key_store.hpp"
#include "dlms/security/random_source.hpp"

#include <cstddef>
#include <memory>
#include <openssl/rand.h>

namespace dlms {
namespace client {
namespace {

class OpenSslRandomSource : public dlms::security::IRandomSource
{
public:
  dlms::security::SecurityStatus Fill(
    std::uint8_t* output,
    std::size_t outputSize) override
  {
    if (output == 0 && outputSize != 0u) {
      return dlms::security::SecurityStatus::InvalidArgument;
    }
    if (outputSize == 0u) {
      return dlms::security::SecurityStatus::Ok;
    }
    return RAND_bytes(output, static_cast<int>(outputSize)) == 1
      ? dlms::security::SecurityStatus::Ok
      : dlms::security::SecurityStatus::InternalError;
  }
};

dlms::security::SecurityByteView SecurityView(
  const std::vector<std::uint8_t>& bytes)
{
  dlms::security::SecurityByteView view;
  view.data = bytes.empty() ? 0 : &bytes[0];
  view.size = bytes.size();
  return view;
}

bool IsZeroSystemTitle(const std::uint8_t title[8])
{
  for (std::size_t i = 0u; i < 8u; ++i) {
    if (title[i] != 0u) {
      return false;
    }
  }
  return true;
}

bool ApplyDiscoveredRemoteSystemTitle(
  dlms::security::SecurityContext* context,
  const dlms::association::AssociationResult& result)
{
  if (context == 0) {
    return false;
  }

  if (!IsZeroSystemTitle(context->remoteSystemTitle)) {
    return true;
  }

  if (result.respondingApplicationTitle.size() != 8u) {
    return false;
  }

  for (std::size_t i = 0u; i < 8u; ++i) {
    context->remoteSystemTitle[i] = result.respondingApplicationTitle[i];
  }
  return true;
}

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
  if (options.profile == ClientProfile::HdlcTcp) {
    tcp.host = options.hdlcTcp.host == 0 ? "" : options.hdlcTcp.host;
    tcp.port = options.hdlcTcp.port;
  } else {
    tcp.host = options.wrapperTcp.host == 0 ? "" : options.wrapperTcp.host;
    tcp.port = options.wrapperTcp.port;
  }
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
  channel.wrapperTcpTraceSink = options.wrapperTcpTraceSink;
  return channel;
}

dlms::profile::ApduChannelOptions MakeHdlcTcpChannelOptions(
  const DlmsClientOptions& options)
{
  dlms::profile::ApduChannelOptions channel =
    dlms::profile::DefaultApduChannelOptions();
  channel.hdlcClientAddress = options.hdlcTcp.clientAddress;
  channel.hdlcLogicalDeviceAddress = options.hdlcTcp.logicalDeviceAddress;
  channel.hdlcPhysicalDeviceAddress = options.hdlcTcp.physicalDeviceAddress;
  channel.hdlcDirection = dlms::profile::HdlcProfileDirection::ClientToServer;
  channel.hdlcRole = dlms::profile::HdlcProfileRole::Client;
  channel.hdlcUseSession = true;
  channel.hdlcMaxInformationFieldLengthTransmit = options.hdlcTcp.maxInfoTx;
  channel.hdlcMaxInformationFieldLengthReceive = options.hdlcTcp.maxInfoRx;
  channel.hdlcWindowSizeTransmit = options.hdlcTcp.windowSizeTx;
  channel.hdlcWindowSizeReceive = options.hdlcTcp.windowSizeRx;
  channel.hdlcRetryCount = options.hdlcTcp.retryCount;
  channel.hdlcRetryDelayMilliseconds = options.hdlcTcp.retryDelayMs;
  return channel;
}

dlms::association::AssociationOptions MakeAssociationOptions(
  const DlmsClientOptions& options,
  const dlms::association::IHighLevelSecurityStrategy* hlsStrategy)
{
  dlms::association::AssociationOptions association =
    dlms::association::DefaultAssociationOptions();
  association.traceSink = options.associationTraceSink;
  association.hasProposedQualityOfService =
    options.associationHasProposedQualityOfService;
  association.proposedQualityOfService =
    options.associationProposedQualityOfService;
  association.proposedDlmsVersionNumber =
    options.associationProposedDlmsVersionNumber;
  association.proposedConformance = options.associationProposedConformance;
  association.clientMaxReceivePduSize =
    options.associationClientMaxReceivePduSize;
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
  } else if (options.authenticationMode ==
             ClientAuthenticationMode::HighLevelSecurity) {
    association.authenticationMode =
      dlms::association::AuthenticationMode::HighLevelSecurity;
    association.highLevelSecurity = hlsStrategy;
  } else if (options.authenticationMode ==
             ClientAuthenticationMode::HighLevelSecurityGmac) {
    association.authenticationMode =
      dlms::association::AuthenticationMode::HighLevelSecurity;
    association.highLevelSecurity = hlsStrategy;
    association.callingApplicationTitle.assign(
      options.security.clientSystemTitle,
      options.security.clientSystemTitle + 8u);
  } else {
    association.authenticationMode =
      dlms::association::AuthenticationMode::None;
  }
  return association;
}

bool UsesSecurityComponents(const DlmsClientOptions& options)
{
  return options.securityMode ==
           ClientSecurityMode::AuthenticatedAndEncrypted ||
         options.authenticationMode ==
           ClientAuthenticationMode::HighLevelSecurityGmac;
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
  if (!UsesSecurityComponents(options)) {
    return std::unique_ptr<dlms::security::SecurityContext>();
  }

  std::unique_ptr<dlms::security::SecurityContext> context(
    new dlms::security::SecurityContext(
      dlms::security::EmptySecurityContext()));
  context->policy = options.securityMode ==
      ClientSecurityMode::AuthenticatedAndEncrypted
    ? dlms::security::SecurityPolicy::AuthenticatedAndEncrypted
    : dlms::security::SecurityPolicy::Authenticated;
  context->role = dlms::security::SecurityRole::Client;
  context->clientSap = options.clientSap;
  context->serverSap = options.serverSap;
  for (std::size_t i = 0u; i < 8u; ++i) {
    context->localSystemTitle[i] = options.security.clientSystemTitle[i];
    context->remoteSystemTitle[i] = options.security.serverSystemTitle[i];
  }
  return context;
}

std::unique_ptr<dlms::security::InMemoryKeyStore> CreateKeyStore(
  const DlmsClientOptions& options)
{
  if (!UsesSecurityComponents(options)) {
    return std::unique_ptr<dlms::security::InMemoryKeyStore>();
  }

  std::unique_ptr<dlms::security::InMemoryKeyStore> keys(
    new dlms::security::InMemoryKeyStore());
  keys->SetKey(
    MakeSecurityKey(
      dlms::security::SecurityKeyRole::GlobalUnicastEncryption,
      options.security.globalUnicastEncryptionKey));
  keys->SetKey(
    MakeSecurityKey(
      dlms::security::SecurityKeyRole::Authentication,
      options.security.authenticationKey));
  return keys;
}

std::unique_ptr<dlms::security::InMemoryInvocationCounterStore>
CreateCounterStore(const DlmsClientOptions& options)
{
  if (!UsesSecurityComponents(options)) {
    return std::unique_ptr<dlms::security::InMemoryInvocationCounterStore>();
  }

  std::unique_ptr<dlms::security::InMemoryInvocationCounterStore> counters(
    new dlms::security::InMemoryInvocationCounterStore());
  counters->SetLocalCounter(options.security.invocationCounter);
  return counters;
}

std::unique_ptr<dlms::security::IRandomSource> CreateRandomSource(
  const DlmsClientOptions& options)
{
  if (options.authenticationMode !=
        ClientAuthenticationMode::HighLevelSecurity &&
      options.authenticationMode !=
      ClientAuthenticationMode::HighLevelSecurityGmac) {
    return std::unique_ptr<dlms::security::IRandomSource>();
  }
  return std::unique_ptr<dlms::security::IRandomSource>(
    new OpenSslRandomSource());
}

std::unique_ptr<dlms::security::HlsHighAuthenticator>
CreateHlsHighAuthenticator(
  const DlmsClientOptions& options,
  dlms::security::IRandomSource* random)
{
  if (options.authenticationMode !=
      ClientAuthenticationMode::HighLevelSecurity ||
      random == 0) {
    return std::unique_ptr<dlms::security::HlsHighAuthenticator>();
  }

  dlms::security::SecurityByteView password;
  password.data = options.highLevelSecurity.password;
  password.size = options.highLevelSecurity.passwordSize;
  return std::unique_ptr<dlms::security::HlsHighAuthenticator>(
    new dlms::security::HlsHighAuthenticator(password, *random));
}

std::unique_ptr<dlms::security::HlsGmacAuthenticator> CreateHlsAuthenticator(
  const DlmsClientOptions& options,
  dlms::security::SecurityContext* context,
  dlms::security::InMemoryKeyStore* keys,
  dlms::security::InMemoryInvocationCounterStore* counters,
  dlms::security::IRandomSource* random)
{
  if (options.authenticationMode !=
      ClientAuthenticationMode::HighLevelSecurityGmac ||
      context == 0 ||
      keys == 0 ||
      counters == 0 ||
      random == 0) {
    return std::unique_ptr<dlms::security::HlsGmacAuthenticator>();
  }

  return std::unique_ptr<dlms::security::HlsGmacAuthenticator>(
    new dlms::security::HlsGmacAuthenticator(
      *context,
      *keys,
      *counters,
      *random));
}

std::unique_ptr<dlms::transport::TcpStreamTransport> CreateTcpStream(
  const DlmsClientOptions& options)
{
  return std::unique_ptr<dlms::transport::TcpStreamTransport>(
    new dlms::transport::TcpStreamTransport(MakeTcpOptions(options)));
}

std::unique_ptr<dlms::profile::IApduChannel> CreateProfileChannel(
  dlms::transport::IByteStream& stream,
  const DlmsClientOptions& options)
{
  if (options.profile == ClientProfile::HdlcTcp) {
    return std::unique_ptr<dlms::profile::IApduChannel>(
      new dlms::profile::HdlcProfileChannel(
        stream,
        MakeHdlcTcpChannelOptions(options)));
  }

  return std::unique_ptr<dlms::profile::IApduChannel>(
    new dlms::profile::WrapperTcpProfileChannel(
      stream,
      MakeWrapperTcpChannelOptions(options)));
}

} // namespace

class ClientHlsAssociationStrategy
  : public dlms::association::IHighLevelSecurityStrategy
{
public:
  const std::vector<std::uint8_t>& ClientChallenge() const
  {
    return clientChallenge_;
  }

  virtual dlms::security::SecurityStatus BuildResponse(
    dlms::security::SecurityByteView challenge,
    std::vector<std::uint8_t>& response) const = 0;

  virtual dlms::security::SecurityStatus VerifyResponse(
    dlms::security::SecurityByteView challenge,
    dlms::security::SecurityByteView response) const = 0;

protected:
  mutable std::vector<std::uint8_t> clientChallenge_;
};

class ClientHlsHighAssociationStrategy : public ClientHlsAssociationStrategy
{
public:
  explicit ClientHlsHighAssociationStrategy(
    dlms::security::HlsHighAuthenticator& hls)
    : hls_(hls)
  {
  }

  dlms::association::HighLevelSecurityMechanism Mechanism() const override
  {
    return dlms::association::HighLevelSecurityMechanism::HlsHigh;
  }

  dlms::association::AssociationStatus BuildInitialChallenge(
    std::vector<std::uint8_t>& output) const override
  {
    output.clear();
    if (hls_.BuildChallenge(output) != dlms::security::SecurityStatus::Ok) {
      return dlms::association::AssociationStatus::UnsupportedAuthentication;
    }
    clientChallenge_ = output;
    return dlms::association::AssociationStatus::Ok;
  }

  dlms::security::SecurityStatus BuildResponse(
    dlms::security::SecurityByteView challenge,
    std::vector<std::uint8_t>& response) const override
  {
    return hls_.BuildResponse(challenge, response);
  }

  dlms::security::SecurityStatus VerifyResponse(
    dlms::security::SecurityByteView challenge,
    dlms::security::SecurityByteView response) const override
  {
    return hls_.VerifyResponse(challenge, response);
  }

private:
  dlms::security::HlsHighAuthenticator& hls_;
};

class ClientHlsGmacAssociationStrategy : public ClientHlsAssociationStrategy
{
public:
  explicit ClientHlsGmacAssociationStrategy(
    dlms::security::HlsGmacAuthenticator& hls)
    : hls_(hls)
  {
  }

  dlms::association::HighLevelSecurityMechanism Mechanism() const override
  {
    return dlms::association::HighLevelSecurityMechanism::HlsGmac;
  }

  dlms::association::AssociationStatus BuildInitialChallenge(
    std::vector<std::uint8_t>& output) const override
  {
    output.clear();
    if (hls_.BuildChallenge(output) != dlms::security::SecurityStatus::Ok) {
      return dlms::association::AssociationStatus::UnsupportedAuthentication;
    }
    clientChallenge_ = output;
    return dlms::association::AssociationStatus::Ok;
  }

  dlms::security::SecurityStatus BuildResponse(
    dlms::security::SecurityByteView challenge,
    std::vector<std::uint8_t>& response) const override
  {
    return hls_.BuildResponse(challenge, response);
  }

  dlms::security::SecurityStatus VerifyResponse(
    dlms::security::SecurityByteView challenge,
    dlms::security::SecurityByteView response) const override
  {
    return hls_.VerifyResponse(challenge, response);
  }

private:
  dlms::security::HlsGmacAuthenticator& hls_;
};

namespace {

std::unique_ptr<ClientHlsAssociationStrategy> CreateHlsStrategy(
  const DlmsClientOptions& options,
  dlms::security::HlsHighAuthenticator* hlsHigh,
  dlms::security::HlsGmacAuthenticator* hls)
{
  if (options.authenticationMode ==
      ClientAuthenticationMode::HighLevelSecurity) {
    if (hlsHigh == 0) {
      return std::unique_ptr<ClientHlsAssociationStrategy>();
    }
    return std::unique_ptr<ClientHlsAssociationStrategy>(
      new ClientHlsHighAssociationStrategy(*hlsHigh));
  }

  if (options.authenticationMode ==
      ClientAuthenticationMode::HighLevelSecurityGmac) {
    if (hls == 0) {
      return std::unique_ptr<ClientHlsAssociationStrategy>();
    }
    return std::unique_ptr<ClientHlsAssociationStrategy>(
      new ClientHlsGmacAssociationStrategy(*hls));
  }

  return std::unique_ptr<ClientHlsAssociationStrategy>();
}

std::unique_ptr<dlms::association::AssociationClient> CreateAssociation(
  dlms::profile::IApduChannel& channel,
  const DlmsClientOptions& options,
  const dlms::association::IHighLevelSecurityStrategy* hlsStrategy)
{
  return std::unique_ptr<dlms::association::AssociationClient>(
    new dlms::association::AssociationClient(
      channel,
      MakeAssociationOptions(options, hlsStrategy)));
}

dlms::xdlms::CosemMethodDescriptor HlsReplyMethod()
{
  dlms::xdlms::CosemMethodDescriptor descriptor =
    dlms::xdlms::EmptyCosemMethodDescriptor();
  descriptor.classId = 15u;
  descriptor.instanceId =
    dlms::xdlms::CosemLogicalName(0, 0, 40, 0, 0, 255);
  descriptor.methodId = 1u;
  return descriptor;
}

ClientStatus EncodeOctetStringData(
  const std::vector<std::uint8_t>& bytes,
  std::vector<std::uint8_t>& encoded)
{
  encoded.clear();
  dlms::apdu::DlmsData data = {};
  data.type = dlms::apdu::DlmsDataType::OctetString;
  data.bytes.data = bytes.empty() ? 0 : &bytes[0];
  data.bytes.size = bytes.size();

  std::uint8_t buffer[256] = {};
  dlms::apdu::ApduWriter writer(buffer, sizeof(buffer));
  const dlms::apdu::ApduStatus status =
    dlms::apdu::EncodeDlmsData(data, writer);
  if (status != dlms::apdu::ApduStatus::Ok) {
    return ClientStatus::InternalError;
  }

  encoded.assign(buffer, buffer + writer.WrittenSize());
  return ClientStatus::Ok;
}

ClientStatus DecodeOctetStringData(
  const std::vector<std::uint8_t>& encoded,
  std::vector<std::uint8_t>& bytes)
{
  bytes.clear();
  if (encoded.empty()) {
    return ClientStatus::AssociationFailed;
  }

  dlms::apdu::DlmsData data = {};
  const dlms::apdu::ApduStatus status =
    dlms::apdu::DecodeDlmsData(&encoded[0], encoded.size(), 8u, data);
  if (status != dlms::apdu::ApduStatus::Ok ||
      data.type != dlms::apdu::DlmsDataType::OctetString ||
      (data.bytes.data == 0 && data.bytes.size != 0u)) {
    return ClientStatus::AssociationFailed;
  }

  bytes.assign(data.bytes.data, data.bytes.data + data.bytes.size);
  return ClientStatus::Ok;
}

} // namespace

DlmsClient::DlmsClient(const DlmsClientOptions& options)
  : ownedStream_(CreateTcpStream(options))
  , ownedChannel_(CreateProfileChannel(*ownedStream_, options))
  , ownedSecurityContext_(CreateSecurityContext(options))
  , ownedKeys_(CreateKeyStore(options))
  , ownedCounters_(CreateCounterStore(options))
  , ownedRandom_(CreateRandomSource(options))
  , ownedHlsHigh_(
      CreateHlsHighAuthenticator(
        options,
        ownedRandom_.get()))
  , ownedHlsGmac_(
      CreateHlsAuthenticator(
        options,
        ownedSecurityContext_.get(),
        ownedKeys_.get(),
        ownedCounters_.get(),
        ownedRandom_.get()))
  , ownedHlsStrategy_(
      CreateHlsStrategy(
        options,
        ownedHlsHigh_.get(),
        ownedHlsGmac_.get()))
  , ownedAssociation_(
      CreateAssociation(
        *ownedChannel_,
        options,
        ownedHlsStrategy_.get()))
  , ownedSecurity_()
  , channel_(*ownedChannel_)
  , association_(*ownedAssociation_)
  , xdlms_()
  , state_(ClientState::Disconnected)
  , constructionStatus_(ValidateDlmsClientOptions(options))
  , hlsAuthentication_(
      options.authenticationMode ==
        ClientAuthenticationMode::HighLevelSecurity ||
      options.authenticationMode ==
        ClientAuthenticationMode::HighLevelSecurityGmac)
{
  if (constructionStatus_ != ClientStatus::Ok ||
      options.securityMode == ClientSecurityMode::None) {
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
  , ownedSecurityContext_()
  , ownedKeys_()
  , ownedCounters_()
  , ownedRandom_()
  , ownedHlsHigh_()
  , ownedHlsGmac_()
  , ownedHlsStrategy_()
  , ownedAssociation_()
  , ownedSecurity_()
  , channel_(channel)
  , association_(association)
  , xdlms_(new dlms::xdlms::XdlmsClient(channel, association))
  , state_(ClientState::Disconnected)
  , constructionStatus_(ClientStatus::Ok)
  , hlsAuthentication_(false)
{
}

DlmsClient::DlmsClient(
  dlms::profile::IApduChannel& channel,
  dlms::association::AssociationClient& association,
  dlms::security::CipheredApduProcessor& security)
  : ownedStream_()
  , ownedChannel_()
  , ownedSecurityContext_()
  , ownedKeys_()
  , ownedCounters_()
  , ownedRandom_()
  , ownedHlsHigh_()
  , ownedHlsGmac_()
  , ownedHlsStrategy_()
  , ownedAssociation_()
  , ownedSecurity_()
  , channel_(channel)
  , association_(association)
  , xdlms_(new dlms::xdlms::XdlmsClient(channel, association, security))
  , state_(ClientState::Disconnected)
  , constructionStatus_(ClientStatus::Ok)
  , hlsAuthentication_(false)
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

  dlms::profile::HdlcProfileChannel* hdlc =
    dynamic_cast<dlms::profile::HdlcProfileChannel*>(ownedChannel_.get());
  if (hdlc != 0) {
    const dlms::profile::ProfileStatus linkStatus = hdlc->ConnectDataLink();
    if (linkStatus != dlms::profile::ProfileStatus::Ok) {
      association_.Close();
      return ClientStatus::ChannelOpenFailed;
    }
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

  if (hlsAuthentication_) {
    if (ownedHlsStrategy_.get() == 0) {
      return ClientStatus::InternalError;
    }

    const std::vector<std::uint8_t>& serverChallenge =
      association_.Result().highLevelSecurityServerChallenge;
    if (serverChallenge.empty() ||
        ownedHlsStrategy_->ClientChallenge().empty()) {
      return ClientStatus::AssociationFailed;
    }

    if (ownedHlsGmac_.get() != 0 &&
        !ApplyDiscoveredRemoteSystemTitle(
          ownedSecurityContext_.get(),
          association_.Result())) {
      return ClientStatus::AssociationFailed;
    }

    std::vector<std::uint8_t> response;
    if (ownedHlsStrategy_->BuildResponse(
          SecurityView(serverChallenge),
          response) !=
        dlms::security::SecurityStatus::Ok) {
      return ClientStatus::SecurityFailed;
    }

    std::vector<std::uint8_t> encodedParameter;
    ClientStatus encodeStatus =
      EncodeOctetStringData(response, encodedParameter);
    if (encodeStatus != ClientStatus::Ok) {
      return encodeStatus;
    }

    std::unique_ptr<dlms::xdlms::XdlmsClient> plainHlsClient;
    dlms::xdlms::XdlmsClient* hlsClient = xdlms_.get();
    if (ownedSecurity_.get() != 0) {
      plainHlsClient.reset(
        new dlms::xdlms::XdlmsClient(channel_, association_));
      hlsClient = plainHlsClient.get();
    }

    dlms::xdlms::ActionResult actionResult =
      dlms::xdlms::EmptyActionResult();
    const ClientStatus actionStatus =
      MapXdlmsStatus(
        hlsClient->Action(
          HlsReplyMethod(),
          true,
          encodedParameter,
          actionResult));
    if (actionStatus != ClientStatus::Ok) {
      return actionStatus;
    }
    if (!actionResult.hasData) {
      return ClientStatus::AssociationFailed;
    }

    std::vector<std::uint8_t> serverResponse;
    ClientStatus decodeStatus =
      DecodeOctetStringData(actionResult.data, serverResponse);
    if (decodeStatus != ClientStatus::Ok) {
      return decodeStatus;
    }

    if (ownedHlsStrategy_->VerifyResponse(
          SecurityView(ownedHlsStrategy_->ClientChallenge()),
          SecurityView(serverResponse)) != dlms::security::SecurityStatus::Ok) {
      return ClientStatus::SecurityFailed;
    }
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
