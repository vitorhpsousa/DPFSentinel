# Transports: rfcomm, BLE, USB dongle

Selected by `TRANSPORT` in `config.py` ("rfcomm" default, or "ble"). Per-device overrides go in an untracked `config_local.py`, imported with `from config_local import *` at the end of `config.py` [code]. `ElmClient` only needs four serial-like calls (`reset_input_buffer`, `write`, `read`, `close`), so either transport plugs in with no other change (`obd/elm_client.py`).

## rfcomm (Bluetooth Classic SPP)

Used with the OBDLink MX+.

- `scripts/bind_rfcomm.sh` (root) runs `rfcomm release 0` then `rfcomm bind 0 <MAC> 1`, waits 1 s and checks `/dev/rfcomm0` exists. Channel 1 is assumed to be SPP [code comment; the script suggests `sdptool records` if it is not].
- The MAC is hard-coded in the script and in `OBD_BT_MAC` in `config.py`. The config value is **unused** by the code (the script keeps its own copy); keep them in sync or edit only the script [code, grep shows no reader]. It is a device identifier: replace it with a placeholder before publishing.
- Pairing/trusting is a one-off manual step (`bluetoothctl`), described in the script header.
- `ElmClient` opens `serial.Serial(RFCOMM_DEVICE, 115200, timeout=0, write_timeout=2.0)`. Baud is irrelevant for SPP but pyserial needs a value. The write timeout makes a stalled write raise instead of blocking the loop [code comment].
- `obd-logger.service` runs the bind as `ExecStartPre=-...` (the leading `-` ignores failure, so the BLE transport still starts if the bind fails) and runs the whole service as root because `rfcomm bind` needs it. `Restart=on-failure`, `StartLimitIntervalSec=0`.
- The `config.py` comment says the MX+ supports a simultaneous second Bluetooth connection (used for calibration with a phone app). [unverified in this repo.]

## BLE (`obd/ble_transport.py`)

For BLE ELM327 adapters (Konnwei, vLinker, Veepeak BLE ...). Uses `bleak` on top of BlueZ; imported lazily, only when `TRANSPORT == "ble"` (`main.make_transport`), so rfcomm installs do not need it. `bleak` is listed in `requirements.txt` with a comment saying it is optional.

Design:

- `BleTransport` runs a private asyncio loop in a daemon thread and exposes blocking calls; every coroutine is submitted with `run_coroutine_threadsafe` and a timeout (connect: `scan_timeout + connect_timeout + 10`; write: 8 s; disconnect: 5 s). A failed connect raises `OSError("BLE connect failed: ...")`.
- Notifications append to an internal buffer under a lock; `read(n)` returns up to `n` buffered bytes, or `b""` if empty, or raises `OSError("BLE link lost")` once the disconnect callback has fired and the buffer is drained. That is how the logger's existing `except Exception` reconnect path is triggered.
- `write()` chunks to `max(20, mtu - 3)` bytes; uses write-without-response when the characteristic allows it, otherwise write-with-response.
- Discovery (mirrors the S3 firmware per the module docstring; the S3 side was not re-read here):
  1. If `BLE_ADDRESS` (or the class-level cached address from an earlier connect) is set, look it up by address. A configured but missing address is an error; a merely cached one falls back to a name scan.
  2. Else scan for `BLE_SCAN_TIMEOUT_S` and pick the strongest-RSSI device whose name contains a hint: `BLE_NAME_HINT` if set, otherwise any of `OBD, ELM, VLINK, V-LINK, VEEPEAK, IOS-, VGATE, KONNWEI, KW9, KW8, KW-, OBDLINK, MX+, LELINK, CX` (case-insensitive).
- Characteristic choice (`pick_characteristics`): if `BLE_SERVICE_UUID`, `BLE_CHAR_TX_UUID` and `BLE_CHAR_RX_UUID` are set, use exactly those. Otherwise prefer the Nordic UART service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`, then any non-generic service (skipping 0x1800, 0x1801, 0x180A) with a notify/indicate characteristic (RX) and a different writable one (TX); if only one characteristic does both (HM-10 style), it is used for both.
- Bring-up helpers: `python3 -m obd.ble_transport scan` and `... dump ADDRESS` (run from the repo root; `--adapter`, `--seconds`).

Settings (`config.py`): `BLE_ADAPTER` (`"usb"`, `"hciN"`, or `""` = BlueZ default), `BLE_ADDRESS`, `BLE_NAME_HINT`, `BLE_SERVICE_UUID`, `BLE_CHAR_TX_UUID`, `BLE_CHAR_RX_UUID`, `BLE_SCAN_TIMEOUT_S` (10).

## USB dongle

`resolve_adapter("usb")` scans `/sys/class/bluetooth/hci*` in sorted order and returns the first whose `device` symlink resolves to a path containing `/usb`; it raises `OSError("no USB Bluetooth adapter found ...")` if none [code]. Reason (code comment): hciN numbering can change between boots, and the MX+ pairing lives on the onboard adapter, so the BLE adapter is kept on a separate controller. `prepare_adapter()` best-effort runs `rfkill unblock bluetooth` and `btmgmt --index N power on` (needs root, which the systemd unit has; errors are swallowed). The BLE transport is passed the adapter name via bleak's `adapter=` keyword. [Unverified: behaviour with more than one USB dongle; the first wins.]

Note: the project notes say this was verified offline only (unit tests with a faked sysfs), not on a car [project memory, not in code].

## One central at a time

A BLE ELM327 adapter accepts a single connection (stated in the `TRANSPORT` comment of `config.py`; a general BLE-adapter property, not tested here for each model). Consequences:

- If the ESP32-S3 (or a phone app) is connected, the Pi cannot connect, and vice versa. If both keep trying they take the adapter from each other, so run one logger at a time, or use the MX+ over rfcomm on the Pi and the BLE adapter on the S3.
- The Pi's failure mode: connect fails -> `OSError` -> caught in `run_normal_loop` -> blank row, close, retry after `RECONNECT_DELAY_S` = 5 s, same session file.

## Failure handling common to both

`run_normal_loop` (`main.py`) catches every exception, writes one blank row, closes the adapter and retries every 5 s (rationale in code comments: process restarts by systemd produced hundreds of near-empty session files on 2026-09-16 and 2026-09-18). It warns once per distinct error (repeat warnings suppressed until the message changes or the link recovers). After `BLANK_STREAK_LIMIT` = 10 consecutive fully blank cycles it raises `ConnectionError`, forcing a reconnect and re-init even though the link looked up. `KeyboardInterrupt` is not caught inside the loop.
