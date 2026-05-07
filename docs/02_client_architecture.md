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
  ChannelFactory["ProfileChannelFactory"]
  TransportFactory["TransportFactory"]
  Association["AssociationClient"]
  Services["XdlmsClient"]

  Options --> Client
  Client --> TransportFactory
  Client --> ChannelFactory
  Client --> Association
  Client --> Services
  ChannelFactory --> TransportFactory
```

## 5. Class Interaction Diagram

```mermaid
classDiagram
  class DlmsClient {
    -DlmsClientOptions options
    -ClientState state
    -IByteStream* byteStream
    -IApduChannel* channel
    -AssociationClient* association
    -XdlmsClient* xdlms
    +Connect() ClientStatus
    +OpenAssociation() ClientStatus
    +ReleaseAssociation() ClientStatus
    +Close() ClientStatus
    +Get(descriptor, data) ClientStatus
    +Set(descriptor, data) ClientStatus
    +Action(descriptor, hasParameter, parameter, returnData) ClientStatus
  }

  class DlmsClientOptions
  class ClientState {
    <<enumeration>>
    Disconnected
    Connected
    Associated
  }

  class TransportFactory {
    +CreateWrapperTcp(options)
  }

  class ProfileChannelFactory {
    +CreateWrapperTcpChannel(stream, options)
  }

  class AssociationClient
  class XdlmsClient

  DlmsClient --> DlmsClientOptions
  DlmsClient --> ClientState
  DlmsClient --> TransportFactory
  DlmsClient --> ProfileChannelFactory
  DlmsClient --> AssociationClient
  DlmsClient --> XdlmsClient
```

## 6. State Machine

```mermaid
stateDiagram-v2
  [*] --> Disconnected
  Disconnected --> Connected: Connect ok
  Connected --> Associated: OpenAssociation ok
  Associated --> Connected: ReleaseAssociation ok
  Connected --> Disconnected: Close
  Associated --> Disconnected: Close
```

## 7. Error Model

Public runtime calls return `ClientStatus`. Constructors only store options and
must not open transports. Destructors may close owned resources best-effort but
must not be the only way to release an association.

## 8. Test Strategy

The first implementation uses fake lower-layer ports for deterministic unit
tests. Real loopback and meter-facing tests belong in root integration or manual
acceptance suites after the facade behavior is stable.
