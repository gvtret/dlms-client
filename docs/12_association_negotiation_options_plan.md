# Association Negotiation Options Plan

## Goal

Expose association negotiation knobs that already exist in `dlms-association`
through `DlmsClientOptions`.

The immediate offline driver is the pilot configuration from
`E:\work\pyDlmsCertification\configs\pilot.json`: WRAPPER legacy mode uses
`PduSizeLLS=2048` and `PduSizeHLS=2048`, while `dlms-client` currently always
uses the `dlms-association` default client max receive PDU size (`0x0200`).

## Requirements

- Existing callers continue to compile without source changes.
- `DefaultDlmsClientOptions()` keeps the current effective default by copying
  the association default max PDU size.
- `ValidateDlmsClientOptions()` rejects zero client max receive PDU size.
- `MakeAssociationOptions()` forwards the configured value into
  `AssociationOptions::clientMaxReceivePduSize`.
- The live smoke can override this value with an environment variable.
- Conformance customization remains out of scope for this phase.

## API Sketch

```cpp
struct DlmsClientOptions
{
  ...
  std::uint16_t associationClientMaxReceivePduSize;
};
```

## Architecture

```mermaid
flowchart LR
  App["Application / live smoke"]
  ClientOptions["DlmsClientOptions"]
  Client["DlmsClient"]
  AssocOptions["AssociationOptions"]
  Assoc["AssociationClient"]
  Aarq["AARQ InitiateRequest"]

  App --> ClientOptions
  ClientOptions --> Client
  Client --> AssocOptions
  AssocOptions --> Assoc
  Assoc --> Aarq
```

## Test Plan

- default client options expose `0x0200`;
- validation rejects zero;
- options-owned client forwards the configured value into AARQ metadata through
  the association trace sink;
- existing client tests continue to pass.

## Phase Commit Message

```text
docs(client): define association negotiation options

Document client-level configuration for association client max receive PDU
size. The plan keeps the existing default, forwards the value into
AssociationOptions, and prepares live smoke parity with the pilot
pyDlmsCertification configuration.

Verification: documentation-only phase.
```
