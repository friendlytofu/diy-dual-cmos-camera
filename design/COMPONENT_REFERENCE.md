# Active component reference

Use this table whenever a wiring instruction mentions a label such as R6. The label identifies one particular part and must stay with that part even if the physical layout changes.

## What the label letters mean

| Label begins with | Meaning |
| --- | --- |
| U | A camera board or integrated-circuit chip |
| R | A resistor |
| C | A capacitor |
| SW | A push button |
| D | A light-emitting diode |
| LCD | The liquid-crystal display |
| SD | A microSD memory card |
| PWR | The power bank |
| CAB | A power cable |
| J | A group of wires connecting the outward and selfie sides |

## Main electronic modules

| Label | Quantity | Exact part or value | Plain-language purpose |
| --- | ---: | --- | --- |
| U1 | 1 | Freenove ESP32-WROVER-E camera board with OV2640 camera | Outward-facing camera. It reads SW1 and SW2 and contains the first microSD card slot. |
| U3 | 1 | 74HC595N, sixteen-leg output-register chip | Controls five lights plus LCD chip select and reset using three controller pins. |
| U4 | 1 | Freenove ESP32-WROVER-E camera board with OV2640 camera | Selfie-facing camera and main controller. It controls the display and the output-register chip. |
| LCD1 | 1 | Waveshare two-inch display, 240 by 320 pixels, serial peripheral interface, product 17344 | Shows the selected camera preview. |
| PWR1 | 1 | Anker Nano A1653 power bank, 5,000 milliamp-hours | Powers both camera boards through two separate Universal Serial Bus outputs. |

## Resistors

| Label | Quantity | Required value | How to identify it | What it does |
| --- | ---: | --- | --- | --- |
| R1 | 1 | 1 kiloohm | A common four-band version is brown, black, red, gold | Limits current through the rear red status light. |
| R2 | 1 | 1 kiloohm | A common four-band version is brown, black, red, gold | Limits current through the rear yellow status light. |
| R3 | 1 | 1 kiloohm | A common four-band version is brown, black, red, gold | Limits current through the rear green ready light. |
| R4 | 1 | 1 kiloohm | A common four-band version is brown, black, red, gold | Limits current through the rear blue feedback light. |
| R5 | 1 | 1 kiloohm | A common four-band version is brown, black, red, gold | Limits current through the outward red photo-status light. |
| R6 | 1 | 10 kiloohms | A common four-band version is brown, black, orange, gold | Holds outward-camera input pin number 32 near 3.3 volts while the shutter button is released. |
| R7 | 1 | 10 kiloohms | A common four-band version is brown, black, orange, gold | Holds outward-camera input pin number 33 near 3.3 volts while the camera-selector button is released. |
| R10 | 1 | 10 kiloohms | A common four-band version is brown, black, orange, gold | Keeps the connection between the selfie/controller board’s pin number 3 and output-register pin 12 near 3.3 volts during startup. |

The active design uses eight resistors: five 1-kiloohm resistors and three 10-kiloohm resistors. None needs adjustment.

## Capacitor

| Label | Quantity | Required value | How to identify it | What it does |
| --- | ---: | --- | --- | --- |
| C1 | 1 | 0.1 microfarad ceramic capacitor | Often printed with the number 104; it has no positive or negative direction | Reduces brief supply-voltage changes at the 74HC595N chip. Connect it between the same 3.3-volt and ground connections used by chip pins 16 and 8. |

## Push buttons

| Label | Quantity | Required type | Function |
| --- | ---: | --- | --- |
| SW1 | 1 | Momentary, normally open push button | Shared shutter button on the outward face. |
| SW2 | 1 | Momentary, normally open push button | Selects the outward or selfie camera. |

Each four-leg button contains two permanently connected legs on one side and two permanently connected legs on the other side. Pressing the button connects the two sides. Place each button across a breadboard center gap.

## Light-emitting diodes

