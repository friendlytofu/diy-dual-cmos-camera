# Active hardware identifiers

Use these IDs on the parts, [wiring plan](FINAL_WIRING_PLAN.md) and firmware. Older files in reference/ may use obsolete assignments.

| ID | Part and role | Physical position |
| --- | --- | --- |
| U1 | Freenove ESP32-WROVER-E, outward OV2640 and built-in SD1 slot; reads SW1–SW2 | Upright 400-point half breadboard attached to outward full board; USB socket up; confirm socket type |
| U2 | Retired 74HC165 button register | **Do not install or wire** |
| U3 | 74HC595 16-pin DIP; Q0–Q4 drive five LEDs, Q5 drives LCD chip select and Q6 drives LCD reset; data from U4 GPIO32, shift clock from GPIO1, update clock from GPIO3/RX | Rear full board, across center gap beside U4 |
| U4 | Freenove ESP32-WROVER-E, selfie OV2640 and built-in SD2 slot; LCD and user interface controller; LCD data on GPIO32, clock on GPIO33 and command/data on GPIO13; boot button unused | Rear full board; selfie lens faces user |
| LCD1 | Waveshare 2-inch 240 × 320 SPI screen | Rear face, insulated mount |
| PWR1 | Anker Nano A1653 power bank | Supported independently; built-in USB-C to U4 only with matching socket or adapter |
| CAB1 | Short cable matched to U1's actual USB socket | PWR1's other USB-C port to U1 socket |
| J1 | Two-conductor D5 harness | Along board edge; U3 Q4/R5 to D5 anode and D5 cathode to U4 ground |

| ID | Control/indicator | Face | Electrical owner |
| --- | --- | --- | --- |
| SW1 | Shared shutter | Outward | U1 GPIO32 direct button |
| SW2 | Outward/selfie selector | Outward | U1 GPIO33 direct button |
| D1 | Red status | Rear | U3 Q0 through R1 |
| D2 | Yellow status | Rear | U3 Q1 through R2 |
| D3 | Green ready | Rear | U3 Q2 through R3 |
| D4 | Blue feedback | Rear | U3 Q3 through R4 |
| D5 | Red outward photo status | Outward | U3 Q4 through R5 and J1 |

| IDs | Value | Exact role |
| --- | --- | --- |
| R1–R5 | **1 kΩ each** | One series current-limiting resistor for D1–D5 |
| R6, R7 | **10 kΩ each** | U1 GPIO32 and GPIO33 pull-ups to U1 3V3 |
| R10 | **10 kΩ** | U4 GPIO3/RX latch pull-up to U4 3V3 |
| C1 | **0.1 µF ceramic** | U3 pin16-to-pin8 local bypass |

SD1/SD2 are inserted directly into their carrier slots; no SD jumper wires are added. U1 and U4 each keep their own 3.3 V and ground nets. No MCP23017, battery-check LEDs/button, IMU, AA holder or regulator is active.
