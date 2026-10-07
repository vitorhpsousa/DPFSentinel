# Protocol and PIDs (technical reference)

This repository's own protocol handling and PID table are documented in
[../architecture.md](../architecture.md) (the ELM327 conversation, ISO-TP
reassembly and adapter init, as implemented in `src/obd/elm_client.cpp` +
`src/obd/isotp.cpp`, duplicated per target) and
[../pid-map.md](../pid-map.md) (the full per-column request, offset, scale
and verification status, implemented in `src/obd/pid_registry.h` +
`pid_decode.cpp`).

This page used to describe the separate, still-private Pi project's parallel
Python implementation (`obd/pid_registry.py`, `obd/elm_client.py`,
`obd/isotp.py`) in detail, including its own copy of the PID table. That
table was never kept in sync with [pid-map.md](../pid-map.md) and should not
be treated as a second source of truth; see
[cross-project-notes.md](../cross-project-notes.md#protocol-and-pids-the-pis-python-implementation)
for what was carried over from it.
