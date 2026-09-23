# Disclaimer

**This is not a diagnostic tool.** It is a hobby data logger that records values the car's engine computer already reports. It has been verified on a single vehicle only.

- **Do not rely on it for safety.** It cannot tell you that a diesel particulate filter is blocked, failing, or safe to keep driving. The soot figure is the engine computer's own estimate, not a measurement of the filter. Some values may be missing, wrong for your car, or provisional (see [pid-map.md](pid-map.md) for what is verified and what is not).
- **Thresholds and colours are the author's guesses.** The 20 hPa idle-pressure flag, the panel's green/amber/red soot bands and the regeneration flags have not been validated over a long history and are not manufacturer limits.
- **Regeneration guidance is informational.** Messages such as "DO NOT SWITCH OFF UNTIL IT FINISHES" reflect general advice that interrupting a regeneration is undesirable. The flag behind them may be wrong or late. Follow your vehicle handbook, warning lights and a qualified mechanic, not this software. Never let this device distract you while driving.
- **Read-only, but at your own risk.** The code only requests readings and does not send clear-code, write or programming commands. Connecting any device to the OBD-II port, and any modification of firmware or a Raspberry Pi in a vehicle, is done at your own risk. The authors give no warranty and accept no liability for damage, loss, warranty or emergency-test consequences, or injury.
- **Trust your workshop's scanner** and judgement over this logger.
- **Security.** The dashboards have no authentication. Do not expose them to the internet. Keep Wi-Fi passwords, Telegram tokens and adapter addresses out of any published copy.
- **Compatibility.** Only the 2014 Hyundai ix35 1.7 CRDi has been tested. Other cars need their own discovery and verification.
