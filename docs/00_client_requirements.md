# dlms-client Requirements

## 1. Scope

`dlms-client` provides an ergonomic public client facade for applications that
need to communicate with DLMS/COSEM servers.

The layer coordinates existing lower layers. It does not reimplement their
protocol logic.

In scope:

- client options and endpoint configuration;
- client lifecycle: connect, open association, release association, close;
- high-level GET, SET, ACTION forwarding;
- profile selection for Wrapper/TCP and HDLC/TCP;
- association authentication selection for no-authentication and LLS;
- security selection for no-security and ciphered APDU operation;
- simple synchronous API;
- status-code error reporting.

Out of scope:

- HDLC/LLC implementation details;
- Wrapper frame codec implementation;
- ACSE and xDLMS APDU encoding details;
- COSEM object model and server dispatch;
- cryptographic primitive implementation;
- persistent credential storage;
- asynchronous scheduler ownership in the first implementation.

## 2. Layer Boundary

`dlms-client` depends downward on:

- `dlms-transport`;
- `dlms-profile`;
- `dlms-association`;
- `dlms-xdlms`;
- `dlms-security`;
- `dlms-apdu` only for public `DlmsData` conversion helpers when needed.

No lower layer may include `dlms-client` headers.

## 3. MVP Requirements

The MVP shall:

- expose `DlmsClientOptions`;
- expose `DlmsClient`;
- support Wrapper/TCP no-security LN association;
- support Wrapper/TCP LLS LN association;
- support Wrapper/TCP ciphered APDU operation after association setup;
- support HDLC/TCP LN association using the existing HDLC/LLC profile channel;
- support public client SAP and configurable server SAP;
- support GET using `CosemAttributeDescriptor`;
- support SET using encoded DLMS `Data` bytes;
- support ACTION with optional encoded DLMS `Data` parameter;
- return encoded DLMS `Data` bytes for GET and ACTION return parameters;
- keep all runtime failures as `ClientStatus` values;
- avoid throwing exceptions from public runtime API paths.

## 4. Security Requirements

The facade shall configure security material explicitly from caller-provided
options. It shall not persist keys, derive keys, or hide key provisioning
behind global state.

Rules:

- `ClientSecurityMode::None` keeps the current unprotected behavior;
- `ClientSecurityMode::AuthenticatedAndEncrypted` configures
  `dlms-security` suite 0 with global unicast encryption and authentication
  keys;
- the caller provides client and server system titles;
- the caller provides the first local invocation counter value;
- the facade owns the in-memory key store and invocation counter store for an
  options-constructed client;
- injected-channel clients may receive an externally composed security
  processor for deterministic tests or advanced composition;
- no-security association authentication remains separate from APDU ciphering.
- LLS credentials are passed exactly as supplied to `dlms-association`; the
  facade shall not hash, derive, persist, or otherwise transform passwords.

Document RAG alignment:

- the service APDU is built first, then ciphered according to the active
  security context;
- incoming ciphered APDUs are deciphered before invoking the xDLMS service
  primitive;
- invocation counters are part of the protected APDU security header and must
  increase monotonically per key context.

## 5. Non-Goals For MVP

- HLS authentication;
- block transfer orchestration;
- event notifications;
- SN referencing;
- automatic object model discovery beyond explicit GET calls;
- retry policy.

## 6. Success Criteria

- Applications can create a client from options.
- The client can connect to a Wrapper/TCP APDU channel.
- The client can connect to an HDLC/TCP APDU channel and establish the HDLC
  data link before opening the application association.
- The client can open a no-security LN association.
- The client can call GET/SET/ACTION through `dlms-xdlms`.
- The client can configure suite 0 authenticated-encrypted APDU protection and
  pass protected GET/SET/ACTION traffic through `dlms-xdlms`.
- Unit tests cover lifecycle state transitions and status mapping.
- Root integration test proves a public-client GET against a minimal in-memory
  server path once the server-side profile loop is available.
