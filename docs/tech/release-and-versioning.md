# Release and versioning

Status first: **the project has no formal release process yet** (no version tags, no `VERSION` file, no changelog). Nothing below describes an existing mechanism unless marked [code]; everything else is a proposal for the maintainer to accept or change. Labels: [code] verified in the tree, [proposal], [unverified].

## What exists today

- This repository **is** under git and hosted on GitHub (`github.com/vitorhpsousa/DPFGuardian`), licensed GPL-3.0-or-later — see the root `LICENSE` file and [README](../../README.md). [code, verified]
- No version constant, `VERSION` file, changelog or git tag anywhere in this repository (`platformio.ini` and each target's `src/config.h`/`config.example.h` carry none) [code, searched]. The separate Pi project is not checked here.
- The three ESP32-S3 targets in this repository are each flashed independently with PlatformIO (`pio run`, environments `esp32-s3-devkitc-1` at the repo root, and the two boards' own environments under `boards/*/firmware/`) [code]. The separate Pi project is deployed by its own means, not described here.
- The only implicit "format version" is the CSV header row: files list their own columns (this repo's own column list is in [../architecture.md](../architecture.md); see also [data-formats.md](data-formats.md)), and column names are effectively an API for the dashboard, panel and alert text.
- Hosting this repository's own copy on a self-hosted Gitea (in addition to GitHub) is described in [../self-hosting-gitea.md](../self-hosting-gitea.md); [unverified] whether that has actually been done for this repo.

## Blockers before the first tag

The Telegram bot token and Wi-Fi values in firmware `config.h` files must stay out of git history: every target (repo root and each board under `boards/`) uses a git-ignored `src/config.h` copied from its own tracked `src/config.example.h` (see the README's "Secrets" section). No release notes or assets may contain logs with odometer/VIN data.

## Proposed versioning [proposal]

Semantic-ish, one version per repository, with the parts noted in release notes:

- `MAJOR`: changes that break data: renamed/removed CSV columns, changed units or scales of an existing column, changed `/api/live` keys (note each ESP32-S3 target's `/api/live` shape already differs from the Pi project's; a bump in one does not imply a bump in the other).
- `MINOR`: new PIDs/columns (added at the end), new features (alerts, transports), new firmware targets.
- `PATCH`: fixes and docs.
- Tags `vX.Y.Z` on `main`.
- Vehicle profile is not a version: state "verified on 2014 ix35 1.7 CRDi D4FD" in every release note, since PIDs are car-specific.

Suggested small code changes to make versions visible (none exist): a `FW_VERSION` macro in each target's `src/config.h`, printed at boot and on the web UI. Keeping a `# version` comment in the CSV is not advised; readers parse the header row.

## Proposed release checklist [proposal]

1. There are no automated tests in this repository today to run as a gate (see [testing.md](testing.md)).
2. `pio run` builds clean for all three targets: the repo root and each of `boards/cyd-s3-3p5/firmware/` and `boards/waveshare-s3-touch-lcd-2/firmware/`.
3. Secret scan of the tree and of the diff since the last tag (`git grep` for token-shaped strings, Wi-Fi passwords, BLE addresses); confirm `.gitignore` covers each board's `src/config.h`.
4. Compare `src/obd/pid_registry.h` + `pid_decode.cpp` across all three targets in this repository for any shared rows that drifted out of sync (they are hand-synchronised, per-target, today). If the separate Pi project's PID table is meant to stay aligned too, check that separately.
5. If a column was added, changed or removed, check each target's `src/web/ui_html.h` tile keys and alert columns, and update [../pid-map.md](../pid-map.md) and [../architecture.md](../architecture.md).
6. Update any docs pages that describe behaviour that has since changed in the code.
7. Tag, push, and attach nothing but source.

## Compatibility rules for contributors [proposal]

- Append new columns; do not reorder or rename existing ones without a MAJOR bump, because logs from earlier runs stay on disk with the old header.
- The three ESP32-S3 targets in this repository, and the separate Pi project, already differ in column count and order; consumers must key by header name, not position.
- Anything that alters what is requested from the ECU stays read-only (see [../contributing.md](../contributing.md)).
