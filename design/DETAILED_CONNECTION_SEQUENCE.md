# CamBuild connection sequence

This is the active position-independent sequence for five LEDs, two buttons, two cameras, two inserted microSD cards and LCD1. Match printed pin names; wire color and breadboard position do not define a connection.

## 1. Keep the two power systems separate

Power U1 and U4 from separate outputs of PWR1. Do not connect the boards' 5-volt pins, 3.3-volt pins, or local power rails together. U1 ground is used only by U1 and SW1/SW2. U4 ground is used by U3, LCD1, D1–D5 and their resistors.

## 2. Connect the two outward buttons to U1

| Control | Connection |
| --- | --- |
| SW1 shutter | Connect U1 GPIO32 to one electrical side of SW1. Connect the opposite side to U1 ground. Connect R6, 10 kiloohms, from GPIO32 to U1 3.3 volts. |
| SW2 camera selector | Connect U1 GPIO33 to one electrical side of SW2. Connect the opposite side to U1 ground. Connect R7, 10 kiloohms, from GPIO33 to U1 3.3 volts. |

A four-leg tactile button has two legs that are always joined on each side. Use one side as the signal and the opposite side as ground.

## 3. Power and stabilize U3 on the U4 board

Orient U3 by its notch. Connect pin 16 to U4 3.3 volts, pin 8 to U4 ground, pin 10 to U4 3.3 volts, and pin 13 to U4 ground. Put C1, 0.1 microfarad, directly between pins 16 and 8. Connect R10, 10 kiloohms, from U3 pin 12 to U4 3.3 volts.

## 4. Connect U4 control signals to U3

| U4 terminal | U3 terminal |
| --- | --- |
| GPIO32 | Pin 14, serial data input |
| GPIO1, marked transmit | Pin 11, shift clock |
| GPIO3, marked receive | Pin 12, storage latch |

Leave U3 pin 9 unconnected. Do not connect U4's boot button or GPIO0.

## 5. Connect all five LED paths

Each path is one U3 output, one 1-kiloohm resistor, one LED and U4 ground. The LED's longer leg is the anode toward the resistor; the shorter leg and flat side are the cathode toward ground.

| Light | Complete path |
| --- | --- |
| D1 rear red | U3 pin 15 to R1 to D1 anode; D1 cathode to U4 ground |
| D2 rear yellow | U3 pin 1 to R2 to D2 anode; D2 cathode to U4 ground |
| D3 rear green | U3 pin 2 to R3 to D3 anode; D3 cathode to U4 ground |
| D4 rear blue | U3 pin 3 to R4 to D4 anode; D4 cathode to U4 ground |
| D5 outward red | U3 pin 4 to R5 to D5 anode through one insulated cross-case wire; D5 cathode back to U4 ground through a second insulated cross-case wire |

## 6. Connect LCD1 to U4

| LCD1 printed pin | Connect to |
| --- | --- |
| VCC | U4 3.3 volts |
| GND | U4 ground |
| DIN | U4 GPIO32, the same source that feeds U3 pin 14 |
| CLK | U4 GPIO33 |
| CS | U3 pin 5, output Q5 |
| BL | U4 3.3 volts; the module's onboard transistor limits the control current |
| DC | U4 GPIO13; one-bit microSD mode leaves this pin available |
| RST | U3 pin 6, output Q6 |

Leave U3 pin 7 and pin 9 unconnected. Do not connect LCD1 to U4 EN/RST, the boot button, GPIO25, GPIO26 or GPIO27. Use individually identified adapter wires for the LCD connector.

## 7. Insert storage and apply power

Insert SD1 directly into U1 and SD2 directly into U4 while power is off. No external card wiring is needed. Inspect the connections against [FINAL_WIRING_PLAN.md](FINAL_WIRING_PLAN.md), then use the upload order in [the final firmware guide](../firmware/final/README.md).
