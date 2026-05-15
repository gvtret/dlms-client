# dlms-client API

## 1. Status Model

`ClientStatus` is the public status enum for the facade.

Initial values:

```cpp
enum class ClientStatus
{
  Ok,
  InvalidArgument,
  InvalidState,
  TransportOpenFailed,
  ChannelOpenFailed,
  AssociationFailed,
  NotAssociated,
  SendFailed,
  ReceiveFailed,
  ServiceRejected,
  SecurityFailed,
  UnsupportedFeature,
  InternalError
};
```

The implementation maps lower-layer statuses into `ClientStatus`; applications
should not need to include lower-layer status enums for basic client usage.

## 2. Options

```cpp
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
  std::uint16_t clientSap;
  std::uint16_t serverSap;
  std::uint32_t connectTimeoutMs;
  std::uint32_t requestTimeoutMs;
};
```

## 3. Data Types

The facade uses xDLMS descriptors directly to avoid duplicating protocol
identity types:

```cpp
using CosemAttributeDescriptor = dlms::xdlms::CosemAttributeDescriptor;
using CosemMethodDescriptor = dlms::xdlms::CosemMethodDescriptor;
```

GET, SET, and ACTION values cross the facade boundary as complete encoded DLMS
`Data` bytes, including the type tag. Typed convenience helpers may be added
later, but they must remain thin wrappers over this encoded-data API.

## 4. Client Class

```cpp
class DlmsClient
{
public:
  explicit DlmsClient(const DlmsClientOptions& options);

  DlmsClient(
    dlms::profile::IApduChannel& channel,
    dlms::association::AssociationClient& association);

  DlmsClient(
    dlms::profile::IApduChannel& channel,
    dlms::association::AssociationClient& association,
    dlms::security::CipheredApduProcessor& security);

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
};
```

## 5. Lifecycle Rules

- The options constructor owns a TCP byte stream, selected APDU channel,
  association client, xDLMS service client, and optional security
  stores/processor.
- For `ClientProfile::WrapperTcp`, `Connect()` opens the Wrapper/TCP APDU
  channel.
- For `ClientProfile::HdlcTcp`, `Connect()` opens the TCP-backed HDLC profile
  channel and establishes the HDLC data link before returning `Ok`.
- The injected constructor receives an already constructed APDU channel and
  association client for deterministic tests or external composition.
- The injected security constructor also receives an already constructed
  `CipheredApduProcessor`.
- `Connect()` opens the APDU channel through `AssociationClient`.
- `OpenAssociation()` requires a connected channel.
- `Get()`, `Set()`, and `Action()` require an established association.
- `ReleaseAssociation()` is idempotent when already not associated. In the
  injected-channel phase a successful release closes the lower channel through
  `AssociationClient::Release()` and returns the facade to disconnected state.
- `Close()` closes the channel and returns the client to the disconnected state.

## 6. Error Mapping

| Lower layer | Facade mapping |
|---|---|
| transport open failure | `TransportOpenFailed` |
| profile open failure | `ChannelOpenFailed` |
| association open/establish failure | `AssociationFailed` |
| xDLMS `NotAssociated` | `NotAssociated` |
| xDLMS send failure | `SendFailed` |
| xDLMS receive failure | `ReceiveFailed` |
| xDLMS service rejection | `ServiceRejected` |
| xDLMS security failure | `SecurityFailed` |
| unsupported lower-layer feature | `UnsupportedFeature` |

## 7. Security Option Rules

`ClientAuthenticationMode::None` keeps the default lowest-level association
authentication behavior.

`ClientAuthenticationMode::LowLevelSecurity` requires a non-null, non-empty
credential. The options-owned constructor copies those bytes into
`dlms-association` and does not transform them.

`ClientAuthenticationMode::HighLevelSecurity` requires a non-null, non-empty
password. The options-owned constructor wires a password-based HLS High
strategy into `dlms-association`. `OpenAssociation()` completes AARQ/AARE,
invokes Association LN `reply_to_HLS_authentication`, and verifies the server
response before reporting the facade as associated. This mode corresponds to
COSEM HLS mechanism id `2` and to certification profiles where `bGMAC=false`
and `HLSPassword=HiPassword`.

`ClientAuthenticationMode::HighLevelSecurityGmac` requires a valid client
system title and a 16-byte authentication key in `ClientSecurityOptions`. The
server system title may be supplied explicitly or discovered from an 8-byte
AARE responding AP title. The options-owned constructor wires an HLS GMAC
strategy into `dlms-association`. `OpenAssociation()` completes the AARQ/AARE
exchange, resolves the remote system title, invokes Association LN
`reply_to_HLS_authentication` through xDLMS ACTION, and verifies the server
response before reporting the facade as associated.

`ClientSecurityMode::None` ignores `ClientSecurityOptions`.

`ClientSecurityMode::AuthenticatedAndEncrypted` requires:

- valid non-zero client and server system titles;
- 16-byte global unicast encryption key;
- 16-byte authentication key;
- a configured invocation counter start value.

The facade maps these fields into a `dlms::security::SecurityContext`,
`InMemoryKeyStore`, `InMemoryInvocationCounterStore`, and
`CipheredApduProcessor`. It does not own persistent key storage and does not
derive LLS/HLS authentication material.
