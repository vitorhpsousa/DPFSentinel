# 5. FAQ

**Can this stop my DPF blocking?**
No. It only shows what the engine computer reports.

**Is the soot figure accurate?**
It is the car's own estimate, not a measurement of the filter.

**Where do the green, amber and red limits come from?**
The owner chose them (14 g and 17 g). They are not manufacturer limits.

**Can it damage my car?**
The software only sends read requests, not clear-code or programming commands. Fitting anything to the OBD port is still at your own risk. See the [disclaimer](../disclaimer.md).

**Does it work on my car?**
Only a 2014 Hyundai ix35 1.7 CRDi has been checked. Other cars need their own testing.

**Do I have to keep the engine running during a regen?**
The screen says "do not switch off until it finishes". This is general advice and the flag may be wrong. Follow your handbook.

**Why is a reading blank?**
The car did not answer that request, or the engine is off. Blanks are normal.

**Does the Pi version send Telegram alerts?**
No. Only the ESP32-S3 version does.

**Is the web page secure?**
No. It has no password. Use it only on networks you trust and never expose it to the internet.

**Does it record my drives?**
Yes. Each drive is saved as a CSV file plus a raw log of the adapter's replies.

**I need more detail.**
See the [main README](../../README.md).
