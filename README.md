# DeskPal Perfboard Assembly Guide

**Board:** 7 × 9 cm dotted perfboard (28 × 36 holes @ 2.54 mm pitch)
**MCU:** ESP32 DevKit 30-pin (USB overhanging bottom edge)
**Target:** Self-contained IoT desk assistant (OLED + BME280 + INMP441 + buzzer + touch/button/switch)

---

## Zoning Rationale

| Zone | Location (holes) | Components | Reasoning |
|------|------------------|------------|-----------|
| **Display North** | Top edge, rows 1–8 | OLED, BME280 | Readable angle, I2C bundled short |
| **Acoustic East-West split** | Mic top-left (cols 1–5, rows 1–6); Buzzer bottom-right (cols 24–28, rows 30–36) | INMP441, Buzzer | Diagonal separation > 100 mm kills acoustic feedback |
| **Compute Core** | Center, rows 14–30 | ESP32 DevKit | Shortest average trace to all peripherals |
| **Human Interface South** | Bottom edge, rows 32–36 | Touch, Button, Switch | Thumb reach, away from I2S |
| **Power Spine** | Left (3V3) and right (GND) verticals | Rails + passives | Single-run rails, no loops |

**Key physical rules**
- Mic and buzzer on **opposite diagonals** — never share a GND return segment.
- I2S lines (purple) run along the **top-left edge**; I2C (yellow) runs along the **top-center**. They never parallel for more than 15 mm.
- 10 kΩ pullups sit at the **OLED cluster**, not at the ESP32, so I2C sees one termination.
- 100 nF ceramic within 5 mm of ESP32 3V3 pin; 10 µF bulk at OLED/BME280 junction.
- Touch (GPIO32) trace kept < 25 mm and off any I2S or clock line.

---

## Step 1 — Prepare the Board & Silkscreen

**Side:** Top
**Reference:** `![Step 1](step_01.svg)`

1. Lay the perfboard with the **copper side up** for reference. Mark rows/columns 1–28 (X) and 1–36 (Y) on masking tape.
2. Transfer the zoning map: draw a pencil box around the ESP32 footprint (cols 8–21, rows 14–30) and small corner dots for every module.
3. Mark Pin 1 of each header with a red sharpie dot at the top-left corner of each footprint box.

**Verification:** Count 28 dots across and 36 down. Every module's Pin 1 red dot is visible.

---

## Step 2 — Solder the 10 kΩ I2C Pullups

**Side:** Bottom
**Reference:** `![Step 2](step_02.svg)`

1. Locate the passive cluster at cols 24–28, rows 14–20.
2. Insert resistor R1 between the SDA feed hole (col 24, row 15) and the 3V3 spine hole (col 24, row 14). Bend legs flat on the copper side.
3. Insert R2 between the SCL feed hole (col 26, row 15) and 3V3 (col 26, row 14). Solder and trim.
4. Bridge the two 3V3-side legs together with a short solder bead so both pullups share one 3V3 tap.

**Verification:** DMM resistance SDA↔3V3 ≈ 10 kΩ; SCL↔3V3 ≈ 10 kΩ; SDA↔SCL open (>1 MΩ).

---

## Step 3 — Solder the Decoupling Capacitors

**Side:** Bottom
**Reference:** `![Step 3](step_03.svg)`

1. **C1 (100 nF):** Insert directly under the ESP32 3V3 pin (col 8, row 15). One leg to 3V3 spine, other to the nearest GND hole (col 9, row 15).
2. **C2 (10 µF):** Insert at cols 26–27, rows 18–19, between the OLED/BME 3V3 feed and GND. Observe polarity (stripe = −, to GND).
3. Solder both on the bottom side; keep C1 body flat against the board.

**Verification:** DMM 3V3↔GND reads **> 1 kΩ** (no dead short). Capacitor stripe (C2) faces the GND spine.

---

## Step 4 — Install ESP32 Female Headers

**Side:** Top (headers), soldered from bottom
**Reference:** `![Step 4](step_04.svg)`

