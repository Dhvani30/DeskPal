# DeskPal Cube — Beginner Assembly Guide

This build has **six separate enclosure faces** and **one flat perfboard inside**. The perfboard does not fold into the cube. Each face folder shows that face as a flat 2D dotted placement plan, then adds its parts and wires step by step. The overview folder shows how the faces surround the internal perfboard shelf and how their wires connect.

> **Important:** The dots in the five wall-face folders are a planning grid, not five extra perfboards. Use your actual enclosure material for the walls. Your single physical perfboard is the internal shelf shown in `face_bottom/`.

## Start with the face folders

Each face folder contains five images. Open them in number order. Earlier parts fade gray; the current addition is highlighted.

| Folder | What it represents | Sequence |
|---|---|---|
| Front | OLED screen and its four-wire connection | [Step 1](face_front/step_01.svg) · [2](face_front/step_02.svg) · [3](face_front/step_03.svg) · [4](face_front/step_04.svg) · [5](face_front/step_05.svg) |
| Back | TP4056 charging board and separate USB-C access for the ESP32 | [Step 1](face_back/step_01.svg) · [2](face_back/step_02.svg) · [3](face_back/step_03.svg) · [4](face_back/step_04.svg) · [5](face_back/step_05.svg) |
| Top | TTP223 touch board, red tactile button, and SPDT quiet-mode switch | [Step 1](face_top/step_01.svg) · [2](face_top/step_02.svg) · [3](face_top/step_03.svg) · [4](face_top/step_04.svg) · [5](face_top/step_05.svg) |
| Bottom | The one internal perfboard shelf, ESP32 sockets, power parts, and buzzer | [Step 1](face_bottom/step_01.svg) · [2](face_bottom/step_02.svg) · [3](face_bottom/step_03.svg) · [4](face_bottom/step_04.svg) · [5](face_bottom/step_05.svg) |
| Left | Round INMP441 microphone breakout and sound opening | [Step 1](face_left/step_01.svg) · [2](face_left/step_02.svg) · [3](face_left/step_03.svg) · [4](face_left/step_04.svg) · [5](face_left/step_05.svg) |
| Right | BME280 module and ventilation openings | [Step 1](face_right/step_01.svg) · [2](face_right/step_02.svg) · [3](face_right/step_03.svg) · [4](face_right/step_04.svg) · [5](face_right/step_05.svg) |

See the [3D assembly overview](overview/cube_assembly.svg), [visual wiring map](overview/wiring_map.svg), and [face-to-face wiring notes](overview/README.md).

## What sits on each face

- **Front:** The OLED is fixed behind a front-wall window, with its screen facing out.
- **Back:** The TP4056 USB-C socket gets its own opening. The ESP32 DevKit has a second USB-C socket, so it needs a separate opening. The charger board and ESP32 are mounted inside; do not solder their USB sockets to a wall.
- **Top:** The TTP223 touch area, red tactile pushbutton, and slide switch are separate controls. In this DeskPal firmware, the slide switch selects quiet mode; it is not the battery power switch.
- **Bottom:** Feet and standoffs hold the single perfboard shelf above the bottom wall. The ESP32, charger, buzzer, and wiring sit on or around this shelf. The LiPo is secured beside it, protected from sharp solder legs.
- **Left:** The round INMP441 sound port faces outward. Keep the active buzzer inside and away from the mic.
- **Right:** The BME280 is exposed to room air through vents. Do not cover its sensor with glue or tape.

The six faces need a box frame, corner posts, screws, or another enclosure method. Perfboard solder joints do not hold the cube walls together. Make a cardboard mock-up first; the drawings are not exact cutting templates because your exact OLED, module, and enclosure dimensions are unknown.

## Parts and extra items

Your pictured parts are the classic 30-pin ESP32 DevKit with USB-C, OLED, TP4056 Type-C charging/protection module, 3.7 V 500 mAh LiPo, TTP223 touch module, SPDT slide switch, BME280 breakout, round INMP441 mic breakout, red tactile pushbutton, active buzzer, and one 2.54 mm dotted perfboard.

