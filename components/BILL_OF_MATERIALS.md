# TofuCam bill of materials

Quantities below are for one two-camera build. Match the labels in `design/HARDWARE_LABELS.md` and the active wiring guide. The assembled wiring and printed fit still require confirmation on the physical build.

## Camera, display, and storage

| Part | Quantity | Specification or role |
| --- | ---: | --- |
| U1 outward camera | 1 | Freenove ESP32-WROVER-E carrier with OV2640 and built-in microSD slot |
| U4 selfie camera | 1 | Same carrier and camera; also controls LCD1 and the website |
| LCD1 | 1 | Waveshare 2-inch 240 × 320 SPI module, SKU 17344 |
| SD1 and SD2 | 2 | microSD cards, one inserted directly in each camera carrier; use a format supported by the firmware |

## Controls and electronics

| Part | Quantity | Value or role |
| --- | ---: | --- |
| U3 | 1 | 74HC595N 16-pin output register |
| SW1 and SW2 | 2 | Normally open momentary buttons: shutter and camera selector |
| D1 and D5 | 2 | Red LEDs: rear status and outward photo status |
| D2, D3, D4 | 3 | One yellow, one green, one blue LED |
| R1–R5 | 5 | 1 kiloohm, one in series with each LED |
| R6, R7, R10 | 3 | 10 kiloohms: two button pull-ups and the U3 latch pull-up |
| C1 | 1 | 0.1 microfarad ceramic capacitor across U3 supply pins |

## Mounting and wiring

| Part | Quantity | Specification or role |
| --- | ---: | --- |
| Full-size breadboards | 2 | Approximately 830 contact points each |
| Half-size breadboard | 1 | Approximately 400 contact points; supports U1 upright |
| LCD adapter leads | 8 | Match the display's eight-pin connector and terminate in separate breadboard pins |
| Insulated jumper wires | As needed | Short breadboard wires, including two labeled conductors for D5 across the enclosure |
| Nonconductive supports | As needed | Prevent exposed contacts and boards from touching the printed case or each other |

## Power

| Part | Quantity | Specification or role |
| --- | ---: | --- |
| PWR1 | 1 | Anker Nano A1653 5,000 mAh bank with built-in USB-C plug and separate USB-C port |
| U1 power cable | 1 | Short cable from PWR1's port to U1's actual USB socket |
| U4 adapter | 0 or 1 | USB-C female to Micro-USB male power adapter only if U4 has a Micro-USB socket |

Power U1 and U4 from separate USB outputs. Do not connect their 3.3-volt rails. Check the actual camera-carrier sockets before selecting a cable or adapter.

## Printed enclosure and fasteners

| Part | Quantity | Specification or role |
| --- | ---: | --- |
| Case body | 1 | Print `case/printable/CamBuildCase.stl` (supplied V1.3) |
| Front | 1 | Print `case/printable/CamBuildFront.stl` (supplied V1.3) |
| Lid | 1 | Print `case/printable/CamBuildLid.stl` (supplied V1.2) |
| M3 machine screws | 5 | Two for the Front, three for the Lid; choose length after checking the printed joint and nut engagement |
| Standard M3 hex nuts | 5 | One nut per screw, seated on the inner side of each Case ear |
| Rigid FDM filament | Enough for 3 parts | PETG or matte PLA, according to printer capability and required heat tolerance |

The assembly guide measures 8 mm of plastic at each Front joint and 9 mm at each Lid joint. Screw length must also cover the chosen nut and a small amount of full thread engagement. Do not force a long screw into electronics or a short screw into a partly engaged nut. The three supplied STL revisions have not been physically fit-checked together.

## Excluded from this build

The retired 74HC165 (U2), MCP23017, IMU, battery gauge, AA holder, booster, record button and standby button are not required.
