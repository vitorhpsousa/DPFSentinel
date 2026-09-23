# Business models around an open-source core

Status: draft thinking, not market research. Nothing here is validated. Numbers are deliberately absent. **[Speculation]** applies to the whole document unless a fact is cited from the repository.

## Facts from the repository

- Verified on **one car** only; other cars need discovery work.
- Two builds: Pi 4 (Bluetooth Classic adapter) and ESP32-S3 (BLE adapter, Telegram alerts). Only the Pi has been used on the real car for long.
- No automated tests. Web dashboards have no authentication.
- Licence undecided.

This is the central business constraint: the tool is only as valuable as the number of car models with verified data.

## Model A: services (garage does the work)

Sell DPF health checks or before/after clean reports using the logger.
- Pros: lowest capital; uses the garage's trust and skills; customers understand "we checked and here's the evidence".
- Cons: only works on verified cars; per-job time; liability sits with the garage ([legal](05-legal-and-liability.md)); hard to differentiate if any competitor can install the same code.

## Model B: hardware kits

Pre-flashed, pre-paired loggers sold or hired.
- Pros: recurring physical value; a simple offer.
- Cons: hardware, returns, warranty and compliance burden; margins thin against a cheap adapter plus free code; support load from users' unusual cars; safety and radio rules.

## Model C: paid support and car onboarding

Charge to add and verify a new car's PID map (the discovery step in [contributing](../contributing.md)).
- Pros: directly addresses the main gap; knowledge stays open source; fits an open-core project.
- Cons: skilled, slow work; each car requires a reference capture; hard to price; only valuable if many customers share a car.

## Model D: hosted dashboards and alerts

Cloud storage of logs, fleet view, and managed alerts for garages.
- Pros: recurring revenue; garages do not run servers; multi-car comparison improves baselines over time (long-term).
- Cons: significant new engineering (accounts, security, privacy, uptime). The current dashboards have no authentication and must not be exposed. Data-protection duties rise. Not built.

## Model E: training and documentation

Workshops on reading DPF data, and a garage certification.
- Pros: cheap; builds a community.
- Cons: small market; credentials mean nothing until the data is trusted.

## Open-source considerations

- **Licence choice shapes the model** (permissive vs copyleft vs source-available). Undecided; take legal advice.
- Competitors can copy the code; your advantage becomes trust, service, verified car data and speed.
- Community car profiles are valuable but need quality control; a wrong PID map gives wrong data on a customer's car.
- Trademarks and warranties: keep the "no warranty" and "not a diagnostic tool" position honest.

## Suggested order [Speculation]

1. Prove the tool on more than one car and get a written baseline dataset.
2. Pilot with one or two friendly garages under the services model, with consent and disclaimers.
3. Only then consider kits or hosting.

## Honest risks

- Market size unknown; some garages already own professional scan tools with DPF live data.
- Accuracy claims cannot exceed what is validated.
- Support for odd cars can eat the time.
- Legal exposure if data is oversold as a diagnosis.
