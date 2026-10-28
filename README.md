# TofuCam

A two-camera ESP32-WROVER-E and OV2640 handheld camera prototype. It uses one microSD card per camera, a Waveshare 2-inch LCD for local preview, two buttons, five status LEDs, and an Anker Nano A1653 power bank. The device-hosted website lists and downloads saved photographs from both cards.

## Repository map

| Category | Contents |
| --- | --- |
| `components/` | Full bill of materials, grouped by electronics, wiring, power, and printed hardware |
| `design/` | Active pin labels, connection sequence, wiring table, technical plan, editable diagrams.net drawing |
| `case/printable/` | Three supplied STL files |
| `case/assembly/` | Six-page assembly PDF with five labeled M3 joints |
| `firmware/final/` | Matching U1 outward-camera and U4 selfie/controller Arduino sketches, startup animation, sample network configuration |
| `web/` | TofuCam page source, device API notes, and the script that rebuilds its embedded header |

## Build order

1. Check the [bill of materials](components/BILL_OF_MATERIALS.md). Print the three STL parts and read [the printable-part notes](case/PRINTABLE_PARTS.md) and [assembly guide](case/assembly/CamBuild_Assembly_Instructions_Updated.pdf).
2. Wire the components using the [connection sequence](design/DETAILED_CONNECTION_SEQUENCE.md), [connection table](design/WIRING_CONNECTION_TABLE.md), and [editable diagram](design/CamBuild_final_wiring.drawio). Labels U1, U3, U4, LCD1, and PWR1 are defined in [hardware labels](design/HARDWARE_LABELS.md).
3. Follow the [firmware upload guide](firmware/final/UPLOAD_AND_FINISH.md). Copy each `config.example.h` to `config.h` and set the same private Wi-Fi name and password in both copies. `config.h` is excluded from this package and must remain private.
4. Place `firmware/final/u4_controller/sdcard/opening.cbs` at the root of U4's microSD card. Insert both cards and upload U4 first, then U1.
5. Connect to U4's configured Wi-Fi network and visit `http://192.168.4.1/` to browse and download saved photographs.

The camera boards and LEDs were checked separately, and the LCD has displayed. The latest combined wiring, mixed-revision case fit, five M3 fasteners, sustained power behavior, full-resolution downloads, and current preview performance still require confirmation on the assembled device. The supplied Case and Front are V1.3; the Lid is V1.2. The STL files are print meshes rather than editable Fusion source.

The example Wi-Fi name is generic and the password is a placeholder. This package contains no operating credentials, captured photographs, personal media, or local machine paths.
