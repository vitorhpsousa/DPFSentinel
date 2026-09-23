# Dependency licence compatibility

> **Not legal advice.** "Verified" means read from a file on this machine during the review. Everything else is from general knowledge, marked **UNVERIFIED**, and must be checked against the package's own licence file or `pip show` / the PlatformIO library page before publishing.

Dependencies were read from `pi/requirements.txt`, `esp32-s3-ble/platformio.ini`, and imports in `pi/` and `tools/`.

| Component | Where used | Licence (as I believe) | Status | With GPL-3.0-or-later | With Apache-2.0 / MIT project |
|---|---|---|---|---|---|
| NimBLE-Arduino 1.4.3 (h2zero) | `esp32-s3-ble` BLE client, PlatformIO `lib_deps` | Apache-2.0 | **Verified**: `.pio/libdeps/.../NimBLE-Arduino/LICENSE` is the Apache-2.0 text; `library.properties` says 1.4.3. Its own upstream includes Apache NimBLE sources (also Apache-2.0); a sub-component check of file headers was not done. | Compatible (Apache-2.0 into GPL-3.0 is allowed) | Compatible |
| pyserial >=3.5 | `pi/` serial transport | BSD-3-Clause | **UNVERIFIED** (not installed here) | Compatible | Compatible |
| Pillow (`pillow`) | `pi/screen/soot_panel.py` drawing | HPND (MIT-CMU style) | **UNVERIFIED** | Compatible | Compatible |
| requests | `pi/alerts/telegram_alerts.py`, soot panel | Apache-2.0 | **UNVERIFIED** | Compatible | Compatible |
| bleak | `pi/obd/ble_transport.py` (BLE only) | MIT | **UNVERIFIED** | Compatible | Compatible |
| numpy | `tools/carscanner_solve.py` only (dev tool, not in requirements.txt) | BSD-3-Clause (bundled parts with other permissive licences) | **UNVERIFIED** | Compatible | Compatible |
| Arduino-ESP32 core (framework = arduino, platform espressif32) | Firmware: `Arduino.h`, `Preferences`, `FS`, WiFi, web server, `LittleFS` | Core is LGPL-2.1-or-later; ESP-IDF underneath is Apache-2.0 | **UNVERIFIED**; the platform packages are not in the project tree checked | LGPL-2.1+ to GPL-3.0 is permitted (LGPL allows conversion to GPL). Distributing a firmware binary carries LGPL relinking obligations regardless of your choice. | Distributing binaries needs the LGPL notice and a way to relink |
| DejaVu fonts | `pi/screen/soot_panel.py` loads from `/usr/share/fonts/truetype/dejavu` at runtime; **not bundled** in the repo | Bitstream Vera licence + public-domain additions | **UNVERIFIED**; not distributed by this repo, so no obligation unless you bundle them | Fine (permissive; only forbids selling the font by itself and renaming rule for modified fonts) | Fine |
| Telegram Bot API | `pi/alerts`, `report/telegram_report.cpp` (HTTPS) | A service, not a library; subject to Telegram's terms | **UNVERIFIED** terms | n/a | n/a |
| Telegram root CA (`astra/discovery/src/tg_root_ca.h`) | Certificate embedded in firmware | Public certificate data | **UNVERIFIED** origin/wording | n/a | n/a |
| Web UI (`pi/webui/static`, `esp32-s3-ble/src/web/ui_html.h`) | Dashboard | No external libraries or CDN URLs found by grep, so no third-party JS/CSS to attribute. **Verified** by search for `http(s)://` in those files (none other than local). | Verified by grep | n/a | n/a |
| Car Scanner (app) | Only as a source of reference captures | Proprietary app; its exports are not in the repo | Not a dependency | Do not add its exports to the repo without checking terms | Same |
| PlatformIO / Python | Build tools | Apache-2.0 / PSF | **UNVERIFIED**; not distributed | n/a | n/a |

## Notes

1. **Nothing found so far conflicts with GPL-3.0-or-later.** The only copyleft component in a distributed firmware image is the Arduino core (LGPL-2.1+), which is compatible in the direction that matters (LGPL library used by a GPL program).
2. **GPL-2.0-only would be a problem** with Apache-2.0 components (NimBLE). Use "GPL-3.0" or "GPL-3.0-or-later", never GPL-2.0-only.
3. **Firmware binaries.** If you ever publish a compiled `.bin`, the licence texts of everything inside it (Arduino core, ESP-IDF, mbedTLS, NimBLE, LwIP, etc.) apply to that binary. List them in `NOTICE`. The full list of transitive components comes from the PlatformIO build (`pio pkg list`, or the map file); it has not been enumerated here.
4. **Tool to verify**: run `pip-licenses` in a venv after `pip install -r pi/requirements.txt`, and `pio pkg list --only-libraries` plus each library's `LICENSE`. Replace the UNVERIFIED cells with results.
5. **Copied code check**: nothing was checked for snippets copied from forums or chatbots (the contributing notes mention chatbot-supplied PID lists). Anything pasted from another source needs its licence verified or removal.
6. **`OBD_BT_MAC`** in `pi/config.py` is a personal adapter address; not a licensing matter, but do not publish it (see `_repo_prep/secrets_audit.md`).