1. Place two 15-pin female headers into cols 8 and 21, spanning rows 15–29 (or per your DevKit's pin length).
2. Tack the four corner pins from the **bottom side** first. Flip board and check the headers sit square against the board.
3. Solder all 30 pins. Use a helping-hand clamp to keep headers vertical.
4. **Do not insert the ESP32 yet.**

**Verification:** No continuity between any two adjacent header pins. Headers are perpendicular to the board (check with a square edge).

---

## Step 5 — Install Peripheral Headers

**Side:** Top (headers), soldered from bottom
**Reference:** `![Step 5](step_05.svg)`

1. **OLED** — 4-pin header at cols 8–11, row 2 (GND, VCC, SCL, SDA order per your module).
2. **BME280** — 4-pin header at cols 20–23, row 5.
3. **INMP441** — 6-pin header at cols 1–3, rows 3–5 (VDD, GND, L/R, WS, SCK, SD).
4. **Buzzer** — 2-pin header at cols 25–26, row 33.
5. **Touch / Button / Switch** — 2-pin headers at cols 4–6, 9–11, 14–16, row 34.

**Verification:** Every header's Pin 1 aligns with the red sharpie dot. No pin shorts to neighbors.

---

## Step 6 — Route the 3.3V Spine (Red)

**Side:** Bottom
**Reference:** `![Step 6](step_06.svg)`

1. Tin a solid-core 22 AWG wire (or heavy solder bridge) along col 5 from row 14 down to row 33.
2. Branch east at row 14 to the ESP32 3V3 pin (col 8), to the passive cluster (col 24), and to the BME280 header.
3. Branch east at row 33 to the buzzer (col 25) and the OLED 3V3 (col 9, row 2 — run the branch up the left side).
4. Solder-drop every joint with a 2 mm blob.

**Verification:** Continuity from col 5/row 14 → ESP32 3V3 → OLED 3V3 → BME 3V3 → buzzer + < 1 Ω each.

---

## Step 7 — Route the GND Spine (Black)

**Side:** Bottom
**Reference:** `![Step 7](step_07.svg)`

1. Tin a solid-core wire along col 24 from row 2 down to row 33.
2. Branch west at row 2 to OLED GND (col 8, row 2).
3. Branch west at row 15 to ESP32 GND (col 21), C1's ground leg, and the mic GND (col 3, row 4 — route along top edge).
4. Branch at row 33 to buzzer −, and to switch/button/touch GND stubs.

**Verification:** Continuity GND → every module GND pin < 1 Ω. **3V3 to GND reads > 1 kΩ** (no short).

---

## Step 8 — Route the I2C Bus (Yellow)

**Side:** Bottom
**Reference:** `![Step 8](step_08.svg)`

1. **SDA (GPIO21):** From ESP32 pin (col 10, row 15) run north along col 10 to row 6, then east to the OLED (col 11, row 2) and BME280 (col 20, row 5).
2. **SCL (GPIO22):** From ESP32 pin (col 11, row 15) run parallel 2.54 mm east, to OLED SCL and BME280 SCL.
3. The two lines must stay parallel and 2.54 mm apart the whole run; cross power rails at 90° only.

**Verification:** SDA–SCL > 1 MΩ; each line < 5 Ω end-to-end; pullups confirmed on both.

---

## Step 9 — Route the I2S Bus (Purple)

**Side:** Bottom
**Reference:** `![Step 9](step_09.svg)`

1. **SCK (GPIO18)** from ESP32 pin col 9, row 15 → up col 9 to row 8 → west to mic col 3, row 3.
2. **WS (GPIO19)** from col 10, row 15 → up col 10 to row 7 → west to mic col 2, row 3.
3. **SD (GPIO35)** from col 12, row 15 → up col 12 to row 6 → west, crossing under the WS line, to mic col 1, row 3.
4. Space lines ≥ 2.54 mm apart; no I2S line runs parallel to the 3V3 spine for more than 15 mm.

**Verification:** Each I2S pin < 5 Ω to mic pad; no short to 3V3 or GND; lines visually do not parallel the red rail.

---

## Step 10 — Route the Buzzer & Front Controls

**Side:** Bottom
**Reference:** `![Step 10](step_10.svg)`

1. **Buzzer (GPIO27):** From ESP32 pin col 20, row 15, run south along col 20 to row 33, east to buzzer + at col 25.
2. **Touch (GPIO32):** From ESP32 pin col 13, row 29, short stub south to touch header at col 5, row 34.
3. **Button (GPIO25):** From col 14, row 29 to button at col 10, row 34.
4. **Switch (GPIO26):** From col 15, row 29 to switch at col 15, row 34.
5. If using bare switches, add 10 kΩ pullups to 3V3 on each control line, placed near the header.

**Verification:** Continuity each GPIO to its header pin; momentary continuity when button/switch/touch is pressed.

---

## Step 11 — Insert the ESP32 & Modules

**Side:** Top
**Reference:** `![Step 11](step_11.svg)`

1. **Insert the ESP32 last.** Align pins, press gently, confirm USB overhangs the bottom edge.
2. Insert OLED (top edge, upright), BME280 (top-right), mic (top-left, port facing outward), buzzer (bottom-right), and the three front controls.
3. Do **not** power on yet.

**Verification:** All modules seated fully, no bent pins, no metal contacts touching the 3V3 spine.

---

## Step 12 — Final Continuity & Insulation Check

**Side:** Both
**Reference:** `![Step 12](step_12.svg)`

1. Continuity check every rail and bus with DMM in beep mode.
2. Check that **no component metal shell or leg** touches the 3V3 or GND spine except at its designated joint.
3. Add hot glue or kapton tape over any exposed wire crossing another wire.
4. Confirm the USB connector is clear of the board edge (≥ 3 mm).

**Verification:** All Step 6–10 checks still pass; no continuity between 3V3 and GND (>1 kΩ); no bridge between SDA and SCL.

---

## Pre-Power-On Checklist

- [ ] **3V3 ↔ GND > 1 kΩ** (no dead short).
- [ ] **Polarity**: C2 (10 µF) stripe faces GND; mic L/R pin configured for left channel; buzzer + to GPIO27.
- [ ] **Pin 1 alignment** on all headers matches the silkscreen dots; ESP32 USB faces outward.
- [ ] **Visual scan**: no solder bridges between adjacent ESP32 header pins; no I2S wire touching the 3V3 spine.

If all four pass → plug in USB, watch for the OLED to illuminate, then flash firmware.

---

## Common Mistakes

1. **Pullups at the wrong end** — placed at ESP32 instead of OLED cluster → longer I2C stubs and ringing. Fix: move to cols 24–28.
2. **I2S parallel to 3V3** — causes buzzer noise to couple into the mic. Fix: re-route purple lines along the top edge, cross red rail at 90° only.
3. **Buzzer on the same GND return as mic** — injects switching transients. Fix: separate GND branches meeting only at ESP32 GND.
4. **Forgot C1 decoupling** — ESP32 brownouts during Wi-Fi TX. Fix: 100 nF within 5 mm of 3V3 pin, mandatory.
5. **Touch trace next to I2S** — false touches. Fix: keep GPIO32 stub < 25 mm and ≥ 5 mm from any purple line.
6. **ESP32 USB not overhanging** — cable cannot plug in. Fix: shift DevKit down so USB body extends beyond board edge.
7. **Solder bridge between 3V3 and GND spines** — instant dead short. Fix: visual + DMM check after every rail.