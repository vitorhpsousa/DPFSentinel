# Release and versioning

Status first: **the project has no release process yet.** Nothing below describes an existing mechanism unless marked [code]; everything else is a proposal for the maintainer to accept or change. Labels: [code] verified in the tree, [proposal], [unverified].

## What exists today

- The repository is not under git (`/Users/vitor/Claude/OBD` has no `.git`); `_repo_prep/git_first_push.sh` is a helper that refuses to run while secret-like files, `logs/`, `azure-snapshots/`, `.pio/` or `venv/` exist, does `git init`, sets `main`, stages, asks for confirmation and makes one commit. It never pushes [code, read; never run].
- No version constant, `VERSION` file, changelog or tag anywhere in `pi/`, `tools/`, `esp32-s3-ble/` (`platformio.ini` and `config.h` carry none) [code, searched].
- The Pi is deployed by manual copy to `/home/ix35/obd-logger` (paths hard-coded in `pi/scripts/*.service`); the firmware is flashed with PlatformIO (`pio run`, environment `esp32-s3-devkitc-1`) [code].
- The only implicit "format version" is the CSV header row: files list their own columns (see [data-formats.md](data-formats.md)), and column names are effectively an API for the dashboard, panel and alert text.
- Planned hosting is Gitea on CasaOS with GitHub as a second remote (`_repo_prep/gitea/`, `self-hosting-gitea.md`); never run [unverified].

## Blockers before the first tag

From `_repo_prep/secrets_audit.md` / `structure_proposal.md` (not repeated in detail here): the Telegram bot token and Wi-Fi values in firmware `config.h` files must move to a git-ignored `secrets.h`, and the token should be regenerated, before any commit, because history is permanent. The adapter MAC in `pi/config.py` and `pi/scripts/bind_rfcomm.sh` should become a placeholder. No release notes or assets may contain logs with odometer/VIN data.

## Proposed versioning [proposal]

Semantic-ish, one version for the repository, with the parts noted in release notes:

- `MAJOR`: changes that break data: renamed/removed CSV columns, changed units or scales of an existing column, changed `/api/live` keys.
- `MINOR`: new PIDs/columns (added at the end), new features (alerts, transports), new firmware targets.
- `PATCH`: fixes and docs.
- Tags `vX.Y.Z` on `main`. The classic-ESP32 archive tag suggested in `structure_proposal.md` is `archive/esp32-classic`; the directory has since been renamed `esp32_DELETE_2026-10-30` (`_repo_prep/README.md`).
- Vehicle profile is not a version: state "verified on 2014 ix35 1.7 CRDi D4FD" in every release note, since PIDs are car-specific.

Suggested small code changes to make versions visible (none exist): a `pi/version.py` with `__version__`, logged at startup and exposed as `"version"` in `/api/schema`; a `FW_VERSION` macro in `esp32-s3-ble/src/config.h` printed at boot and on the web UI. Keeping a `# version` comment in the CSV is not advised; readers parse the header row.

## Proposed release checklist [proposal]

1. `pytest tests -q` green (221 passed, 1 expected xfail at the time of writing; see [testing.md](testing.md)).
2. `pio run -e esp32-s3-devkitc-1` builds with `secrets.example.h` (needs the secrets split).
3. Secret scan of the tree and of the diff since the last tag (`git grep` for token-shaped strings, MAC addresses, Wi-Fi passwords); confirm `.gitignore` covers `secrets.h`, `config_local.py`, `logs/`, `.pio/`, `venv/`.
4. Compare `pi/obd/pid_registry.py` with `esp32-s3-ble/src/obd/pid_registry.h` and `pid_decode.cpp` for the shared rows (they are hand-synchronised; see `structure_proposal.md` section 1.1).
5. If a column was added, changed or removed, check `pi/webui/static/app.js` tile keys, `pi/screen/soot_panel.py` `DETAIL_FIELDS`, `pi/alerts` `ALERT_COLUMNS`, and update [../pid-map.md](../pid-map.md) and [data-formats.md](data-formats.md).
6. Update the drafts that go stale (this project has several: the Telegram page still says the Pi has no alerts; `architecture.md`/`contributing.md` still describe `/api/schema` as broken and `pi/requirements.txt` as lacking `requests`/`pillow`; both are fixed in the code as of 2026-09-23).
7. Tag, push, and attach nothing but source. Flashing and Pi deployment stay manual: back up `/home/ix35/obd-logger` on the Pi first and avoid `rsync --delete` on the first deploy (advice from `structure_proposal.md`, step 9).

## Compatibility rules for contributors [proposal]

- Append new columns; do not reorder or rename existing ones without a MAJOR bump, because logs from earlier runs stay on disk with the old header.
- The Pi (21 columns) and S3 (28) already differ in order; consumers must key by header name.
- Anything that alters what is requested from the ECU stays read-only (see [../contributing.md](../contributing.md)).
