# Screen renderer

This repository has two touchscreen targets, each with its own on-device UI:
`boards/cyd-s3-3p5/firmware/` (3.5" CYD board) and
`boards/waveshare-s3-touch-lcd-2/firmware/` (2" Waveshare board). The root
build (repo root `src/`) has no screen at all — it only exposes the built-in
web UI. Both touchscreen boards run a small multi-page UI (`pages.cpp`/`.h`:
soot, trend, trip, a board-specific page, and system info), and on both of
them **the soot/regen screen is forced onto the display whenever a regen is
active**, overriding whatever page was open. This document is about that
soot/regen screen specifically — `soot_ui.cpp` on each board — which is the
one page shared, nearly line-for-line in logic, between the two boards
despite their very different screens and display libraries.

(A full-screen soot display also exists in the separate, still-private Pi
project, for a 3.5" SPI panel driven over the Linux framebuffer. It predates
both of these boards and uses a different language, library and font
renderer entirely. See
[cross-project-notes.md](../cross-project-notes.md#screen-renderer-the-pis-soot_panelpy)
for what was carried over from documenting it.)

## Display libraries and canvases

| | CYD 3.5" (`boards/cyd-s3-3p5`) | Waveshare 2" (`boards/waveshare-s3-touch-lcd-2`) |
|---|---|---|
| Panel | ST77922, QSPI, 320x480 (per `board_notes.md`) | ST7789, SPI, 240x320 |
| Library | Adafruit GFX Library 1.11.9 (`adafruit/Adafruit GFX Library@^1.11.9` in `platformio.ini`) | GFX Library for Arduino 1.5.0, moononournation (`moononournation/GFX Library for Arduino@1.5.0` in `platformio.ini`) |
| Off-screen canvas | `GFXcanvas16` (`UI_W`=480 x `UI_H`=320, landscape), allocated once in PSRAM by `board.cpp` (`new GFXcanvas16(UI_W, UI_H)`) | `Arduino_Canvas` (240x320 portrait), allocated once in `uiBegin()` on top of the board's `Arduino_GFX*` (`new Arduino_Canvas(W, H, g)`) |
| Text rendering | Custom anti-aliased renderer (`aatext.cpp`/`.h`): pre-rendered DejaVu Sans/Bold glyph bitmaps, alpha-blended onto the `GFXcanvas16`. Several fixed point sizes per font (e.g. `aaR17`, `aaB30`, `aaB44`), with `aaFit()` picking the largest size from an explicit largest-to-smallest list that still fits a given pixel width. | The GFX library's own built-in bitmap font via `setTextSize()`/`getTextBounds()` — no custom glyph data. `fitSize()` does the equivalent shrink-to-fit by trying text sizes from a maximum down to 1 until `getTextBounds()` reports a width that fits. |
| Redraw control | `pages.cpp` tracks a dirty flag and content hash; `uiDraw()` always paints into the canvas when called, and the page manager decides when that is. | `uiRender()` builds a `Frame` struct (mode, colours, every formatted value string) and `memcmp`s it against the previous frame (plus the odometer string, tracked separately); it returns immediately if nothing changed, so a steady state costs no redraw or panel flush. `uiInvalidate()` forces the next `uiRender()` to repaint, for use after another page has drawn over the panel. |
| Push to panel | `pages.cpp` presents the shared `GFXcanvas16` to the display. | `s_canvas->flush()` (an `Arduino_Canvas` method) after drawing into it. |

Both boards' soot screens are driven from the same `UiState` shape (soot,
rpm, speed, coolant, diffP, catTemp, intercooler, maf, sinceRegen, odometer,
regen, hasData, stale, ip, ssid) — the CYD's `UiState` additionally carries a
`clock` string (`"Sat 26 Sep 18:37"` or `"clock not set"`) that is drawn
bottom-right; the Waveshare board's soot screen does not show a clock at all
(its `PagesData` still tracks `clockText` for other pages, such as the
system page — the soot screen specifically just omits it, likely because the
smaller 240x320 panel has less room).

## Colour coding (soot level)

Both boards use **identical constants and logic** for the soot-level
background colour, matching each other byte-for-byte in `sootColor()`:

```cpp
static const float GREEN_MAX = 14.0f, AMBER_MAX = 17.0f, DARK_AT = 28.0f;
```

- `soot` is `NaN` (no value) → a dim grey, `rgb(0x3a, 0x3f, 0x44)`.
- `soot < 14.0` → green, `rgb(0x1f, 0xa5, 0x4c)`.
- `14.0 <= soot < 17.0` → amber, `rgb(0xe8, 0xa5, 0x48)`.
- `soot >= 17.0` → **not** a blend starting from the amber colour itself: at
  the 17.0 boundary the colour jumps to `rgb(0xd0, 0x2c, 0x20)` (a red,
  distinct from the amber band below it), then blends linearly from there
  towards dark red `#8b0000`, reaching full dark red at `soot == 28.0` (and
  staying there above it): `t = min(1, (soot - 17.0) / (28.0 - 17.0))`, then
  each of R/G/B is interpolated between `0xd02c20` and `0x8b0000` by `t`.

These three thresholds (14 g / 17 g / 28 g) are the project owner's own
choices for the one verified car, not a manufacturer specification — the
same numbers the Pi's original soot panel used (see
[../disclaimer.md](../disclaimer.md)). When the computed background happens
to equal the amber colour exactly, both boards switch the foreground text
and labels to a dark colour (`0x0000` text, `rgb(0x33,0x33,0x33)` labels)
instead of white, for contrast against the lighter amber background.

## Screen states

Both boards' soot screens have the same three states, driven by the same
`UiState` fields:

| Condition | What's shown |
|---|---|
| `!hasData` | centred message ("NO LOGGER DATA" on the CYD; two lines "NO LOGGER" / "DATA" on the Waveshare board, which fits a size-matched pair of strings to the narrower screen instead of one long one), grey background, plus the IP/SSID network line |
| `hasData && stale` | same layout as above, but "NO LIVE DATA" (CYD) / "NO LIVE" + "DATA" (Waveshare) |
| `regen` true | near-black background (`rgb(0x1c,0x1c,0x1c)`); an amber "DO NOT SWITCH OFF" / "UNTIL IT FINISHES" warning; the soot figure called out in amber as the number to watch; on the CYD, a 3x3 grid of Soot/RPM/Speed/Cat. Temp/Coolant/Diff P./Since Regen/Intercooler/MAF below the warning; on the Waveshare board (less screen space), just the big soot number, a "SOOT LEVEL" caption, and a single "Cat. Temp `<value>`" line |
| otherwise (normal) | background colour from the soot band above; a large "Soot" figure, then every other field in a small label/value grid |

### Normal-state grid layout

- **CYD (480x320 landscape):** Soot gets its own larger cell top-left,
  paired with RPM top-right in the same row; the remaining fields (Speed,
  Coolant, Diff P., Cat. Temp, Intercooler, MAF, Since Regen, Regen state,
  Odometer, and an "IP `<ssid>`" row) fill a 2-column x 5-row grid below
  that, each cell with a label line and a size-fitted value line. The clock
  is right-aligned at the very bottom.
- **Waveshare (240x320 portrait):** a "Soot" label and the big soot figure
  span the top (centred, in the largest size for a 7-character budget that
  still fits), then RPM, Speed, Coolant, Diff P., Cat. Temp, Intercooler,
  MAF, Since Regen, Regen state and Odometer fill a 2-column x 5-row grid
  below, each value's text size independently shrunk to fit its half-width
  column. A footer line holds the IP (and, if known, "WiFi: `<ssid>`" on a
  second line) — there is no clock row on this board's soot screen.

## Demo / bench-test modes

Both boards can render the soot screen with fabricated data, without a car
or logger link:

- CYD: toggling the firmware's demo mode (`demoActive` in `main.cpp`) jumps
  to `pagesGoto(PAGE_SOOT)`, fabricates a `UiState` with `demoFill()`, and
  renders it through the normal `pagesRender()` path every loop.
- Waveshare: `uiDemo()` in `soot_ui.cpp` cycles through 7 fixed states every
  7 seconds — no data, stale, green soot (8.2 g), amber soot (15.5 g), a
  blended amber-to-red soot level (21.0 g), a near-maximum soot level
  (28.0 g), and an active regen — calling `uiRender()` with each in turn.
  `pages.cpp`'s own `pagesDemo()` similarly cycles the other pages every 5 s.

## What to change where

- Soot-band thresholds and colours: `sootColor()` in each board's
  `src/ui/soot_ui.cpp` (and `src/ui/pages.cpp`, which defines the same
  constants again for its own trend-page colouring — keep both in sync by
  hand, they are not shared code today).
- Grid layout and field order: `uiDraw()` (CYD) / `drawFrame()` (Waveshare)
  in the respective `soot_ui.cpp`.
- Fonts: CYD regenerates its anti-aliased glyph bitmaps from
  `tools/gen_fonts.py` (per the comment in `aatext.h`) rather than loading a
  font file at runtime; the Waveshare board has no equivalent step since it
  uses the GFX library's built-in font.
