# CamBuild final two-board firmware

This is the active firmware for the complete five-LED, two-button build.

| Board | Upload file | Responsibilities |
| --- | --- | --- |
| U1 outward camera | `u1_outward/u1_outward.ino` | Outward OV2640, SD1, SW1 shutter, SW2 camera selector, and the private frame/capture service used by U4. |
| U4 selfie/controller | `u4_controller/u4_controller.ino` | Selfie OV2640, SD2, LCD1, U3 and D1–D5, active-camera state, opening animation, and the no-login TofuCam photo shelf. |

## Implemented operation

- Startup runs the red, yellow, green, blue LED sequence, initializes LCD1, and plays the centered opening animation once before starting the live view. Green indicates the controller camera is ready. Yellow additionally indicates that SD2 is unavailable.
- LCD1 starts in outward-camera mode and shows only the active camera image. SW2 changes the active-camera state. The browser is now a saved-photo shelf and does not control or display live footage.
- LCD pixels use a fast direct-GPIO transfer on the proven GPIO32/GPIO33 wiring. The display update keeps chip select active for an entire decoded JPEG block rather than updating U3 around every command. The refresh scheduler allows up to 20 frames per second; actual rate depends on capture, JPEG decoding and the extra Wi-Fi hop for U1.
- Both live views default to QVGA 320 × 240. U4 fills the LCD with the outward view and rotates the sideways selfie image into a 180 × 240 portrait view with 70-pixel black borders on the left and right. Every shutter capture temporarily switches the selected OV2640 to maximum UXGA 1600 × 1200 and JPEG quality 8, writes the full-resolution file, then restores the fast preview settings. In the ESP32 camera quality scale, a lower number means less compression.
- LCD1 uses ST7789 inversion-on mode, RGB panel order and the established landscape direction. U4 sends RGB565 pixels unchanged. The final six-colour hardware result showed that RGB corrected the channel order and INVON corrected the remaining pure complementary palette. The optimized GPIO transfer includes explicit data-setup and clock-hold pauses for reliable breadboard signals. The outward OV2640 is intentionally mirrored at its sensor. Because the selfie module is mounted sideways, U4 rotates its 320 × 240 preview 90 degrees counterclockwise and fits it into a 180 × 240 portrait view with black side borders. Saved selfie files retain their original sensor orientation.
- SW1 takes a photograph with the selected camera. Outward photographs are written to SD1 as `OUT_n.JPG`; selfies are written to SD2 as `SELF_n.JPG`. D5 red lights during the capture and blue gives brief feedback afterward.
- A normal SW2 press still switches between the outward and selfie cameras. Holding SW2 for at least two seconds also replays the opening animation once; it does not change the selected camera a second time.
- The local TofuCam website at `http://192.168.4.1/` opens directly and lists saved front and selfie photographs. It does not fetch every original while building the gallery: a preview is loaded only when pressed. Selected originals download one at a time. During a file transfer U4 turns the LCD controller off, restores it afterward and resumes live view; the hard-wired backlight may remain faintly lit. Wi-Fi sleep is disabled on both boards for lower transfer latency. The page intentionally has no date filter, live browser footage or browser camera controls.
- There is no record button, standby button, video recorder, battery gauge, MCP23017, 74HC165, or IMU in this firmware.
- The cameras communicate only through U4's local Wi-Fi access point. Cloudflare and GitHub hosting are not needed while operating the camera.

## Required Arduino setup

1. Install Espressif ESP32 core 3.3.3 or a compatible release.
2. Install `JPEGDEC` by Larry Bank. Version 1.8.4 was used for the verified compilation.
3. In each board folder, copy `config.example.h` to `config.h`, then give both private files exactly the same network name and password. Do not publish `config.h`.
4. Select **ESP32 Wrover Module**, enable PSRAM, and use the partition setting that provides at least 1.3 MB for the application.
5. Copy `u4_controller/sdcard/opening.cbs` to the root of SD2, so the card contains `/opening.cbs`. Keep this file on U4's card; U1 does not use it.
6. Upload both updated sketches: U4 first, then U1. The SW2 hold event requires the matching U1 and U4 versions. GPIO1 and GPIO3 operate U3 after boot, so the Serial Monitor is not part of normal U4 operation.
7. Connect a phone or computer to the private network configured in both `config.h` files, then open `http://192.168.4.1/`. The website does not have a second login prompt.

`config.h` remains ignored so the operating credentials stay private. See `UPLOAD_AND_FINISH.md` for the complete upload and completion sequence.

## Fixed active wiring used by this firmware

| Connection | Final assignment |
| --- | --- |
| SW1 shutter signal | U1 GPIO32, with R6 10-kiloohm pull-up to U1 3.3 volts; button to U1 ground |
| SW2 selector signal | U1 GPIO33, with R7 10-kiloohm pull-up to U1 3.3 volts; button to U1 ground |
| U3 serial data | U4 GPIO32 |
| U3 shift clock | U4 GPIO1, marked TX |
| U3 storage latch | U4 GPIO3, marked RX, with R10 10-kiloohm pull-up to U4 3.3 volts |
| LCD1 DIN | U4 GPIO32, shared only with U3 serial data |
| LCD1 CLK | U4 GPIO33 |
| LCD1 CS | U3 pin 5, output Q5 |
| LCD1 BL | U4 3.3 volts |
| LCD1 DC | U4 GPIO13, available because SD2 uses one-bit mode |
| LCD1 RST | U3 pin 6, output Q6; do not use U4 EN/RST or the boot button |

Never connect LCD1 to U4 GPIO25, GPIO26, or GPIO27. Those pins are already used by the OV2640. Keep the two USB power outputs and the two boards' 3.3-volt rails separate.

## Build verification

The isolated GPIO-only test produced its color sequence, proving LCD1, its 3.3-volt supply and the GPIO32 data, GPIO33 clock and GPIO13 command/data paths. The final integrated firmware gives LCD1 explicit chip-select and reset signals through U3 Q5/Q6. The opening animation replaces the earlier startup color fills and uses the same display path; missing SD2 or `/opening.cbs` does not block normal camera startup. The user confirmed that the display operated, but the original conservative per-bit transfer produced an impractically slow live view. The fast-transfer, split preview/still-resolution and animation revisions keep the same wiring; the combined result still requires an on-device run.