You will also need a soldering iron with stand, electronics solder, small wire cutters, wire strippers, multimeter, insulated hookup wire, female socket strips for the ESP32, and enclosure hardware. A regulated battery supply is also required. The firmware currently expects a 128×64 SH1106 OLED; check the label or product listing for your screen, because an SSD1306 screen needs the matching firmware setting. If the buzzer draws more current than an ESP32 GPIO can safely provide, add a transistor driver and base resistor. The exact buzzer rating is not visible in the supplied photo, so read its label/specification before wiring.

## Soldering safety and tools

1. Work with ventilation that moves fumes away from your face. Do not breathe solder smoke.
2. Wear eye protection. Keep the iron in its stand whenever it is not in your hand. The metal tip stays hot after unplugging.
3. Hold the iron like a pencil. Keep fingers behind the hot tip. Tie back long hair and keep paper, drinks, and the LiPo away from the iron.
4. If using a sponge, wet it and squeeze it so it is damp, not dripping. Briefly wipe the iron tip; do not press the sponge against it. Brass wool is another tip cleaner.
5. Solder is hot. Let each joint cool before touching it. Wash your hands after soldering, especially if using leaded solder.
6. Disconnect the battery before soldering or changing wires. Never solder directly to the LiPo pouch or cell tabs. Stop using a swollen, punctured, hot, or damaged cell.

Useful plain-English terms:

- A **pad** is the small metal ring around a perfboard hole or a flat metal spot on a module where a wire is soldered.
- **GND** is the shared electrical return connection, often called ground.
- **Continuity** is an unbroken electrical path. A multimeter often beeps when it finds one.
- **Tinning** means adding a tiny coat of solder to a wire end or iron tip to help solder flow. Do not melt solder directly on the iron and call that a joint; heat the wire and pad together.

## Wiring map for this DeskPal firmware

These pin assignments are taken from `DeskPal/DeskPalConfig.h` for the classic ESP32 profile. The module's printed pin labels still take priority. GPIO35 is input-only, which is suitable for mic data. GPIO34 is reserved in the firmware for optional battery measurement; that feature is disabled. **Never connect a battery directly to any ESP32 GPIO.** See the [Espressif ESP32 datasheet](https://documentation.espressif.com/esp32_datasheet_en.html).

| Module pin | Connect to | Plain-English job |
|---|---|---|
| OLED SDA | ESP32 GPIO21 | I2C data wire |
| OLED SCL | ESP32 GPIO22 | I2C clock wire |
| OLED GND | ESP32 GND | Shared return |
| OLED VCC | Regulated 3.3 V if the exact OLED board allows it | Module power; verify its pin labels and voltage |
| BME280 SDA / SCL | GPIO21 / GPIO22 | Shares the OLED I2C wires: SDA-to-SDA and SCL-to-SCL |
| BME280 GND | ESP32 GND | Shared return |
| BME280 VIN/VCC | 3.3 V only if the breakout accepts it | Do not assume a “5 V” board label means its signal pins are safe at 5 V |
| INMP441 SCK | GPIO18 | Audio clock |
| INMP441 WS | GPIO19 | Audio left/right frame clock |
| INMP441 SD | GPIO35 | Digital audio data into the ESP32 |
| INMP441 VDD / GND | 3.3 V / GND | The mic is a 3.3 V part; do not feed it 5 V |
| INMP441 L/R | GND if the breakout uses this for left-channel selection | Check the breakout label |
| TTP223 OUT | GPIO32 | Touch signal into the ESP32 |
| TTP223 VCC / GND | 3.3 V / GND, if module label agrees | Touch-board power |
| Tactile pushbutton | GPIO25 and GND | This is the firmware's mic-mute button; firmware uses `INPUT_PULLUP` |
| SPDT switch | Common pin to GPIO26; one outer pin to GND | Quiet-mode input; leave the other outer pin open |
| Active buzzer | GPIO27 through a driver if required | Check its `+` mark, rated voltage, and current first |

