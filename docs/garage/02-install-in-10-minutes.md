# Install in 10 minutes

Status: draft. Timings are targets, not measured. The repository does not cover in-car power, so the power and battery section is **recommendation and speculation**, not tested behaviour.

## Before you start

- Confirm the car is supported. Today that means the 2014 Hyundai ix35 1.7 CRDi. Elsewhere the logger will mostly log empty cells.
- Get the customer's consent to fit the device and to keep the data (see [legal](05-legal-and-liability.md)).
- Have ready: a pre-paired logger (Pi 4 + Bluetooth adapter, or ESP32-S3 + BLE ELM327 adapter), with its own Wi-Fi/Telegram settings for that customer or none. Never reuse another customer's tokens or Wi-Fi credentials.

## Steps

1. **Ignition off.** Locate the OBD-II port (usually under the dash, driver side).
2. **Plug in the adapter.** Check it seats firmly and cannot foul pedals or knees.
3. **Power the logger.** See below.
4. **Ignition on, engine off.** Confirm the logger sees the adapter. On the Pi, a new `session_N.csv` appears in the log directory; on the S3, the web UI (own hotspot) shows live rows. Some cells are legitimately empty with the engine off.
5. **Start the engine.** Confirm RPM and coolant temperature are non-empty, then the DPF pressure and soot cells.
6. **Note in your job sheet:** date/time, odometer, adapter, logger, session number. The logger has no real-time clock; the S3 sets its clock from NTP or a phone, the Pi from its system clock, so check the timestamp is sane.
7. **Secure and tidy** the cable and device, out of the way of the driver.
8. **Brief the customer** with the [handout](04-customer-handout.md).

## Safety

- Nothing hangs where it can catch feet, pedals, airbags or steering.
- Do not leave a Pi (a full computer) in a hot, unventilated place such as a sun-baked dash. [Speculation] this is a heat and reliability risk; not tested.
- The logger is read-only, but any device on the OBD port is at the fitter's and customer's risk (see the disclaimer). Some ECUs and alarms/immobilisers misbehave with third-party OBD devices. [Speculation] on which cars.
- The logger must never distract the driver. The soot panel shows a "do not switch off" banner during regen; treat that as information, not instruction.
- Cheap adapters vary in quality. Use a known-good unit.

## Not draining the battery

The repository does not document in-car power. Recommendations, unverified:

- Prefer power that goes off with the ignition (switched 12 V or a USB port that is switched off), so the logger stops when the car does.
- Beware permanently-live OBD ports: many cars keep the OBD port live, so a logger powered from it can run the battery down over days. [Speculation] behaviour differs per car.
- The Pi 4 draws far more than an ESP32. [Speculation] on numbers; measure before promising duration.
- For a multi-day take-home, check battery voltage on return. The S3 build logs battery/module voltage (PID 0142), which will show a sagging battery. [Measured on the ix35 only.]
- Tell customers with old batteries or short-trip use that any add-on load matters more to them.

## Removal

1. Ignition off. Unplug the adapter, then power.
2. Copy `session_N.csv` and `raw_N.log` off the logger (SD card for S3, or the Pi's log directory). Keep the raw log with the CSV.
3. Note the removal time and odometer.
4. Check the car starts, dash shows no new warnings (report anything to the customer).
5. Wipe or securely store customer data per your data policy and hand a copy to the customer if they asked.
