# Data formats

This repository's own CSV, raw-log and HTTP API formats are documented where
they are produced, not duplicated here:

- `session_N.csv` and `raw_N.log` column lists and write behaviour for the
  three ESP32-S3 targets: [../architecture.md](../architecture.md#files).
- The built-in web UI's HTTP endpoints, including the real `/api/live`
  shape (`{"t", "link", "session", ..., "values": {...}}` — no
  `has_data`/`stale`/`latest` wrapper): [../adapting/other-apps.md](../adapting/other-apps.md).

The separate, still-private Pi project uses a different CSV column set and a
different HTTP API shape (`has_data`/`stale`/`latest`-wrapped `/api/live`,
plus `/api/schema`, `/api/sessions`), from `storage/session_store.py` and
`webui/server.py`. This page used to document those in detail; see
[cross-project-notes.md](../cross-project-notes.md#data-formats-pi-csv-raw-log-and-http-api)
for what was carried over from that review. Do not mix the two shapes up —
a value template or importer written for one will not work against the
other.
