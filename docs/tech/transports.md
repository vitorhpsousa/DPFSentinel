# Transports: BLE

This repository's ESP32-S3 targets only support BLE (`src/obd/elm_client.cpp`
per target, using NimBLE-Arduino): there is no rfcomm/Bluetooth-Classic path
here. The real adapter-discovery and connection behaviour — scan vs.
cached-address connect, `BLE_TARGET_ADDRESS`/`BLE_NAME_HINT`, the Nordic UART
service preference, and reconnect-after-two-failures behaviour — is
documented in [../esp32-s3.md](../esp32-s3.md); that is the page to read for
this repository's own firmware.

## One central at a time

A BLE ELM327 adapter accepts a single connection — this is a general
BLE-adapter property, not something this repository's firmware controls. If
a phone app (or another logger) is already connected to the adapter, this
repository's firmware will look dead until that connection is closed. See
also [../adapting/other-cars.md](../adapting/other-cars.md).

## The Pi project's transports

The separate, still-private Pi project supports both Bluetooth Classic SPP
(for the OBDLink MX+, via `rfcomm`) and BLE (via `bleak`, with a USB-dongle
pinning trick to keep the BLE adapter on a separate Bluetooth controller from
the MX+). This page used to document that transport layer — `config.py`,
`obd/elm_client.py`, `obd/ble_transport.py`, `scripts/bind_rfcomm.sh` — in
detail; none of those files exist in this repository. See
[cross-project-notes.md](../cross-project-notes.md#transports-rfcomm-ble-usb-dongle-pi)
for what was carried over from it, including the failure-handling behaviour
that is specific to the Pi's Python logging loop (`run_normal_loop`).
