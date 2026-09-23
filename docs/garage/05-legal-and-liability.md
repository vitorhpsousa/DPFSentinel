# Legal and liability

**This is not legal advice.** It is a list of issues to raise with a solicitor, your insurer and your trade body. It is a draft, written by a non-lawyer, and nothing here has been checked against current law. Laws change and differ by country.

## What the project already says

The [disclaimer](../disclaimer.md) states the tool is not a diagnostic tool, is verified on one car only, thresholds are the author's guesses, it is read-only but used at your own risk, and the authors give no warranty. A garage using it commercially should not rely on that text alone.

## Garage-specific points to raise with an adviser

### Disclaimer and no warranty
- Open-source licences usually disclaim warranty. The licence for this repository is still undecided. A licence does not stop *you* being liable to *your customers* for how you use the tool.
- Make your own customer-facing wording: the data is supporting evidence, not a diagnosis or guarantee. Do not claim it certifies a filter as clean, legal or road-worthy.
- Do not use it as the sole basis for a repair, a refusal, or a safety statement.

### Contract, consent and liability
- Get written consent before fitting a device and before keeping data.
- Check your terms of business, professional indemnity and public liability insurance cover fitting a third-party device and reporting from it. [Speculation] many policies are silent on this; ask.
- Consumer law: in the UK (Consumer Rights Act 2015) services must be performed with reasonable care and skill, and liability for death or personal injury from negligence cannot be excluded. EU member states have their own rules. Ask an adviser.

### Vehicle warranty and insurance
- Fitting equipment to the OBD port may affect a manufacturer or extended warranty position, or an insurer's view, for some vehicles. [Speculation] on frequency; check terms, and tell the customer.
- The project is read-only (it sends no clear-code, write or programming commands), but the ECU may still be affected by a poorly built adapter. Use good hardware.

### Data protection
- Logs contain vehicle data linked to a customer, and possibly timing/location patterns. In the UK/EU this is likely personal data (UK GDPR / EU GDPR). Consider: lawful basis, privacy notice, retention period, security, and deletion on request. The ESP32 build sends Telegram messages via a third party; the dashboards have no authentication and must not be exposed to the internet.
- Never put a customer's Wi-Fi password or Telegram token in shared or public files.

### DPF legality
- In the UK and EU, removing or defeating a DPF can be illegal and can fail an inspection (MOT or equivalent). This project only reads data and does not support removing a filter. Do not use it or its data to support DPF deletion.
- Emissions testing rules vary. Ask your local authority or trade body.

### Product and trade points, if you sell kits
- Selling hardware may trigger product-safety, radio-equipment (CE/UKCA), WEEE and battery rules. [Speculation] on which apply to which builds; ask.
- Marketing claims must be accurate. Do not advertise "diagnoses blocked DPFs".
- Trademarks: this repository names a phone app and adapter brands only as descriptions. Do not imply endorsement.

## Checklist before going live

1. Solicitor reviews your customer wording and consent form.
2. Insurer confirms cover in writing.
3. Data protection notice and retention policy in place.
4. Decision on the repository licence.
5. Written process for a customer complaint or a battery/electrical fault.
