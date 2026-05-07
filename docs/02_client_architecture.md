# dlms-client Architecture

## 1. Scope

`dlms-client` is a facade. It owns application ergonomics and lifecycle
composition, not protocol semantics.

## 2. Dependencies

```mermaid
flowchart TB
  App["Application"]
  Client["dlms-client<br/>public facade"]
  XDlms["dlms-xdlms<br/>GET/SET/ACTION services"]
  Assoc["dlms-association<br/>AA state machine"]
  Profile["dlms-profile<br/>APDU channel"]
  Transport["dlms-transport<br/>TCP/UDP/serial"]
  Apdu["dlms-apdu<br/>DLMS Data helpers"]

  App --> Client
  Client --> XDlms
  Client --> Assoc
  Client --> Profile
  Client --> Transport
  Client --> Apdu
  XDlms --> Assoc
  XDlms --> Profile
```

## 3. Public Modules

- `client_status`: facade status enum and string names.
- `client_options`: profile, endpoint, SAP, timeout, and security options.
- `dlms_client`: lifecycle and GET/SET/ACTION facade.
- `client_data`: optional encoded DLMS `Data` helper functions.

## 4. Layer Diagram

```mermaid
flowchart LR
  Options["DlmsClientOptions"]
  Client["DlmsClient"]
  Stream["TcpStreamTransport"]
  Channel["IApduChannel"]
  Association["AssociationClient"]
  Services["XdlmsClient"]

  Options --> Client
  Client --> Stream
  Stream --> Channel
  Channel --> Client
  Association --> Client
  Client --> Association
  Client --> Services
  Services --> Channel
  Services --> Association
```

## 5. Class Interaction Diagram

```mermaid
classDiagram
  class DlmsClient {
    -ClientState state
    -TcpStreamTransport ownedStream
    -WrapperTcpProfileChannel ownedChannel
    -AssociationClient ownedAssociation
    -AssociationClient& association
    -XdlmsClient xdlms
    +DlmsClient(options)
    +DlmsClient(IApduChannel, AssociationClient)
    +Connect() ClientStatus
    +OpenAssociation() ClientStatus
    +ReleaseAssociation() ClientStatus
    +Close() ClientStatus
    +State() ClientState
    +Get(descriptor, data) ClientStatus
    +Set(descriptor, data) ClientStatus
    +Action(descriptor, hasParameter, parameter, returnData) ClientStatus
  }

  class ClientState {
    <<enumeration>>
    Disconnected
    Connected
    Associated
  }

  class DlmsClientOptions
  class TcpStreamTransport
  class WrapperTcpProfileChannel
  class IApduChannel
  class AssociationClient
  class XdlmsClient

  DlmsClient --> DlmsClientOptions
  DlmsClient --> TcpStreamTransport
  DlmsClient --> WrapperTcpProfileChannel
  DlmsClient --> ClientState
  DlmsClient --> IApduChannel
  DlmsClient --> AssociationClient
  DlmsClient --> XdlmsClient
```

## 6. State Machine

```mermaid
stateDiagram-v2
  [*] --> Disconnected
  Disconnected --> Connected: Connect ok
  Connected --> Associated: OpenAssociation ok
  Associated --> Disconnected: ReleaseAssociation ok
  Connected --> Disconnected: Close
  Associated --> Disconnected: Close
```

## 7. Error Model

Public runtime calls return `ClientStatus`. Invalid constructor options are
stored and returned by `Connect()` because constructors do not throw. The
options-owned Wrapper/TCP path uses `AssociationClient::Release()`, which sends
RLRQ, receives RLRE, closes the channel, and returns the facade to
`Disconnected`.

## 8. Test Strategy

The first implementation uses fake lower-layer ports for deterministic unit
tests. Real loopback and meter-facing tests belong in root integration or manual
acceptance suites after the facade behavior is stable.
