# Telegram alerts (ESP32-S3 firmware)

The ESP32-S3 firmware (`esp32-s3-ble/`) can message you on Telegram when a DPF
regeneration starts, finishes or is interrupted, send a daily report with the
session logs, and send a periodic status ping. This guide sets it up end to end.

**Pi app:** the Pi logger also sends regen START / END / INTERRUPTED alerts
(`pi/alerts/`). Put `TG_BOT_TOKEN` and `TG_CHAT_ID` in the Pi's untracked
`config_local.py` (never in git), restart `obd-logger`, and test with
`python3 -m alerts.telegram_alerts test` from `pi/`. Use a separate bot per device.
The Pi sends no daily report or status pings (S3 only).

## 1. Create the bot

1. In Telegram open `@BotFather` and send `/newbot`. Pick a display name and a
   username ending in `bot`.
2. BotFather replies with a **token** (`<BOT_TOKEN>`, looks like
   `123456789:AA...`). Treat it like a password.

## 2. Get the chat id

1. Open your new bot in Telegram and send it any message (press Start).
2. In a browser open `https://api.telegram.org/bot<BOT_TOKEN>/getUpdates`
   (this is the method the comment in `esp32-s3-ble/src/config.h` describes).
3. Find `"chat":{"id":<CHAT_ID>, ...}`. For a private chat it is a positive number.

**Group:** add the bot to the group, send a message in it (in a group, mention
the bot or send `/start@yourbot` if `getUpdates` shows nothing; with privacy
mode on, bots only see commands and mentions), then read `getUpdates` again.
Group ids are negative (supergroups usually start with `-100`). Put the minus
sign in the value. (Group-id behaviour is standard Telegram Bot API, not tested
in this repo; the firmware just sends `chat_id=<value>` as a form field, see
`sendMessage()` in `src/report/telegram_report.cpp`.)

## 3. Put them in the firmware

Macros in `esp32-s3-ble/src/config.h`:

| Macro | Meaning |
|---|---|
| `TG_BOT_TOKEN` | Bot token. **Empty string switches all Telegram features off** (`reportBegin()` logs "Telegram report: off" and starts no task; `reportEvent()` returns immediately). |
| `TG_CHAT_ID` | Chat id as a string, e.g. `"<CHAT_ID>"` |
| `TG_SEND_HOUR`, `TG_SEND_MIN` | Daily report time, local time. Default 17:30 |
| `TG_SEND_RAW` | 1 = also upload `raw_N.log` files |
| `TG_MIN_BYTES` | Skip session files smaller than this (default 3000) |
| `TG_ALERT_REGEN` | 1 = regen start/finish/interrupted messages (checked in `main.cpp`) |
| `TG_ALERT_WIFI` | 1 = message with SSID and IP when a Wi-Fi network is joined |
| `TG_STATUS_ENABLED`, `TG_STATUS_INTERVAL_MIN`, `TG_STATUS_MIN_COOLANT_C` | Periodic status ping (default every 5 min, only while rpm > 0 and coolant >= 80 C) |
| `TG_HOST`, `TG_PORT`, `TG_USE_TLS` | `api.telegram.org`, 443, TLS on (leave alone) |

Wi-Fi for internet access: `WIFI_STA_SSID`/`WIFI_STA_PASS` and the second slot
`WIFI_STA_SSID2`/`WIFI_STA_PASS2` (tried alternately; home Wi-Fi plus a phone
hotspot). ESP32-S3 Wi-Fi is 2.4 GHz only. The board's own hotspot
(`WIFI_AP_ENABLED`, `WIFI_AP_SSID`, `WIFI_AP_PASS`) has no internet.

TLS: the certificate chain is pinned via `src/report/tg_root_ca.h` (Go Daddy root,
noted valid to 2037). If Telegram ever changes CA, sends will fail until that file
is updated.

Time zone is hard-coded to UK rules (`GMT0BST,...`) in `src/web/web_ui.cpp` and
`src/web/timekeeper.cpp`; change both if you are elsewhere, or the daily report
time will be off.

## 4. Keep the token out of git

**Warning:** in this project's working copy `config.h` contains a real token, chat id
and Wi-Fi passwords. Check your own copy before you commit or share anything,
and never publish those values (if you ever did, revoke them, section 7).

The firmware has no secrets file today (it needs a plain `#define`). A simple
pattern:

1. Move the secrets into `esp32-s3-ble/src/secrets.h` (contains `TG_BOT_TOKEN`,
   `TG_CHAT_ID`, `WIFI_STA_*`) and add `#include "secrets.h"` to `config.h`
   in place of those defines.
2. Add `esp32-s3-ble/src/secrets.h` to `.gitignore`.
3. Commit `secrets.h.example` with placeholder values.

Also do not paste `getUpdates` output or serial logs publicly: they contain
your token URL/chat id. Reasoning, not verified: the token is also compiled into
the firmware image, so treat built `.bin` files as secret too.

## 5. What triggers messages

From `src/report/regen_watch.cpp` (fed once per logged row from `main.cpp`):

- **Regen started**: the regen flag has been active for 3 consecutive rows
  (about 3 s at 1 row/s). Message has soot (g), miles since last regen, EGT and
  DPF pressure if known, plus the clock time if the clock is set.
- **Regen finished**: flag inactive for 3 rows. Duration, peak EGT, soot before -> after.
- **Interrupted**: data resumes after a gap of over 120 s while a regen was open
  (engine off or link lost). Sent when data resumes, not when it stops.
- **Still running** ping every 30 s during a regen (type `TICK`; also sent via the
  same queue).
- **Wi-Fi joined** (if `TG_ALERT_WIFI`), **status ping**, and the **daily report**
  (text summary plus uploads of session/raw files not yet sent; if the board was off
  at report time it sends at the next opportunity; failed sends retry after 10 min).

## 6. Offline queueing and test

Alerts go into a RAM queue (`reportEvent()` in `telegram_report.cpp`) and a
background task sends them every 5 s **only when Wi-Fi is connected and the clock is valid**
(NTP after joining, or set from a phone via the web UI `/api/time`).
Limits you should know:

- The queue holds **4 messages of 224 characters**; when full the **oldest is dropped**.
  The 30 s "still running" pings share it, so a long regen while offline can push the
  "started" message out. Start-of-regen is safest when a hotspot is already joined.
- RAM only: lost on reboot or power-off.
- With `WIFI_AP_ENABLED 1`, the board tries your home/hotspot network at boot,
  and afterwards **only retries every 10 minutes and only while the engine is
  not running** (`staService()` in `web_ui.cpp`). So enable your phone hotspot
  before switching the ignition on; it will not join one that appears mid-drive.
  Each attempt gives up after 20 s.

**Test:** with the board on a joined network, open
`http://<board-ip>/api/testalert` (or `http://obd-logger.local/api/testalert`).
It returns `{"queued":true}` and a "TEST alert" message should arrive within
about 5 s. Also available: `/api/statusnow` (immediate status ping),
`/api/send` (send the daily report now), `/api/report` (JSON status; `?reset`
forgets what was sent). The web UI is unauthenticated: only use it on networks you trust.
Serial output (115200) shows "Alert sent" per delivery.

If nothing arrives: check the serial log, the token/chat id, that you pressed
Start on the bot, and that the phone hotspot is 2.4 GHz.

## 7. Revoke / regenerate a token

In `@BotFather`: `/mybots`, pick the bot, **API Token**, **Revoke current token**
(or `/revoke`). The old token stops working at once. Put the new one in your
secrets file and re-flash. Do this if a token was ever pasted into a chat,
issue, screenshot or repo.
