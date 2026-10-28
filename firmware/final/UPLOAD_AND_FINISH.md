# Upload and finish TofuCam

This guide uploads the matching final programs to both ESP32-WROVER-E camera boards and completes the current five-light, two-button build.

## Before uploading

1. Install Arduino IDE 2.
2. In **Boards Manager**, install **esp32 by Espressif Systems**. The final build was compiled with version 3.3.3.
3. In **Library Manager**, install **JPEGDEC by Larry Bank**. Version 1.8.4 was used for the final build.
4. Confirm that these private files already exist:
   - `u1_outward/config.h`
   - `u4_controller/config.h`
5. In both files, use exactly the same Wi-Fi network name and password. Keep `config.h` private; do not publish it.
6. Prepare two microSD cards. FAT32 is the recommended format, although the complete card setup is not yet hardware-verified.
7. Copy `u4_controller/sdcard/opening.cbs` to the top level of U4's card. Its final path must be `/opening.cbs`.

## Arduino settings for both boards

Use these settings for U1 and U4:

| Arduino menu | Setting |
| --- | --- |
| Board | ESP32 Wrover Module |
| PSRAM | Enabled |
| Upload speed | 115200 for the most reliable first upload |
| Flash frequency | 40 MHz |
| Partition scheme | Any option providing at least 1.3 MB for the application; Huge APP is suitable |

## Upload U4 first

U4 is the selfie-camera board and main controller. It runs LCD1, the lights, the opening animation and the TofuCam photo shelf.

1. Turn off the Anker power bank and disconnect its USB cables from both boards.
2. Connect only U4 to the computer with a USB data cable.
3. Open `u4_controller/u4_controller.ino` in Arduino IDE.
4. Select **ESP32 Wrover Module** and the USB port that appeared when U4 was connected.
5. Click **Verify**. A successful build confirms that `config.h`, `camera_web.h` and JPEGDEC are available.
6. Click **Upload**. Normally, the BOOT button is not needed.
7. If Arduino remains at `Connecting...`, hold U4's **BOOT** button, start Upload, and release BOOT when `Writing at...` appears.
8. Wait for `Hard resetting via RTS pin...` or the upload-complete message.
9. Disconnect the computer USB cable before reconnecting battery power.

U4 GPIO1 and GPIO3 also drive U3 after startup. If uploading fails while the complete circuit is attached, turn all power off, temporarily remove only the GPIO1-to-U3-clock and GPIO3-to-U3-latch wires, upload U4, turn power off again, and restore those two wires to their labeled connections.

## Upload U1 second

U1 is the outward-camera board. It reads the shutter and camera-selector buttons, saves outward photographs and sends outward images to U4 over the private local Wi-Fi connection.

1. Keep the Anker power bank disconnected.
2. Connect only U1 to the computer with a USB data cable.
3. Open `u1_outward/u1_outward.ino` in Arduino IDE.
4. Use the same board, PSRAM, flash and partition settings used for U4.
5. Select U1's USB port, click **Verify**, and then click **Upload**.
6. If it remains at `Connecting...`, use the same BOOT-button method described for U4.
7. Disconnect the computer USB cable after the upload completes.

Do not power either board from the computer and the Anker power bank at the same time during this procedure.

## Complete the physical build

1. Insert the first microSD card into U1's built-in slot. U1 stores outward photographs as `OUT_n.JPG`.
2. Insert the card containing `/opening.cbs` into U4's built-in slot. U4 stores selfies as `SELF_n.JPG`.
3. Compare every connection with `design/FINAL_WIRING_PLAN.md`. Do not use the superseded diagrams in `reference/`.
4. Confirm the two-button arrangement:
   - SW1 connects to U1 GPIO32 with R6, a 10-kiloohm pull-up resistor to U1 3.3 volts. The other button terminal connects to U1 ground.
   - SW2 connects to U1 GPIO33 with R7, a 10-kiloohm pull-up resistor to U1 3.3 volts. The other button terminal connects to U1 ground.
5. Confirm LCD1:
   - DIN to U4 GPIO32
   - CLK to U4 GPIO33
   - DC to U4 GPIO13
   - CS to U3 output Q5, U3 pin 5
   - RST to U3 output Q6, U3 pin 6
   - VCC and BL to U4 3.3 volts
   - GND to U4 ground
6. Keep U1 and U4 3.3-volt rails separate. Do not connect the two Anker USB outputs together.
7. Connect one Anker USB output to U4 and the other USB output to U1. Start U4 first, then U1 a few seconds later if the outputs do not start simultaneously.

## Normal finished operation

- U4 runs the red-yellow-green-blue light sequence and then plays the LCD opening animation.
- Green means the controller is ready. Yellow also indicates that U4 could not use its microSD card.
- Press SW2 normally to switch the active camera shown on LCD1.
- Hold SW2 for two seconds to replay the opening animation. The hold does not perform a second camera switch.
- Press SW1 to save a maximum-resolution photograph from the selected camera. The outward red photo light indicates capture; blue gives brief feedback afterward.
- Connect a phone or laptop to the private Wi-Fi network configured in both `config.h` files.
- Open `http://192.168.4.1/` to use TofuCam. The page lists the saved **Front** and **Selfie** photographs from their separate cards and downloads the selected originals. It intentionally has no live browser view and no second password prompt.
- Photograph tiles initially show only names and file sizes. Press **Load preview** only for photographs you want to inspect. **Download selected** transfers one original at a time and reports progress. LCD1 turns off during each transfer and resumes afterward; its separately powered backlight can remain faintly lit.

The final combined assembly, microSD behavior, sustained two-board power and enclosure thermal behavior remain to be confirmed on the completed hardware.
