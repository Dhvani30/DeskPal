# Overview: how the faces connect

The front, back, top, left and right are separate enclosure walls. The bottom wall carries feet and standoffs. One flat perfboard is held horizontally above the bottom wall as the internal shelf. The perfboard does not become any of the walls.

Each face diagram is a flat 2D placement view. Mount the actual module to its wall, pass flexible labeled wires through a smooth opening, and solder the wire ends to the matching rows or connectors on the internal shelf. Keep a small service loop so the walls can open.

## Connection map

| From | To | Wire / note |
|---|---|---|
| OLED SDA and BME280 SDA | ESP32 GPIO21 | Blue I2C data; join SDA only to SDA |
| OLED SCL and BME280 SCL | ESP32 GPIO22 | Blue I2C clock; join SCL only to SCL |
| OLED/BME280 confirmed 3.3 V supply | Regulated 3.3 V row | Red; do not assume a 5 V labeled board is 3.3 V logic-safe |
| INMP441 SCK / WS / SD | GPIO18 / GPIO19 / GPIO35 | Yellow audio bundle; matches DeskPalConfig.h |
| INMP441 L/R | GND | Select left channel if the breakout label agrees |
| TTP223 OUT | GPIO32 | Green control signal |
| Momentary tactile button | GPIO25 and GND | Firmware uses `INPUT_PULLUP`; this is the mic-mute button |
| SPDT switch | GPIO26 common; one outer pin to GND | Quiet-mode input; leave the other outer pin open |
| ESP32 GPIO27 | Active buzzer through suitable driver if required | Check buzzer rated voltage/current and polarity |
| TP4056 B+ / B− | LiPo + / − | Confirm the actual board markings and polarity |
| TP4056 OUT+ / OUT− (if present) | Separate load disconnect/regulator input and common GND | Use protected output as the module instructions say |

Every module ground, ESP32 ground, and regulator ground must be connected together. Power and ground wires are not the same as signal wires. GPIO35 is input-only and is used for mic data. GPIO34 is reserved for optional battery measurement, which is disabled; never connect a battery directly to a GPIO.

## Safe power-up order

1. Build and inspect with the battery disconnected.
2. Check the ground and signal wires using a multimeter.
3. Power the ESP32 from USB alone for the first test.
4. Add one face module at a time, removing power before each wiring change.
5. Use the battery only after the regulator output and TP4056 current setting have been verified for the 500 mAh cell.
6. Disconnect the load while charging with the TP4056. The SPDT switch is a quiet-mode input, not a load disconnect; add a separate disconnect or unplug the load.

The 500 mAh number is cell capacity, not its safe charge-current setting. Follow the battery maker's charge-current limit. A “1 A” charger board may need its current reduced before use with this cell.