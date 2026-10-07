# Alerts internals

This repository's alert state machine and Telegram sender are
`src/report/regen_watch.cpp` and `src/report/telegram_report.cpp`, duplicated
per ESP32-S3 target (repo root and both `boards/*/firmware/src/report/`).
User-facing setup and the behaviour that matters day to day (what triggers a
message, queueing and retry limits, the test endpoints) is documented in
[../telegram-alerts.md](../telegram-alerts.md) — that is the page to read for
this repository's own firmware.

The separate, still-private Pi project has its own Python alerting code
(`alerts/regen_watch.py`, `alerts/telegram_alerts.py`) that this page used to
document in detail. It was a port of this repository's `regen_watch.cpp`
with matching constants, but it is a different codebase with its own queueing
and retry behaviour. See
[cross-project-notes.md](../cross-project-notes.md#alerts-internals-pi) for
what was carried over from it.
