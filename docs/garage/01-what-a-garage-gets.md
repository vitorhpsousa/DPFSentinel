# What a garage gets

Status: draft. Read the [disclaimer](../disclaimer.md) first. Legend used throughout these guides:

- **[Measured]** a value the engine computer reports and the logger records, decoded and checked on one car.
- **[Inferred]** a conclusion drawn from measured values. Useful, but a judgement.
- **[Speculation]** an idea not yet tested.

**Important limit.** Only a 2014 Hyundai ix35 1.7 CRDi (D4FD) has been verified. On any other car the logger will not give meaningful DPF data until someone does the discovery work ([contributing](../contributing.md)). Everything below describes what the tool does on that car, and what it could do for a garage once a car's PIDs are verified.

## The value in one sentence

A garage can hand the customer a timestamped record of what the DPF was doing before and after the work, instead of "it's clean now, trust us."

## What the logger records [Measured]

Once a second, per drive, into a CSV plus the raw adapter replies: ECU soot estimate (g), differential pressure across the filter (hPa), exhaust temperature before the DPF, distance since last regeneration, regen flags, RPM, speed, coolant temperature and odometer. The ESP32-S3 build can also send a Telegram message when a regen starts, finishes or is interrupted.

## Four uses

### 1. DPF health evidence for the customer
A few days of logs show soot trend, how often regens complete and how long they run. That is a page of evidence you can show, rather than a verbal opinion. [Inferred] It does not prove the filter is healthy or blocked; it is supporting data next to your normal diagnosis.

### 2. Pre- and post-clean comparison
Log a drive (or a hot idle) before a forced regen or chemical/offline clean, and the same again after. Compare soot estimate and idle differential pressure at a similar warm idle.
- [Measured] both numbers.
- [Inferred] a fall in idle pressure at the same warm idle after cleaning suggests less restriction.
- Caveat: the ECU soot figure is the computer's model, and it typically resets or drops after a regen or after the ECU is told the filter was serviced, whether or not the filter is actually clean. Pressure is the more physical signal; the soot estimate alone is weak proof.
- Caveat: the project has no calibrated pressure limits. Compare the same car with itself, same conditions.

### 3. Regeneration verification
After a forced or drive-cycle regen, the log shows whether the regen flags came on, the exhaust temperature before the DPF reached burn levels, and whether distance-since-regen reset. [Inferred] A complete cycle is evidence the regen ran; the flags were derived from a single reference regen and are not validated over many events.

### 4. Diagnosing short-trip drivers
Take-home logging for a week shows trip lengths, speeds and whether the exhaust ever got hot enough for long enough. [Inferred] If soot climbs with short trips and regens keep being interrupted, the pattern points at driving use rather than a faulty filter. See [reading the data](03-reading-the-data.md).

## What it does not do

- It does not clear codes, write to the ECU or force a regen. It is read-only.
- It does not replace your scan tool, differential pressure sensor tests, or visual/flow inspection.
- It has no calibrated "blocked" threshold. The 20 hPa idle flag and the panel colours are the author's guesses.
- [Speculation] Other cars, other makes' manufacturer PIDs, and a fleet of adapters have not been tried.
