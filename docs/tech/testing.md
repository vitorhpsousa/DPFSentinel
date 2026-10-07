# Testing

**This repository (`obd-esp32`) has no automated tests of any kind today** —
no `test`-named files and no CI workflow anywhere in this tree. There is no
firmware test suite for the three ESP32-S3 targets (root build,
`boards/cyd-s3-3p5/firmware/`, `boards/waveshare-s3-touch-lcd-2/firmware/`).

A future C++ suite would need a framework decision first (for example
PlatformIO's `pio test` with Unity) — see the open question in
[../contributing.md](../contributing.md#tests). Candidates to cover: ISO-TP
frame reassembly (`src/obd/isotp.cpp`), every PID decoder
(`src/obd/pid_decode.cpp`) against payloads taken from real `raw_N.log`
captures, and replaying a captured session through the regen state machine
(`src/report/regen_watch.cpp`).

The separate, still-private Pi project has its own pytest suite (221 tests
as last counted) covering its Python code. It is a different codebase and
is not a substitute for tests in this repository; see
[cross-project-notes.md](../cross-project-notes.md#testing-the-pi-projects-pytest-suite)
for what it covers.
