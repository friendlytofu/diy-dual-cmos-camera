# TofuCam microSD photo shelf

The device-hosted TofuCam website has one purpose: browse and download photographs already saved by the front and selfie cameras. It contains no footage, shutter, rotation, flip, grid, prompt, or camera-setting controls.

## Open the design preview

Open `camera/index.html` in a browser. Local-file mode shows representative front and selfie card entries without contacting an ESP32.

Run `python3 web/tools/build_embed.py` from the project root to produce the self-contained `web/dist/index.html` and the compressed firmware header used by U4. The generated page has no external fonts, scripts, images, or network dependencies.

## Current features

- separate front and selfie microSD galleries
- filename and file size without automatically transferring every original
- on-demand photograph previews
- selection across both cards
- select-all, refresh, individual download, and download-selected controls
- responsive phone, tablet, and desktop layout
- direct local access without a website password

Selected originals transfer one at a time. U4 pauses LCD drawing and turns the LCD controller off during each transfer so display work does not compete with the microSD and Wi-Fi path. The backlight is wired to 3.3 volts and can remain faintly lit.

## Hosting decision

Keep the operational interface on U4. GitHub is useful for publishing source and instructions, but GitHub Pages or Cloudflare Pages is not required for stable local card access. A public HTTPS page cannot reliably reach an ESP32 on a private LAN across browsers. The `TOFUCAM` network remains password-protected.

Status: the simplified TofuCam page and dual-card API are embedded in the final U4 firmware. LCD1 live view, camera operation, physical controls, LEDs, full-resolution capture, and the LCD opening animation remain firmware functions outside the website. The complete card gallery and downloads still require an integrated on-device check.
