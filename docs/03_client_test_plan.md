# dlms-client Test Plan

## 1. Unit Tests

Status and options:

- status names are stable;
- default options select Wrapper/TCP, no security, public client SAP, logical
  name association context;
- invalid options reject empty host, zero port, and unsupported profile/security
  combinations.

Lifecycle:

- new client starts disconnected;
- `Connect()` opens transport and APDU channel;
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

## 3. Manual Acceptance

Manual tests may use a real Wrapper endpoint with:

- client 16, no security;
- client 32, LLS;
- client 48, HLS.

Only client 16 is in the no-security MVP. LLS/HLS stay pending until
`dlms-security` is implemented.
