# Reading the data

Status: draft. Read the [disclaimer](../disclaimer.md). This is guidance for interpretation, not a diagnosis. Labels: **[Measured]** reported by the ECU and logged; **[Inferred]** a judgement from measured values; **[Speculation]** untested.

All figures refer to the verified car (2014 ix35 1.7 CRDi). The [PID map](../pid-map.md) shows how each was derived.

## The columns that matter

| Column | Meaning | Type |
|---|---|---|
| `dpf_soot_level_g` | ECU's soot estimate in grams; steps of about 0.39 g | [Measured] number, but it is the computer's model, not a filter measurement |
| `dpf_diff_pressure_hpa` | Pressure difference across the filter | [Measured] |
| `dpf_dist_since_regen_mi` | Distance since last regen, in miles | [Measured] |
| `egt_before_dpf_c` | Exhaust temperature before the DPF | [Measured] |
| `dpf_regen_active`, `dpf_regen_burning` | Regen in progress / hot burn phase | Measured bytes, but the meaning was derived from one reference regen [Inferred] |
| `dpf_zone_temp_c` | Actually catalyst temperature, not DPF temperature | [Measured] but misleadingly named |

## Soot (g)

- The author's ix35 rose from 12.2 g to 17.6 g over two short rides. The 14 g / 17 g / 28 g panel colours are the owner's choices, not manufacturer limits.
- [Inferred] A steadily rising soot estimate with no drop means regens are not completing.
- [Inferred] A soot number that falls after a regen is what you expect; a fall after a clean is evidence of a reset as much as of cleaning (the model gets reset).
- Do not treat a single number as a verdict. Trend over days matters.

## Differential pressure (hPa)

- Pressure rises with exhaust flow, so it is high at motorway speed for normal reasons. Compare **at the same warm idle** (engine warm, similar RPM).
- The 20 hPa at idle, RPM at or below 1100, flag is an unverified placeholder. The code does not check that the engine is warm. Do not quote it to customers as a limit.
- [Inferred] Higher idle pressure than the same car's own earlier baseline, when warm, is consistent with a more restricted filter. Consistent with, not proof.
- [Speculation] A useful workshop baseline would be many clean-filter cars at warm idle. None exists yet.

## Distance since regen

- [Measured] It resets when a regen completes.
- [Inferred] Long distance since last regen plus rising soot: regens are not happening or not finishing.
- Compare with the manufacturer's typical regen interval from the handbook or your own experience. This project has no number.

## Exhaust temperature (EGT before DPF)

- [Measured] It has to be high for a burn. The logger's own comment puts the hot burn phase at about 500 C and above.
- [Inferred] A regen that starts but never reaches burn temperature, or whose EGT falls off early, was interrupted or failed to complete. Short trips are the classic cause.
- The S3 alerts include a peak EGT figure when a regen finishes. Check the interrupted message: it triggers when data resumes after a gap over 120 s with a regen open.

## Blocked filter vs driving pattern

| Observation | Points towards | Confidence |
|---|---|---|
| Short trips, EGT never gets hot, soot creeps up, regens interrupted, pressure normal at warm idle | Driving pattern | [Inferred], moderate |
| Regens complete (flags, EGT high, distance resets) yet soot returns quickly and idle pressure high | Filter restriction or ash loading, possible sensor/EGR/injector issue | [Inferred], weak; needs your normal diagnosis |
| Idle pressure drops a lot after an offline clean, same conditions | Clean worked | [Inferred] |
| Pressure erratic or zero | Sensor/pipe fault or data gap | [Speculation]; check raw log |
| Cells empty | Adapter or ECU not answering; not a car problem | Check `raw_N.log` |

## Data quality checks

1. Does the raw log contain `NO DATA` or header-switch failures?
2. Are there large timestamp gaps (power loss on short trips)? The Pi fsyncs each row; the S3 flushes after each.
3. Is the timestamp real? The S3 rows are stamped with milliseconds only until its clock is set.
4. Was the engine warm for idle comparisons? Check coolant temperature. The logger does not.
5. Regen flags are thresholds on single bytes, unvalidated over many events. Cross-check with EGT and distance-since-regen.