The OLED and BME280 share the two I2C wires. Many breakout boards already contain pull-up resistors (small parts that hold signal lines at a high voltage); do not add extra resistors unless the module documentation says they are needed. The INMP441 pin names above match the common breakout labels; verify your board before soldering. See the [INMP441 datasheet](https://product.tdk.com/system/files/dam/doc/product/sw_piezo/mic/mems-mic/data_sheet/inmp441.pdf).

## How wires travel between the faces

1. Mount each module on its own enclosure face using screws, a small bracket, or a suitable adhesive.
2. Make a small, smooth opening near that module's pins. Use an edge grommet or tape if the enclosure edge is sharp.
3. Solder a flexible insulated wire to each module pin or its header. Label both ends with tape (for example, `MIC SCK`).
4. Feed the wires through the opening toward the cube interior. Leave a relaxed loop so the face can open for repair.
5. On the internal shelf, solder each wire end to the matching labeled ESP32 socket pin, supply row, or shared signal row.
6. Keep every wire insulated. Bare wires must not touch each other or the battery pouch.

The overview uses red for regulated 3.3 V, black for GND, blue for I2C, yellow for mic audio, green for touch/button signals, and purple for the quiet-mode switch signal. These colors are drawing helpers; always follow the wire labels and module pin names.

## Soldering a wire to perfboard: tiny steps

Practice on spare wire and a spare hole before working on the DeskPal.

1. With the battery unplugged, strip about 3 mm of insulation from the wire.
2. Twist loose copper strands together with your fingers so none stick out sideways.
3. Push the bare end through a perfboard hole. Bend it slightly underneath so it stays in place when you turn the board over.
4. Hold the iron like a pencil. Touch its tip to both the copper wire and the metal ring around the hole for about one second.
5. Touch solder to the hot wire-and-ring joint, not to the iron tip. Let a small amount flow around both pieces.
6. Pull the solder away, then lift the iron. Keep the board still while the joint cools.
7. After it cools, gently tug the wire. Trim any long bare wire with cutters, with the cut end pointed away from your eyes.
8. Look for a small smooth joint. If solder touches a neighboring ring, do not power the board; remove the extra solder and inspect again.

**Checkpoint:** A good joint holds the wire without movement and does not touch the neighboring hole. If you are unsure, ask someone experienced to inspect it before adding power.

## Build order across all six folders

### A. Plan and dry-fit

1. Open each face's `step_01.svg` and mark which physical wall it represents.
2. Place your actual modules on a cardboard mock-up. Measure the OLED window, both USB-C ports, switch slot, mic sound hole, and BME280 vents from the parts themselves.
3. Put the real perfboard inside as a horizontal shelf. Check that the ESP32, charger, regulator, buzzer, wires, and LiPo all fit without pressing against the walls.
4. Keep the ESP32 antenna near a plastic wall and away from the battery and metal.

**Checkpoint:** The mock-up closes without pinching wires or squeezing the battery. No enclosure holes have been cut in final material yet.

### B. Assemble the face modules

1. Work through `face_front/step_01.svg` to `step_05.svg` for the OLED.
2. Work through `face_top/step_01.svg` to `step_05.svg` for the touch module, pushbutton, and switch.
3. Work through `face_left/step_01.svg` to `step_05.svg` for the microphone. Keep its sound port open.
4. Work through `face_right/step_01.svg` to `step_05.svg` for the BME280. Leave the air vents open.
5. Work through `face_back/step_01.svg` to `step_05.svg` for the charger opening and separate ESP32 USB-C opening.
6. Work through `face_bottom/step_01.svg` to `step_05.svg` for the internal perfboard shelf.
7. Label every wire at both ends before it enters the cube.

**Checkpoint:** Every module is held securely, the two USB-C ports have separate openings, and face wires reach the internal shelf without being stretched.

### C. Connect the internal shelf

1. Solder female socket strips to the perfboard so the ESP32 can be removed. Use the ESP32 to line up the two rows, tack the end pins first, check alignment, then solder the remaining pins.
2. Connect one black GND wire from an ESP32 GND pin to a ground row. Connect the module GND wires to that same row.
3. Connect ESP32 3V3 to a separate red supply row. Only connect modules verified for 3.3 V there.
4. Connect OLED and BME280 SDA together to GPIO21. Connect their SCL wires together to GPIO22. Do not swap the two signals.
5. Connect the mic wires to GPIO18 (SCK), GPIO19 (WS), and GPIO35 (SD), then connect its supply and ground.
6. Connect TTP223 OUT to GPIO32. Connect the tactile button between GPIO25 and GND.
7. Connect the SPDT common leg to GPIO26 and one outer leg to GND. Leave its other outer leg open. This switch selects quiet mode in the firmware; it does not switch battery power.
8. Connect the buzzer to GPIO27 only if its current is safe for that pin. Otherwise use a suitable NPN transistor driver and base resistor. The buzzer part rating is unknown from the photo, so do not guess.

**Checkpoint:** With USB and battery disconnected, use a multimeter to check each connection end-to-end. Confirm GND points connect, SDA reaches SDA, SCL reaches SCL, and no signal is shorted to 3.3 V or GND.

### D. Battery, charger, and enclosure

1. Identify the charger pads by their printed labels: `B+`, `B−`, and, if provided, `OUT+`, `OUT−`.
2. The LiPo positive wire goes to `B+`; negative goes to `B−`. Do not connect the battery until polarity is confirmed with the meter.
3. Use the protected `OUT` pads for the load only if your charger board has them and its instructions say to do so.
4. Add a 3.3 V regulator or suitable converter between the battery/load output and the ESP32 supply. TP4056 is a charger, not a voltage regulator. **Never connect raw LiPo voltage directly to ESP32 3V3 or 5V.**
5. Check the charger current setting against the battery maker's limit. 500 mAh is capacity, not a charging-current specification. If the cell maker permits 1C, 500 mAh corresponds to 500 mA; use a lower setting if required. The TP4056 current-setting resistor is described in its [datasheet](https://www.toppwr.com/uploadfile/file/20240307/65e993d6379ea.pdf).
6. A basic TP4056 does not provide a safe run-while-charging power path. The SPDT is already used for quiet mode, so add a separate load disconnect or unplug the load while charging.
7. Attach standoffs to the bottom wall, then attach the perfboard shelf above it. Secure the LiPo beside the shelf with a soft strap or tape, away from sharp solder legs and screw points.
8. Join the six enclosure faces with a frame, posts, or screws. Keep the back or top removable for service.

**Checkpoint:** Battery polarity, charger current, regulator output, and load-disconnect method are checked before the LiPo is connected.

## First power-up checklist

- [ ] Inspect for loose wire strands and solder bridges between neighboring holes.
- [ ] Battery disconnected: check that 3.3 V is not shorted to GND. A steady near-zero resistance means stop and find the short.
- [ ] Verify module labels at both ends of every wire. Do not trust color alone.
- [ ] Power the ESP32 from USB by itself first. Do not connect USB and battery power together unless the exact DevKit supports it.
- [ ] Add one module at a time, turning power off before changing wires.
- [ ] Connect the LiPo only after the regulator and charge-current setting are confirmed.
- [ ] Turn the load off/disconnect it during charging. Stop if the LiPo becomes hot, swollen, or damaged.

The face diagrams are 2D layouts; the [overview diagram](overview/cube_assembly.svg) explains how their separate walls meet around the internal shelf. Their dotted grids are not cutting templates. Measure your actual components before cutting the cube.
