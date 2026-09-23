# Screen renderer (`pi/screen/soot_panel.py`)

A full-screen status display for a 3.5" SPI panel on the Pi. This page is about the code; the hardware/overlay setup story is in [../pi-setup.md](../pi-setup.md) (section 8, 3.5" screen). Labels: [code] from source, [unverified] not checked.

## Data flow

1. Every `POLL_INTERVAL_S` = 1 s, `GET http://localhost:8080/api/live` (`requests`, 3 s timeout). It reads the same endpoint as the web UI (see [data-formats.md](data-formats.md)) rather than the CSVs. Any exception becomes `{"has_data": false, "_error": ...}`, so a dead dashboard shows "NO LOGGER DATA".
2. `render(state) -> PIL.Image` (480x320 RGB).
3. `to_rgb565_bytes(img)` converts to 16-bit little-endian RGB565 (`(r>>3)<<11 | (g>>2)<<5 | (b>>3)`, low byte first), 307,200 bytes.
4. The frame is written to `/dev/fb0` (opened `wb` each time) only if it differs from the previous frame's bytes. A steady state therefore costs no SPI traffic. The conversion is a pure-Python per-pixel loop (about 153,600 iterations per frame); CPU cost on a Pi 3 or 4 is not measured here [unverified].

Because the module opens `/dev/fb0`, the service runs as root (`pi/scripts/soot-panel.service`, `After=obd-dashboard.service`, `Restart=on-failure`). It ignores touch input.

Framebuffer/overlay assumptions (not enforced in code): RGB565 little-endian framebuffer and 480x320 landscape, which the author obtained with the vendor `mhs35` overlay and `rotate=90` [documented in pi-setup.md, "confirmed with photos", not re-verifiable]. If your driver differs, `FB_W`, `FB_H` and `to_rgb565_bytes` are the places to change.

## Screens

| Condition | Screen |
|---|---|
| `has_data` false | gray (`0x3a3f44`), "NO LOGGER DATA" |
| `stale` true, or `age_s` > 15 s (`STALE_AFTER_S`) | gray, "NO LIVE DATA". The server already sets `stale` at 8 s, so the 15 s check only matters if `stale` is missing [code] |
| `dpf_regen_active` truthy | near-black background (`0x1c1c1c`): "ACTIVE REGENERATION", amber "DO NOT SWITCH OFF" / "UNTIL IT FINISHES", soot in large type, "EGT (before DPF)" |
| otherwise | background by soot band, 2-column grid of 11 fields |

The no-data screens are deliberately static: an earlier "Xs old" counter forced a full redraw each second (comment, 2026-09-23). Values missing (`None`) render as an em dash. Stale frames are identical whatever the age, so they are not redrawn (covered by tests).

Grid fields (`DETAIL_FIELDS`, mirrors most of `LIVE_TILES` in `pi/webui/static/app.js` minus EGT and the EPS values): Soot (drawn larger), RPM, Speed, Coolant, Diff P., Cat. Temp (`dpf_zone_temp_c`, catalyst not DPF), Intercooler, MAF, Since Regen, Regen (ACTIVE/OFF/dash), Odometer.

Layout code stacks lines from a running y cursor using measured font heights (`textbbox`, `getmetrics`) instead of fixed offsets, after overlaps were found on the real screen (comments). The regen banner picks the largest bold font that fits 456 px (`_fit_bold_font`), shared by both banner lines. `draw_ink_top_aligned` aligns the ink, not the glyph box, so the em-dash placeholder does not collide with the row above.

Fonts: DejaVu Sans / Bold from `$SOOT_FONT_DIR` (default `/usr/share/fonts/truetype/dejavu`); if a font file is missing PIL's tiny default bitmap font is used silently, which looks broken (`_font`).

## Soot thresholds

`soot_color(soot_g)`:

| Soot (g) | Colour |
|---|---|
| None | gray |
| < 14 (`SOOT_GREEN_MAX`) | green `0x1fa54c` |
| 14 to < 17 (`SOOT_AMBER_MAX`) | amber `0xe8a548` |
| >= 17 | linear blend amber -> dark red `0x8b0000`, `t = (soot-17)/(28-17)` capped at 1 (`SOOT_DANGER_MAX` = 28) |

These are the owner's choices, tightened on 2026-09-23 so the range seen in short-trip driving reads amber/red (code comments). `SOOT_DANGER_MAX` is described in a comment as "purely a guess": there is no confirmed forced-regen figure for this ECU. They are UI colours, not a specification; the ECU's own trigger point is not known here. Tests pin the band edges and monotonic gradient (`tests_draft/test_soot_panel.py`).

The idle-blockage flag (`dpf_idle_flag`) and `DPF_TEMP_WARN_THRESHOLD_C` from `config.py` are used by the web UI only; the panel does not show them.

## Demo and previews

- `python3 screen/soot_panel.py --demo` cycles `DEMO_STATES` (no logger data, no live data, green, amber, red, dark red, regen active) every 30 s and writes them to the framebuffer. Soot values in the demo sit inside each band; update them if thresholds change (comment).
- `SOOT_FONT_DIR=... python3 tools/render_screen_previews.py OUT_DIR` renders the same states to PNG with no hardware (`tools/render_screen_previews.py`); the file names in `docs/img/` match the demo state names (an inference; how those PNGs were made is not recorded). The module imports `requests` and `PIL` at load time even for previews, and it builds fonts at import.

## Known limitations

- Only reads `/api/live`; if the Pi's clock is wrong (no RTC/NTP), `age_s` can look stale even though rows are fresh [inferred; see [data-formats.md](data-formats.md)].
- Hard-coded 480x320, `/dev/fb0`, port 8080; no config file.
- The plan to retire it once an ESP32-C3 round-screen unit exists is a project note, not code [unverified].