| Label | Quantity | Color | Series resistor | Function |
| --- | ---: | --- | --- | --- |
| D1 | 1 | Red | R1, 1 kiloohm | Rear status light and the first light in the startup sequence. |
| D2 | 1 | Yellow | R2, 1 kiloohm | Rear warning and error-pattern light. |
| D3 | 1 | Green | R3, 1 kiloohm | Rear ready light. |
| D4 | 1 | Blue | R4, 1 kiloohm | Rear feedback light. |
| D5 | 1 | Red | R5, 1 kiloohm | Outward-facing photo-capture status light. |

The longer diode leg is positive. The shorter leg and the flat side of the plastic body are negative.

## Memory cards

| Label | Quantity | Part | Purpose |
| --- | ---: | --- | --- |
| SD1 | 1 | microSD card inserted directly into U1 | Stores outward-camera files. The earlier brief proposed 256 gigabytes, but supported format and capacity remain unverified. |
| SD2 | 1 | microSD card inserted directly into U4 | Stores selfie-camera files. The earlier brief proposed 1 gigabyte, but supported format and capacity remain unverified. |

No separate memory-card module or breadboard wiring is required.

## Breadboards and wiring

| Item | Quantity | Required type | Purpose |
| --- | ---: | --- | --- |
| Half-size breadboard | 1 | About 400 contacts | Holds the outward-camera ESP32 board. |
| Full-size breadboard | 2 | About 830 contacts each | One supports the outward controls; one supports the selfie/controller electronics. |
| J1 | 1 group of 2 wires | Two separately labeled insulated jumper wires | Carries U3 Q4 to the outward D5 anode and returns the D5 cathode to U4 ground. |
| General jumper wires | As needed | Short insulated male-to-male breadboard jumpers | Connect components on each breadboard. |
| Display adapter wires | 8 | Wires that mate with the display connector and end in individual breadboard pins | Connect the display’s eight conductors to the breadboard without cutting its original cable. |
| Insulated supports | 1 set | Nonconductive spacers or bracket | Keep the breadboards, display, and power bank from touching exposed electrical contacts. |

## Power cables

| Label | Quantity | Required type | Purpose |
| --- | ---: | --- | --- |
| CAB1 | 1 | Short cable from Universal Serial Bus Type-C to the actual socket on U1 | Connects the Anker power bank’s separate Type-C port to the outward-camera board. |
| Optional adapter | 0 or 1 | Type-C female to Micro-USB male, with correct Type-C source configuration | Allows the Anker built-in Type-C plug to power U4 only if U4 has a Micro-USB socket. |

Check the actual sockets before buying the cable or adapter. A board marked ESP32-WROVER-E can be installed on different carrier boards with different power sockets.

## Printed enclosure and fasteners

| Item | Quantity | Current specification |
| --- | ---: | --- |
| Printed Case body | 1 | Latest supplied `case/printable/CamBuildCase.stl`, V1.3 |
| Printed Front | 1 | Latest supplied `case/printable/CamBuildFront.stl`, V1.3 |
| Printed Lid | 1 | Latest supplied `case/printable/CamBuildLid.stl`, V1.2 |
| M3 machine screws | 5 | Two Front and three Lid joints; determine length from the printed stack and nut engagement |
| Standard M3 hex nuts | 5 | One per screw on the inner Case ears |

See `case/assembly/CamBuild_Assembly_Instructions_Updated.pdf` for F1–F2 and L1–L3. The mixed STL revisions still need a physical fit check.

## Parts that are not used

| Part | Status |
| --- | --- |
| 74HC165 input-register chip, formerly U2 | Retired. Do not install it. |
| MCP23017 input and output expander | Not part of the design. |
| Motion or balance sensor | Removed from the design. |
| Battery-level button and battery-level lights | Removed from the design. |
| Four-cell battery holder | Replaced by the Anker power bank. |
| Five-volt voltage regulator | Replaced by the Anker power bank. |
| Separate microSD modules | Not needed because both camera boards contain card slots. |

