# CamBuild active wiring table

This table contains every external electrical connection in the final two-button build. The cameras and microSD sockets are already wired internally on their carrier boards.

| Group | Start | Through | End | Reason |
| --- | --- | --- | --- | --- |
| SW1 | U1 3.3 volts | R6, 10 kiloohms | U1 GPIO32 | Holds shutter input high while released |
| SW1 | U1 GPIO32 | SW1 | U1 ground | Pulls shutter input low while pressed |
| SW2 | U1 3.3 volts | R7, 10 kiloohms | U1 GPIO33 | Holds selector input high while released |
| SW2 | U1 GPIO33 | SW2 | U1 ground | Pulls selector input low while pressed |
| U3 power | U4 3.3 volts | Direct wire | U3 pin 16 | Powers U3 |
| U3 reset | U4 3.3 volts | Direct wire | U3 pin 10 | Keeps U3 out of reset |
| U3 ground | U4 ground | Direct wire | U3 pin 8 | Ground return |
| U3 enable | U4 ground | Direct wire | U3 pin 13 | Enables U3 outputs |
| U3 bypass | U3 pin 16 | C1, 0.1 microfarad | U3 pin 8 | Filters supply disturbances |
| U3 startup | U4 3.3 volts | R10, 10 kiloohms | U3 pin 12 and U4 GPIO3 | Holds latch high during startup |
| U3 data | U4 GPIO32 | Direct wire | U3 pin 14 | Sends the eight output bits |
| U3 shift clock | U4 GPIO1, transmit | Direct wire | U3 pin 11 | Moves data bits into U3 |
| U3 latch | U4 GPIO3, receive | Direct wire | U3 pin 12 | Updates all U3 outputs together |
| D1 red | U3 pin 15 | R1, 1 kiloohm, then D1 | U4 ground | Red status output |
| D2 yellow | U3 pin 1 | R2, 1 kiloohm, then D2 | U4 ground | Warning output |
| D3 green | U3 pin 2 | R3, 1 kiloohm, then D3 | U4 ground | Ready output |
| D4 blue | U3 pin 3 | R4, 1 kiloohm, then D4 | U4 ground | Feedback output |
| D5 outward red | U3 pin 4 | R5, 1 kiloohm, then two cross-case wires and D5 | U4 ground | Photo-in-progress output |
| LCD power | U4 3.3 volts | Direct wire | LCD1 VCC | Powers display electronics |
| LCD ground | U4 ground | Direct wire | LCD1 GND | Display ground return |
| LCD data | U4 GPIO32 | Direct wire | LCD1 DIN | Sends commands and pixels |
| LCD clock | U4 GPIO33 | Direct wire | LCD1 CLK | Clocks display data |
| LCD selection | U3 pin 5, output Q5 | Direct wire | LCD1 CS | Firmware selects and releases the display around every transfer |
| LCD backlight | U4 3.3 volts | Direct wire | LCD1 BL | Holds the onboard backlight switch fully on |
| LCD mode | U4 GPIO13 | Direct wire | LCD1 DC | Selects command or pixel data directly, without depending on U3 |
| LCD reset | U3 pin 6, output Q6 | Direct wire | LCD1 RST | Gives the powered display its own deliberate low reset pulse |

U3 pin 7, output Q7, and pin 9, serial output Q7S, are unused. U4 GPIO0 and the boot button are unused. U4 GPIO25, GPIO26 and GPIO27 belong to the camera and must not receive LCD wires. U1 and U4 communicate over local Wi-Fi; there is no inter-board data jumper.
