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
- profile selection for Wrapper/TCP in the MVP;
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
- `dlms-apdu` only for public `DlmsData` conversion helpers when needed.

No lower layer may include `dlms-client` headers.

## 3. MVP Requirements

The MVP shall:

- expose `DlmsClientOptions`;
- expose `DlmsClient`;
- support Wrapper/TCP no-security LN association;
- support public client SAP and configurable server SAP;
- support GET using `CosemAttributeDescriptor`;
- support SET using encoded DLMS `Data` bytes;
- support ACTION with optional encoded DLMS `Data` parameter;
- return encoded DLMS `Data` bytes for GET and ACTION return parameters;
- keep all runtime failures as `ClientStatus` values;
- avoid throwing exceptions from public runtime API paths.

## 4. Non-Goals For MVP

- LLS and HLS authentication;
- ciphered APDUs;
- block transfer orchestration;
- event notifications;
- SN referencing;
- automatic object model discovery beyond explicit GET calls;
- retry policy.

## 5. Success Criteria

- Applications can create a client from options.
- The client can connect to a Wrapper/TCP APDU channel.
- The client can open a no-security LN association.
- The client can call GET/SET/ACTION through `dlms-xdlms`.
- Unit tests cover lifecycle state transitions and status mapping.
- Root integration test proves a public-client GET against a minimal in-memory
  server path once the server-side profile loop is available.
