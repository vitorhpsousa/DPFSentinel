# Testing

The suite lives in `_repo_prep/tests_draft/` (intended to become `tests/` next to `pi/`, per `_repo_prep/README.md`). It tests the Pi Python code only; there are no firmware tests in the repo. Labels: [code] read, [ran] executed for this page, [unverified].

## Result of a run

[ran, 2026-09-23] Copied `tests_draft/` to a scratch `tests/` beside symlinks to `pi/` and `tools/`, fresh venv with `pytest pillow requests` (no pyserial): `221 passed, 1 xfailed` in about 2.5 s. `_repo_prep/README.md` quotes "177 pass", which is out of date ( the suite has grown, for example with the BLE and alert tests).

## Running

```bash
python3 -m venv venv && venv/bin/pip install pytest pillow requests
venv/bin/pytest tests -q
```

`tests/conftest.py` puts `pi/` and `tools/` on `sys.path` (so `import config`, `import obd.isotp` work as on the Pi; override the root with `OBD_REPO_ROOT`), sets `sys.dont_write_bytecode`, and installs a stub `serial` module whose `Serial` raises `SerialException`. So pyserial and hardware are not needed. It also provides a `frames` fixture that builds ELM ATH1/ATS0-style reply text (single frame up to 7 bytes, otherwise first + consecutive frames) from a payload [code].

`test_soot_panel.py` uses `pytest.importorskip` for Pillow and requests; `test_dpf_and_tools.py` skips the `carscanner_parse` tests if that tool cannot be imported.

## Layout

| File | Covers |
|---|---|
| `conftest.py` | path setup, `serial` stub, `frames` fixture |
| `payloads.py` | synthetic payload builders (`payload_2103`, `payload_soot`, `payload_eps`, `std`) constructed from the documented byte offsets, not copied from real logs [code docstring] |
| `fake_elm.py` | scripted stand-in for `ElmClient` (`healthy_replies`, `FakeElm`, a shared "world" so reconnects follow one script) |
| `test_isotp.py` | single/multi-frame reassembly, echo lines, wrong ids, short frames |
| `test_pid_registry.py` | every decoder against hand-built payloads, prefix/length handling, table shape |
| `test_elm_client.py` | `send_command`, init sequence, header caching |
| `test_ble_transport.py` | name matching, `pick_characteristics` (NUS, FFE0-style, generic services skipped), `resolve_adapter` against a fake sysfs tree under `tmp_path`, byte buffering and errors; no radio needed |
| `test_main.py` | `poll_cycle` (shared requests, derived odometer, wrong prefix, header failure), the reconnect loop (blank row per failure, one warning, 5 s delay, 10-blank re-init, raw-log thinning), `connect_adapter` |
| `test_session_store.py` | file numbering, CSV/raw/calibration output, one strict `xfail` documenting the `begin()` `makedirs` bug |
| `test_webui.py` | record conversion, downsampling, idle flag, path-traversal regex, live/summary/schema builders |
| `test_alerts.py` | `RegenWatch` and `TelegramSender` (see [alerts-internals.md](alerts-internals.md)) |
| `test_soot_panel.py` | colour bands and gradient, image size/mode, static stale frames, RGB565 encoding, `_fmt` |
| `test_dpf_and_tools.py` | idle-blockage rule, `carscanner_parse.reassemble` |
| `ci.yml` | GitHub Actions draft (below) |

Approximate test-function counts per file from `grep`: alerts 11, ble 26, dpf/tools 4, elm 7, isotp 14, main 21, pid_registry 17, session_store 9, soot_panel 15, webui 10 (parametrisation expands these to 221 cases).

## Conventions when adding tests

- Build payloads with `payloads.py` or `frames`, and assert against values computed by hand from the documented formula (see [protocol-and-pids.md](protocol-and-pids.md)); do not import the decoder to compute the expectation.
- Time and network are injected: `RegenWatch(clock_ok=...)` and explicit `now_ms`; `TelegramSender(post=..., sleep=...)`. Never hit the real Telegram API or sleep in tests.
- Anything touching `LOG_DIR` must use `tmp_path` plus `monkeypatch.setattr(config, "LOG_DIR", ...)` (as `test_webui.py` does).
- A known bug goes in as `xfail(strict=True)` with the reason, so fixing it turns the test red until the marker is removed.
- New PID: add a decoder case with a payload taken from a reference capture, not invented, and update [../pid-map.md](../pid-map.md).

## CI draft (`tests_draft/ci.yml`)

Two jobs on push/PR [code; never executed, since the project is not yet on a git host]:

1. `python`: Python 3.11, `pip install pytest pillow requests ruff`, `ruff check --select E,F,W --ignore E501,E701,E702 pi tools tests`, `pytest tests -q`. pyserial deliberately not installed (stubbed).
2. `firmware`: PlatformIO cache, copies `esp32-s3-ble/src/secrets.example.h` to `secrets.h`, then `pio run -e esp32-s3-devkitc-1`. **Precondition not yet met**: the workflow assumes credentials have been moved out of `config.h` into a git-ignored `secrets.h` with a committed example (see `_repo_prep/secrets_audit.md`). Until then this job cannot pass without real secrets. [unverified: the PlatformIO build itself was not run.]

## Not covered

Firmware (C++), the real serial/BLE hardware paths, systemd units, the web UI's JavaScript, the framebuffer write, `tools/carscanner_solve.py`, and any comparison of the Pi and S3 decoders. `_repo_prep/structure_proposal.md` suggests shared golden vectors between the two; nothing implements that yet.
