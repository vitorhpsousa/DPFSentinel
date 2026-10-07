# Dependency licence compatibility

> **Not legal advice.** "Verified" means read from a file on this machine during the review. Everything else is from general knowledge, marked **UNVERIFIED**, and must be checked against the package's own licence file or `pip show` / the PlatformIO library page before publishing.

Dependencies were read from the three `platformio.ini` files in this repo
(root, `boards/cyd-s3-3p5/firmware/`, `boards/waveshare-s3-touch-lcd-2/firmware/`)
and this repo's own `tools/` scripts. The separate, still-private `obd-pi`
repo has its own Python dependencies (pyserial, Pillow, requests, bleak,
numpy) which are **not** part of this table — see
[cross-project-notes.md](../cross-project-notes.md#dependency-licensing-the-pi-projects-python-dependencies)
for what was carried over from an earlier review of them.

| Component | Where used | Licence (as I believe) | Status | With GPL-3.0-or-later | With Apache-2.0 / MIT project |
|---|---|---|---|---|---|
| NimBLE-Arduino 1.4.3 (h2zero) | All three `obd-esp32` PlatformIO targets' BLE client (`lib_deps`: root, `boards/cyd-s3-3p5`, `boards/waveshare-s3-touch-lcd-2`) | Apache-2.0 | **Verified**: `.pio/libdeps/.../NimBLE-Arduino/LICENSE` is the Apache-2.0 text; `library.properties` says 1.4.3. Its own upstream includes Apache NimBLE sources (also Apache-2.0); a sub-component check of file headers was not done. | Compatible (Apache-2.0 into GPL-3.0 is allowed) | Compatible |
| Adafruit GFX Library 1.11.9 | `boards/cyd-s3-3p5/firmware` display driver, PlatformIO `lib_deps` | BSD-style (Adafruit's own licence, permissive) | **UNVERIFIED**: not re-read from the installed package in this pass; Adafruit publishes this library under a BSD-style licence per its repo | Compatible | Compatible |
| GFX Library for Arduino 1.5.0 (moononournation, `Arduino_GFX`) | `boards/waveshare-s3-touch-lcd-2/firmware` display driver (`src/board/board.cpp`), PlatformIO `lib_deps` | Apache-2.0 (per the library's own repo) | **UNVERIFIED**: not re-read from the installed package in this pass | Compatible | Compatible |
| pyserial >=3.5 | this repo's `tools/discover.py` (dev tool, not a firmware dependency) | BSD-3-Clause | **UNVERIFIED** (not installed here) | Compatible | Compatible |
| numpy | this repo's `tools/carscanner_solve.py` only (dev tool; this repo has no `requirements.txt`) | BSD-3-Clause (bundled parts with other permissive licences) | **UNVERIFIED** | Compatible | Compatible |
| Arduino-ESP32 core (framework = arduino, platform espressif32) | Firmware: `Arduino.h`, `Preferences`, `FS`, WiFi, web server, `LittleFS` | Core is LGPL-2.1-or-later; ESP-IDF underneath is Apache-2.0 | **UNVERIFIED**; the platform packages are not in the project tree checked | LGPL-2.1+ to GPL-3.0 is permitted (LGPL allows conversion to GPL). Distributing a firmware binary carries LGPL relinking obligations regardless of your choice. | Distributing binaries needs the LGPL notice and a way to relink |
| Telegram Bot API | `report/telegram_report.cpp`, duplicated per target (HTTPS) | A service, not a library; subject to Telegram's terms | **UNVERIFIED** terms | n/a | n/a |
| Telegram root CA (`src/report/tg_root_ca.h`, and the same file under each board's `firmware/src/report/`) | Certificate embedded in firmware (one copy per target: root, `boards/cyd-s3-3p5`, `boards/waveshare-s3-touch-lcd-2`) | Public certificate data | **UNVERIFIED** origin/wording | n/a | n/a |
| Web UI (`src/web/ui_html.h`, with a separate copy per board at `boards/cyd-s3-3p5/firmware/src/web/ui_html.h` and `boards/waveshare-s3-touch-lcd-2/firmware/src/web/ui_html.h`) | Dashboard | No external libraries or CDN URLs found by grep, so no third-party JS/CSS to attribute. **Verified** by search for `http(s)://` in all three `ui_html.h` files (none other than local). | Verified by grep | n/a | n/a |
| Car Scanner (app) | Only as a source of reference captures | Proprietary app; its exports are not in the repo | Not a dependency | Do not add its exports to the repo without checking terms | Same |
| PlatformIO / Python | Build tools | Apache-2.0 / PSF | **UNVERIFIED**; not distributed | n/a | n/a |

## Notes

1. **Nothing found so far conflicts with GPL-3.0-or-later.** The only copyleft component in a distributed firmware image is the Arduino core (LGPL-2.1+), which is compatible in the direction that matters (LGPL library used by a GPL program).
2. **GPL-2.0-only would be a problem** with Apache-2.0 components (NimBLE). Use "GPL-3.0" or "GPL-3.0-or-later", never GPL-2.0-only.
3. **Firmware binaries.** If you ever publish a compiled `.bin`, the licence texts of everything inside it (Arduino core, ESP-IDF, mbedTLS, NimBLE, LwIP, etc.) apply to that binary. List them in `NOTICE`. The full list of transitive components comes from the PlatformIO build (`pio pkg list`, or the map file); it has not been enumerated here. `boards/cyd-s3-3p5/firmware/NOTICE.md` and `boards/waveshare-s3-touch-lcd-2/firmware/NOTICE.md` exist today, but cover only the third-party sources used while reverse-engineering each board's pins/registers — not the full compiled dependency list this note describes, and not NimBLE/Arduino-ESP32/Adafruit-GFX/Arduino_GFX themselves. The root target (`platformio.ini` at the repo root) has no `NOTICE.md` at all; it needs one too if a compiled `.bin` for it is ever distributed.
4. **Tool to verify**: `pio pkg list --only-libraries` plus each library's `LICENSE`; for the two Python dev scripts, `pip-licenses` in a venv after `pip install pyserial numpy`. Replace the UNVERIFIED cells with results.
5. **Copied code check**: nothing was checked for snippets copied from forums or chatbots (the contributing notes mention chatbot-supplied PID lists). Anything pasted from another source needs its licence verified or removal.
