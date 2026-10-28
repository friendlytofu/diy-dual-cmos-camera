# CamBuild integrated wiring — proposed build map

## Current purchased-parts revision

The current build has **five LEDs and two buttons only**: SW1 shutter and SW2 camera selector. Remove every SW3 record-button wire, R8 resistor, and rear-button ground return from older tables. Keep R6 and R7 as 10-kilohm pull-ups for U1 GPIO32 and GPIO33. The LCD connections below are still required.

For LCD1, connect U4 3.3 volts to VCC and BL, U4 ground to GND, U4 GPIO32 to DIN and U4 GPIO33 to CLK. Connect U4 GPIO13 directly to LCD DC. Connect U3 pin 5/Q5 to LCD CS and U3 pin 6/Q6 to LCD RST. This preserves the explicit chip-select and post-power reset behavior that made the isolated display test work without consuming camera or microSD pins. Keep C1, 0.1 microfarad, directly between U3 pin 16 and pin 8. U3 Q0–Q4 drive the five LED paths through R1–R5, each 1 kilohm.

Do not connect LCD CS, DC or RST to U4 GPIO25, GPIO26 or GPIO27. Those three pins are already used by the OV2640 camera. GPIO13 is available because firmware mounts SD2 in one-bit mode, which uses GPIO2, GPIO14 and GPIO15 and does not use DAT3/GPIO13.

**Status (2026-10-24):** The isolated GPIO-only test produced its color sequence, proving LCD1, its supply and its direct DIN/CLK/DC paths. The earlier faint-blue result came from the original display-control path rather than the panel. The final circuit now uses explicit CS and RST outputs from U3. The complete camera + SD + LCD + controls circuit has not yet been proven in one on-device run.

U2 (74HC165) is removed: its D1 input kept reading low even when 3.3 V was measured on that pin. U3 (74HC595) remains as the proposed output driver. Earlier LED observations were inconsistent (the green LED later failed to light), so its five output paths are **not** marked as validated. There is no MCP23017, IMU, battery gauge, AA holder, regulator, or separate microSD breakout. Begin with the [plain-language connection sequence](DETAILED_CONNECTION_SEQUENCE.md). Use the [human-readable connection table](WIRING_CONNECTION_TABLE.md), [labels](HARDWARE_LABELS.md), and [editable placement drawing](CamBuild_final_wiring.drawio) only as supporting references.

## Position the parts first

