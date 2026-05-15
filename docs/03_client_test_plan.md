# dlms-client Test Plan

## 1. Unit Tests

Status and options:

- status names are stable;
- default options select Wrapper/TCP, no security, public client SAP, logical
  name association context;
- default options select no association authentication;
- LLS options require a non-null non-empty credential;
- oversized LLS credentials are rejected before construction work is used;
- HLS GMAC options require a valid client system title and authentication key
  material; the server system title may be explicit or discovered from AARE;
- invalid options reject empty host, zero port, and unsupported profile/security
  combinations.
- security options validate system titles and key sizes for protected mode.
- HDLC/TCP options validate host, TCP port, HDLC addresses, information field
  sizes, window sizes, retry count, and retry delay.

Lifecycle:

- new client starts disconnected;
- `Connect()` opens transport and APDU channel;
- `Connect()` for HDLC/TCP also performs data-link connect before reporting
  connected state;
- `OpenAssociation()` requires connected state;
- successful association moves to associated state;
- `ReleaseAssociation()` returns to disconnected state because the current
  association client closes the APDU channel after RLRE;
- `Close()` returns to disconnected state from connected or associated state;
- repeated close is idempotent.

Service forwarding:

- `Get()` requires associated state;
- `Get()` forwards descriptor to `dlms-xdlms`;
- `Get()` copies encoded data on success;
- `Set()` forwards descriptor and encoded data;
- `Action()` forwards method descriptor and optional parameter;
- lower-layer service rejection maps to `ClientStatus::ServiceRejected`.
- lower-layer security failure maps to `ClientStatus::SecurityFailed`.

Security composition:

- default options keep `ClientSecurityMode::None`;
- protected options construct key and invocation counter stores;
- injected security constructor forwards protected GET through `XdlmsClient`;
- authenticated response failure maps to `SecurityFailed`;
- no-security service tests remain unchanged.

Association authentication composition:

- options-owned client maps LLS credential bytes into the AARQ;
- options-owned client maps HLS GMAC into an association strategy;
- HLS GMAC `OpenAssociation()` invokes Association LN method 1 after AARE;
- HLS GMAC copies an 8-byte AARE responding AP title into the remote system
  title when no explicit server system title was configured;
- HLS GMAC server response verification failure maps to `SecurityFailed`;
- no-authentication client AARQ remains unchanged.

Failure mapping:

- transport open failure;
- channel open failure;
- association failure;
- send failure;
- receive failure;
- unsupported feature.

## 2. Integration Tests

Root integration should cover:

- public client GET over fake Wrapper/TCP channel once a matching server loop is
  available;
- public client SET over the same path;
- public client ACTION over the same path;
- future live-meter smoke test against configurable endpoint, disabled by
  default.
- opt-in HDLC/TCP live-meter smoke mode against configurable endpoint.
- client SAP 32 LLS live smoke with environment-provided password;
- protected public client GET over a fake Wrapper/TCP channel once the matching
  server security composition is available.

## 3. Manual Acceptance

Manual tests may use a real Wrapper endpoint with:

- client 16, no security;
- client 32, LLS password;
- client 48, HLS GMAC when the target meter supports that mechanism.

Client 16 remains the first live-meter smoke path. Client 32 is enabled by LLS
association options. Client 48 requires HLS challenge-response work before it
becomes an automated acceptance test. Proprietary HLS high-password mechanisms
remain a separate phase from HLS GMAC.
