# Third-party acknowledgements — CYD 3.5" ST77922 board

This firmware's code is original: written from scratch, or generated with AI
assistance directed by the project owner, from facts independently derived by
reading this exact board's own factory firmware and by live probing on real
hardware (see `../firmware_analysis.md` and `../board_notes.md`). No
third-party source file is compiled into this firmware.

Some pin numbers, register addresses and chip configuration facts were
cross-checked against other people's public work on similar hardware, as
good practice, and are credited here even though facts of this kind
(what a byte means, what value a register expects) are not protected by
copyright — see `docs/licensing/options.md` (the "caveat about GPL and PID
data" note) in the repo root for the fuller reasoning:

- **jlmeredith/ES3C35P** (MIT licence) — pin facts and hardware notes for
  a similar 3.5" ST77922 board.
  https://github.com/jlmeredith/ES3C35P
- **78/xiaozhi-esp32** (MIT licence) — board configuration facts (pin
  numbers, colour order) for a board using the same panel.
  https://github.com/78/xiaozhi-esp32
- **ESPHome's `st7123` touchscreen component** (GPL-3.0) — the touch
  controller's register map (addresses, bit meanings) was cross-checked
  against this driver's logic. This project's own touch driver
  (`src/board/board.cpp`) is an independent implementation, not a copy of
  ESPHome's source file.
  https://github.com/esphome/esphome
- **Espressif's public ES8311 reference driver** — the speaker codec's
  required register configuration was cross-checked against Espressif's
  own reference implementation.
- **DejaVu fonts** (Bitstream Vera licence) — see `tools/fonts/README`.

Raw copies of some of the above (downloaded for reference during bring-up)
are kept locally in `../vendor/` for the project owner's own use and are
**not** part of this firmware and are not distributed with it.
