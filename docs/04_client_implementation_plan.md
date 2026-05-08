# dlms-client Implementation Plan

## Phase 0. Documentation

Deliverables:

- requirements;
- public API contract;
- architecture diagrams;
- state-machine diagram;
- test plan;
- implementation plan;
- minimal CMake interface target.

Commit message:

```text
docs(client): define public client facade
```

## Phase 1. Status And Options

Deliverables:

- `ClientStatus`;
- status name helper;
- `DlmsClientOptions`;
- option validation;
- default options helper;
- unit tests.

Commit message:

```text
feat(client): add facade status and options
```

## Phase 2. Injected-Channel Facade

Deliverables:

- `DlmsClient` over externally supplied `IApduChannel` and
  `AssociationClient`;
- lifecycle state machine;
- GET/SET/ACTION forwarding to `XdlmsClient`;
- fake-channel tests.

Commit message:

```text
feat(client): add injected channel facade
```

## Phase 3. Wrapper/TCP Factory

Deliverables:

- Wrapper/TCP transport/profile construction from `DlmsClientOptions`;
- ownership model for transport, profile channel, association, and xDLMS client;
- lifecycle tests with fakes or loopback.

Commit message:

```text
feat(client): compose wrapper tcp client
```

## Phase 4. Root Integration

Deliverables:

- root submodule entry;
- root CMake wiring;
- root integration test for public-client GET against the minimal server path;
- full root build and test run.

Commit message:

```text
test: cover public client get integration
```

## Phase 5. SET/ACTION Integration

Deliverables:

- root integration tests for public-client SET and ACTION;
- error-path tests for service rejection.

Commit message:

```text
test: cover public client set action integration
```

## Phase 6. Security Options Documentation

Deliverables:

- public security option requirements;
- API contract for `ClientSecurityMode::AuthenticatedAndEncrypted`;
- architecture sequence for client/security/xDLMS composition;
- status mapping and test plan for security failures.

Commit message:

```text
docs(client): define security options
```

## Phase 7. Injected Security Facade

Deliverables:

- injected constructor accepting `CipheredApduProcessor`;
- `ClientStatus::SecurityFailed`;
- xDLMS security failure mapping;
- focused tests for protected GET and authentication failure using injected
  channel composition.

Commit message:

```text
feat(client): add injected security facade
```

## Phase 8. Options-Owned Security Composition

Deliverables:

- `ClientSecurityOptions`;
- validation for protected mode;
- options constructor ownership of key store, counter store, security context,
  and ciphered APDU processor;
- tests proving options-owned protected client construction.

Commit message:

```text
feat(client): compose security options
```