- **Outward face:** U1 with OV2640 and SD1 plugs into the upright 400-point half breadboard attached to the outward 830-point full breadboard. Keep its USB socket pointing up and lens clear. Put SW1 shutter by the upper-right grip, SW2 camera select below, and D5 red photo LED near the lens. Put R6/R7 near U1 GPIO32/33.
- **Selfie/user face:** U4 with OV2640 and SD2 sits on the other 830-point board. Put U3 across its center gap, C1 beside U3 pins 16/8, D1–D4 in a short red–yellow–green–blue row, and LCD1 on insulated supports with its 8-pin lead accessible. Put R1–R5 beside U3.
- **Between faces:** Insulate and mechanically space the boards. Route the two D5 conductors along an edge. The boards have **separate power and ground nets**. Do not join their 3.3 V rails or USB 5 V rails. Match the proposed hole coordinates to the printed carrier pin labels; carrier header geometry has not been measured.
- **Power:** PWR1 Anker A1653 has a built-in USB-C plug and a separate USB-C port. The Freenove FNK0060 [camera-board guide](https://docs.freenove.com/projects/fnk0060/en/latest/fnk0060/codes/C/34_Camera_Web_Server.html) specifies a **Micro-USB** cable; the metal WROVER-E marking alone does not identify the carrier's USB socket. If U4's actual socket is USB-C, the built-in plug may feed it directly if mechanically supported. If U4 is Micro-USB, use a properly wired USB-C-female-to-Micro-USB-male power adapter, supported so it cannot strain the board. CAB1 must match U1's actual socket: USB-C-to-USB-C or USB-C-to-Micro-USB. Do not force a USB-C plug into Micro-USB.

## Pins deliberately kept clear

Both cameras use GPIO4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 34, 35, 36 and 39 in the tested WROVER map. Each microSD card inserts into its carrier; firmware must use **one-bit SD_MMC** with GPIO2 D0, GPIO14 CLK and GPIO15 CMD. GPIO12 remains untouched. GPIO13/SD DAT3 is unused in one-bit mode and is assigned to LCD DC on U4 only. GPIO16/17 belong to WROVER PSRAM. U1 GPIO3 remains free. On U4, GPIO32 is shared by U3 DS and LCD1 DIN, GPIO1 supplies the U3 shift clock, and GPIO3/RX supplies the U3 storage clock. U4’s boot button and GPIO0 are not connected. The older claim that WROVER-E uses 1.8 V flash and needs GPIO12 high at boot was wrong: the [WROVER-E datasheet](https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html) specifies 3.3 V VDD_SDIO, and [Espressif's SD guidance](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/peripherals/sd_pullup_requirements.html) explains the GPIO12 boot-voltage conflict. Leave the carrier's GPIO12/SD DAT2 arrangement untouched; a new external pull-up or pull-down could disrupt either boot or card operation. Camera and card maps: [Freenove camera pins](https://github.com/Freenove/Freenove_ESP32_WROVER_Board/blob/main/C/Sketches/Sketch_06.1_CameraWebServer/camera_pins.h), [Freenove one-bit SD sketch](https://github.com/Freenove/Freenove_ESP32_WROVER_Board/blob/main/C/Sketches/Sketch_03.1_SDMMC_Test/Sketch_03.1_SDMMC_Test.ino).

## Exact electrical connections

U1-GND and U4-GND name different local nets. A breadboard rail stripe is not proof that all rail sections are joined; land each connection on the actual named net.

### Outward board and two-wire D5 harness

| Part/net | Exact connection |
| --- | --- |
| BTN-A | U1 GPIO32 = R6 lower end = SW1 contact A |
| R6 10 kΩ | U1 3V3 → BTN-A, fixed pull-up |
| SW1 shutter | BTN-A → switch → U1-GND |
| BTN-B | U1 GPIO33 = R7 lower end = SW2 contact A |
| R7 10 kΩ | U1 3V3 → BTN-B, fixed pull-up |
| SW2 selector | BTN-B → switch → U1-GND |
| J1-3 | Rear R5 output → D5 outward red LED anode |
| J1-4 | D5 cathode → U4-GND; **not** U1-GND |

Place each four-leg tactile button across the breadboard center trench. Use the two terminals that become connected **only when pressed**; two legs on the same side are already common. BTN-A and BTN-B sit near 3.3 volts while released and near ground while their button is pressed. U1 reads both as active-low digital inputs. R6 and R7 require no adjustment.

### Selfie/controller board, output register and display

The short names in parentheses match labels printed on the modules. Each row describes one complete connection and its purpose.

| Start point | End point | Purpose |
| --- | --- | --- |
| Selfie/controller ESP32 general-purpose input/output pin 32 | Output-register pin 14, serial data input (DS), and display DIN | Carries shared data. The output register and display accept it only when their own separate clock changes. |
| Selfie/controller ESP32 general-purpose input/output pin 1, also marked transmit (TX) | Output-register pin 11, shift-register clock input (SHCP) | Moves each data bit into the 74HC595. |
| Selfie/controller ESP32 general-purpose input/output pin 3, also marked receive (RX) | Output-register pin 12, storage-register clock input (STCP) | Transfers the completed eight-bit value to the output pins. |
| Resistor R10, 10 kiloohms | Selfie/controller ESP32 3.3-volt supply at one end; general-purpose input/output pin 3 and the storage-register clock connection at the other end | Keeps the storage-clock connection high during startup. |
| Selfie/controller ESP32 3.3-volt supply | Output-register pin 16, positive supply (VCC) | Powers the 74HC595. |
| Selfie/controller ESP32 3.3-volt supply | Output-register pin 10, master reset (MR) | Keeps the 74HC595 out of reset. |
| Selfie/controller ESP32 ground | Output-register pin 8, ground (GND) | Provides the 74HC595 ground return. |
| Selfie/controller ESP32 ground | Output-register pin 13, output enable (OE) | Keeps the 74HC595 outputs enabled. |
| Capacitor C1, 0.1 microfarad | Directly between output-register pin 16, positive supply, and pin 8, ground | Filters short supply disturbances. Keep both leads short and place the capacitor beside the chip. |
| Selfie/controller ESP32 general-purpose input/output pin 32 | Display serial data input, marked DIN | Sends pixel and command data to the display. |
| Selfie/controller ESP32 general-purpose input/output pin 33 | Display serial clock input, marked CLK | Clocks each display data bit. |
| Selfie/controller ESP32 3.3-volt supply | Display positive supply, marked VCC | Powers the display at the signal voltage. |
| Selfie/controller ESP32 ground | Display ground, marked GND | Provides the display ground return. |
| Output-register pin 5, output Q5 | Display chip-select input, marked CS | Selects the display only while a command or pixel block is being transferred. |
| Selfie/controller ESP32 GPIO13 | Display command-or-data input, marked DC | Selects commands or image data directly. One-bit SD mode does not use GPIO13. |
| Output-register pin 6, output Q6 | Display reset input, marked RST | Produces a deliberate low reset pulse after the display supply is stable. |
| Output-register pin 7, output Q7 | No connection | Unused. |
| Output-register pin 9, serial output (Q7S) | No connection | This unused pin must remain empty. |

LCD1 has eight pins named VCC, GND, DIN, CLK, CS, DC, RST, BL in the [Waveshare definition](https://www.waveshare.com/wiki/2inch_LCD_Module). Follow those printed names, not cable colors. Supply it from U4 3.3 V so supply and signal voltage match. Firmware drives CS and RST through U3 Q5/Q6.

The [official LCD schematic](https://files.waveshare.com/upload/e/ee/2inch_LCD_Module_SchDoc.pdf) shows BL feeding a 1 kΩ base resistor and an onboard NPN backlight switch. Connecting BL to U4 3.3 volts drives that control input, rather than powering the LED directly. This applies to the specified Waveshare SKU 17344 module. A differently labeled display needs its own schematic before this connection is reused.

| U3 output | Complete LED series path |
| --- | --- |
| Pin 15 Q0 | Q0 → R1 1 kΩ → D1 rear red anode → cathode → U4-GND |
| Pin 1 Q1 | Q1 → R2 1 kΩ → D2 rear yellow anode → cathode → U4-GND |
| Pin 2 Q2 | Q2 → R3 1 kΩ → D3 rear green anode → cathode → U4-GND |
| Pin 3 Q3 | Q3 → R4 1 kΩ → D4 rear blue anode → cathode → U4-GND |
| Pin 4 Q4 | Q4 → R5 1 kΩ → J1-3 → D5 outward red anode → cathode → J1-4 → U4-GND |

U3 and LCD1 share GPIO32 only as their data connection. U3 uses GPIO1 as its movement clock; LCD1 uses GPIO33 as its display clock. A latch pulse on GPIO3 updates U3 outputs. Firmware maintains LED bits Q0–Q4, LCD CS on Q5 and LCD RST on Q6; Q7 remains off and unconnected. LCD DC is direct on GPIO13 and LCD BL stays high from U4 3.3 volts. Firmware raises CS, pulses RST low after power is stable, then sends Waveshare's official 2-inch ST7789 initialization sequence. GPIO1/3 are UART TX/RX, so serial communication must be disabled after U3 control starts. Boot messages can transiently shift U3, but the GPIO3/RX latch pull-up holds the storage latch steady until firmware writes the first known byte.

## Expected behavior and limits

U1 reads SW1 and SW2 and sends button events to U4 over the WPA-protected local Wi-Fi network named `TOFUCAM`. U4 owns camera selection, LCD output, the no-login website and U3 state. LCD1 shows only the active camera image in landscape. Both cameras default to a QVGA 320 × 240 live preview. The outward image fills the LCD, while the sideways selfie image is rotated into a 180 × 240 portrait view with black side borders. Each SW2 press toggles the shared active-camera state. SW1 temporarily changes the selected OV2640 to its maximum UXGA 1600 × 1200 resolution and JPEG quality 8, saves the full-resolution image to SD1 or SD2, and then restores the preview setting. The website lists and downloads saved pictures from both cards and enlarges a selected preview. There is no recording or standby feature.

LED meanings are: startup **red → yellow → green → blue**; green ready; yellow when U4's card is unavailable; brief blue feedback; red plus yellow after a physical capture failure; D5 red while a photograph is being taken. R1–R5 are **all 1 kΩ**.

The electrical pin allocation avoids camera/SD collisions, but the complete device is **not guaranteed** until real integration verifies GPIO32 shared-data behavior, U3 power-up transients and LED paths, 3.3 V capacity, Wi-Fi/LCD frame rate and simultaneous power-bank load. [Anker describes A1653 as a two-port power bank](https://www.anker.com/uk/products/a1653-usb-c-portable-charger-5000mah). The hole coordinates below are proposed placements based on the photographed carrier orientation, not measured header positions.

## Proposed physical hole placement

This coordinate plan is for the Freenove carrier orientation shown in the project photos, **not for every board marked ESP32-WROVER-E**. The metal-module marking does not specify header order. Before placing any jumper, match the carrier's printed `GND`, `3V3`, `32`, `33`, `RX` and `TX` labels to the positions below. If even one differs, use the named-pin schedule above and leave the coordinate plan unused. This is a placement aid, not a claim of completed assembly.

`H` is the upright 400-point half board; `F` is the outward 830-point board; `R` is the rear 830-point board. A row's a–e holes share one contact strip; f–j share another. The center trench separates them. `L−` and `R+` below mean the rear board's **left blue** and **right red** power rails, not its normal row holes. U1's ground never goes on those rails.

| Part | Proposed positions |
| --- | --- |
| U1 carrier | Header rows H.a5–a24 and H.i5–i24; USB socket at row 5/upward. On this carrier orientation: GND H.a5, GPIO33 H.i17, GPIO32 H.i18, 3V3 H.i24. |
| U4 carrier | Header rows R.a41–a60 and R.i41–i60; USB socket at row 41. On this same photographed carrier orientation: GND R.a41, GPIO3/RX R.a56, GPIO1/TX R.a57, GPIO13 R.i46, GPIO33 R.i53, GPIO32 R.i54 and 3V3 R.i60. Match every point to the printed carrier label before wiring. |
| U3 74HC595 | Across rear trench: pins 1–8 in R.e16–e23; pins 16–9 in R.f16–f23. Notch at row 16. Thus Q0 pin15=R.f17; Q1 pin1=R.e16; Q2 pin2=R.e17; Q3 pin3=R.e18; Q4 pin4=R.e19; Q5 pin5=R.e20; Q6 pin6=R.e21; Q7 pin7=R.e22. |
| Rear LEDs | D1 red anode R.e4/cathode R.f4; D2 yellow R.e7/R.f7; D3 green R.e10/R.f10; D4 blue R.e13/R.f13. |
| Outward controls | SW1 across F.e14/F.f14; SW2 across F.e24/F.f24; D5 anode F.e35/cathode F.f35. |
| LCD1 | Insulated above R, with its eight individually identified lead ends landing in R.j24–R.j31 in this order: VCC, GND, DIN, CLK, CS, DC, RST, BL. The PH2.0 housing is **not** a breadboard plug; use suitable individual mating ends. |

The four-leg tactile switch positions name **electrical sides**, not individual metal legs. A switch must straddle the trench with one common pair on e and the opposite common pair on f. If its body does not fit the named rows, use short insulated leads to those same strips.

### Rails and local parts

| Connection | Exact holes |
| --- | --- |
| U4 local 3.3 V, lower and upper rear rail sections | R.j60 → R.R+60; R.h60 → R.R+20. This feeds both halves even when a marked rail break is present. Do not join to U1 3V3. |
| U4 local ground, lower and upper rear rail sections | R.b41 → R.L−41; R.c41 → R.L−20. Do not join to U1 ground. |
| R6 10 kΩ, BTN-A pull-up | H.f24 ↔ H.f18. H.i24 is U1 3V3; H.i18 is U1 GPIO32. |
| R7 10 kΩ, BTN-B pull-up | H.g24 ↔ H.g17. H.i17 is U1 GPIO33. |
| R10 10 kΩ, U4 GPIO3/RX latch pull-up | R.b38 ↔ R.b39; R.e38 → R.R+38; R.e56 → R.a39. |
| C1 0.1 µF, U3 bypass | R.g16 ↔ R.g15, then R.h15 → R.L−15. R.f16 is U3 VCC. Keep the capacitor next to that pin. |
| R1–R4 1 kΩ | R.b2↔R.b4, R.b5↔R.b7, R.b8↔R.b10, R.b11↔R.b13 respectively. Their lower ends share each LED's anode strip. |
| R5 1 kΩ | F.a38 ↔ F.a35; F.e35 is D5 anode. |

`R.R+20` and `R.L−20` mean rail holes beside row 20; `R.R+60` and `R.L−41` are beside rows 60/41. Their separate source wires avoid relying on the usual middle rail gap. Keep all connections to U4 rails within their supplied section.

### Jumper ends, one wire per line

| ID | From | To | Net |
| --- | --- | --- | --- |
| W01 | H.h18 | F.a14 | BTN-A to SW1 |
| W02 | H.h17 | F.a24 | BTN-B to SW2 |
| W03 | H.b5 | F.j14 | U1 ground to SW1 |
| W04 | H.c5 | F.j24 | U1 ground to SW2 |
| W07 | R.g17 | R.a2 | U3 Q0 to R1/D1 red |
| W08 | R.d16 | R.a5 | U3 Q1 to R2/D2 yellow |
| W09 | R.d17 | R.a8 | U3 Q2 to R3/D3 green |
| W10 | R.d18 | R.a11 | U3 Q3 to R4/D4 blue |
| W11–W14 | R.j4, R.j7, R.j10, R.j13 | R.L−4, R.L−7, R.L−10, R.L−13 | D1–D4 cathodes to U4 ground |
| J1-3 | R.d19 | F.b38 | U3 Q4 to R5/D5 anode |
| J1-4 | F.g35 | R.L−40 | D5 cathode to U4 ground |
| W15 | R.j16 | R.R+16 | U3 pin16 VCC |
| W16 | R.j22 | R.R+22 | U3 pin10 MR high |
| W17 | R.a23 | R.L−23 | U3 pin8 ground |
| W18 | R.j19 | R.L−19 | U3 pin13 OE low |
| W19 | R.j54 | R.g18 | U4 GPIO32 to U3 pin14 DS; shared with LCD1 DIN |
| W20 | R.j54 | R.g26 | U4 GPIO32 to LCD DIN strip |
| W21 | R.h53 | R.g27 | U4 GPIO33 to LCD CLK strip |
| W22 | R.e57 | R.g21 | U4 GPIO1/TX to U3 pin11 SHCP |
| W23 | R.e56 | R.g20 | U4 GPIO3/RX to U3 pin12 STCP |
| W24 | U4 header pin printed `13` | R.g29 | U4 GPIO13 to LCD DC strip |
| W25 | R.d21, U3 pin 6/Q6 strip | R.g30 | U3 Q6 to LCD RST strip |
| W26 | R.R+31 | R.g31 | U4 3.3-volt rail to LCD BL strip |
| W27 | R.g24 | R.R+24 | LCD VCC |
| W28 | R.g25 | R.L−25 | LCD GND |
| W29 | R.d20, U3 pin 5/Q5 strip | R.g28 | U3 Q5 to LCD CS strip |

J1-3 and J1-4 may be a labeled two-conductor plug or two separately labeled insulated jumpers. U3 pin 7/Q7 and pin 9/Q7S (R.f23) stay empty. SD1/SD2 and both cameras use their existing sockets; add no breadboard wires to them. The two Anker power paths are PWR1→U4 and PWR1→CAB1→U1, each with the socket-matched adapter or cable described above.
