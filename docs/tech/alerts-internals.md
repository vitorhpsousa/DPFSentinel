# Alerts internals (Pi)

Files: `alerts/regen_watch.py` (state machine), `alerts/telegram_alerts.py` (sender), glue in `main.py` (`feed_alerts`, `run_normal_loop`). Setup for users is in [../telegram-alerts.md](../telegram-alerts.md) (note: that draft still says the Pi has no Telegram support; it does now). Labels: [code] from source, [unverified] not checked.

## Wiring

`run_normal_loop` builds `sender = sender_from_config()` and `watch = RegenWatch()` once. `sender_from_config()` returns `None` unless both `TG_BOT_TOKEN` and `TG_CHAT_ID` are non-empty, in which case no alert code runs at all. Each cycle, after the CSV row is written and only if `sender` exists, `feed_alerts` is called with the values of `ALERT_COLUMNS`: `dpf_regen_active, egt_before_dpf_c, dpf_soot_level_g, dpf_dist_since_regen_mi, dpf_diff_pressure_hpa` [code].

`feed_alerts` wraps everything in `try/except Exception` and prints the traceback to stderr, so an alerting bug can never stop logging. It sends the event text only for `START`, `END`, `INTERRUPTED`; the `TICK` event type exists in `RegenWatch` (a "still running" ping every 30 s) but `main.py` does not send it (comment in `regen_watch.py`) [code].

Credentials: `TG_BOT_TOKEN`/`TG_CHAT_ID` belong in the untracked `config_local.py`. The token is embedded in the request URL (`https://api.telegram.org/bot<token>/sendMessage`) inside `_post`; nothing logs the URL, but do not add debug prints of it. Never commit real values.

## RegenWatch state machine

Pure logic, no I/O; time is injected: `update(now_ms, wall, active, egt, soot, dist, pressure) -> RegenEvent`. `main.py` passes `int(time.monotonic()*1000)` and `time.time()`. It is a port of the S3's `src/report/regen_watch.cpp` (`obd-esp32` repo), whose constants match (`STREAK_ROWS=3`, `GAP_MS=120000`, `TICK_MS=30000`) [code, both files read].

Constants: `STREAK_ROWS = 3`, `GAP_MS = 120_000`, `TICK_MS = 30_000`.

States: `in_regen` False (idle) or True (regen open). Per call, in order:

1. `active` is None/NaN -> return no event, and touch nothing (streaks and the gap timer are not advanced or reset). A blank row (link down) is therefore "invisible".
2. **Interrupted**: if `in_regen` and `now_ms - last_valid_ms > GAP_MS` (data just resumed after more than 2 minutes) -> emit `INTERRUPTED`, reset `in_regen` and both streaks, set `last_valid_ms = now_ms`, and return immediately (the current row is not counted toward a new regen). Detection only happens when a valid row arrives; if the logger never comes back, no message is sent [inferred from the control flow].
3. Update `last_valid_ms`, remember the last non-NaN soot, and while in regen track the peak EGT.
4. Streaks: `active > 0.5` increments the active streak and zeroes the inactive one; otherwise the reverse.
5. Transitions:
   - idle and active streak >= 3 -> `in_regen = True`, record start time/soot/distance/peak EGT, emit `START` ("soot X g, Y mi since the last regen, EGT Z C", plus DPF pressure if known).
   - regen and inactive streak >= 3 -> `in_regen = False`, emit `END` (duration in whole minutes, peak EGT, soot start -> end).
   - regen and >= 30 s since the last tick -> emit `TICK` (not sent by `main.py`).

Debounce means the alert appears about 3 poll cycles after the flag changes. Timestamps in the text: `_at()` appends " at HH:MM" only when `clock_synced()` is true and the wall time is after 2025-01-01 (constant 1735689600); otherwise the time is omitted rather than stated wrongly. `clock_synced()` runs `timedatectl show -p NTPSynchronized --value` (3 s timeout) and caches the result for 60 s; any error counts as "not synced". It is never true on a system without `timedatectl` [code].

Inputs come from `dpf_regen_active`, which is a single-byte threshold inferred from one reference regen (see [protocol-and-pids.md](protocol-and-pids.md)); the alert logic is only as good as that flag [inferred].

## Telegram sender

`TelegramSender(token, chat_id, post=None, max_queue=4, backoff=BACKOFF_S, sleep=None)`; `post` and `sleep` are injection points used by the tests.

- `send(text)` appends to a `collections.deque(maxlen=4)` under a condition variable and returns immediately. When full, the oldest message is silently discarded.
- A daemon thread takes `q[0]`, calls `post(text)`, and acts on the result:
  - `ok`: pop the message (only if `q[0]` still equals it), reset the failure counter.
  - `retry`: sleep `BACKOFF_S = (15, 30, 60, 120, 300)` seconds indexed by the failure count (capped at the last), and try the same head again. Messages are delivered in order.
  - `drop`: pop and print a one-line warning to stderr; no retry.
- `_post`: JSON body `{"chat_id", "text"}` (plain text, no parse mode), 10 s timeout. Classification: JSON `ok` true -> `ok`; HTTP 429 or >= 500 -> `retry`; other HTTP errors (bad token/chat, i.e. 4xx) -> `drop`; `URLError`/`OSError`/`ValueError` (offline, DNS, timeout, bad JSON) -> `retry`. Being offline in the car is the normal case, so a regen-start message waits until a network appears; the 4-message cap bounds memory.
- `close()` stops the thread; `main.py` never calls it (daemon thread dies with the process).

Manual check: `python3 -m alerts.telegram_alerts test` (run from the repo root) sends one message using the configured credentials and prints `sent` or a failure hint. It makes a real network call, so it is not run by the test suite.

## Tests

`tests_draft/test_alerts.py` (11 tests): start needs 3 rows, end needs 3 rows and reports soot drop, peak EGT tracking, interrupted-after-gap, missing flag ignored, clock shown only when synced, queue order, offline retry without loss, newest-4 retention, permanent rejection dropped, `feed_alerts` swallows errors. See [testing.md](testing.md).
