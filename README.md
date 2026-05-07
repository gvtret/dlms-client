# dlms-client

`dlms-client` is the public client facade layer for the DLMS/COSEM framework.

It will compose lower repositories into an application-facing synchronous API:

- `dlms-transport` for TCP, UDP, serial, and future transport construction;
- `dlms-profile` for Wrapper and HDLC APDU channels;
- `dlms-association` for application association lifecycle;
- `dlms-xdlms` for GET, SET, and ACTION service primitives.

The facade must not implement protocol codecs, association negotiation rules,
COSEM object storage, or xDLMS APDU semantics. Those responsibilities stay in
their dedicated layer repositories.

## Documentation

- [Requirements](docs/00_client_requirements.md)
- [API](docs/01_client_api.md)
- [Architecture](docs/02_client_architecture.md)
- [Test Plan](docs/03_client_test_plan.md)
- [Implementation Plan](docs/04_client_implementation_plan.md)
