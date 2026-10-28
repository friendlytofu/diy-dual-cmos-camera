# TofuCam local controller API

U4 serves both the page and this API on the local `TOFUCAM` network. The current build has no website login or session cookie. U1's endpoints are used by U4 on the same camera network.

## Status

`GET /api/status`

```json
{
  "activeCamera":"outward",
  "connected":{"outward":true,"selfie":true},
  "settings":{"vflip":false,"quality":20,"framesize":"qqvga"},
  "storage":{"outward":"ready","selfie":"ready"},
  "physicalCaptureRevision":3,
  "lastPhysicalCamera":"outward"
}
```

## Camera selection

`POST /api/camera` with `{"camera":"outward"}` or `{"camera":"selfie"}`. U4 records the selection and forwards later capture/control requests to the selected board.

## Optional diagnostic frame

`GET /stream?camera=outward` returns one current JPEG for diagnostics or a future client. The normal TofuCam website does not request footage or current frames. Its interface reads only the two microSD libraries.

## Capture

`GET /api/capture?camera=outward` temporarily switches the selected OV2640 to UXGA 1600 × 1200 and JPEG quality 8, saves that JPEG to the selected camera's card when the card is available, returns the same JPEG with `Cache-Control: no-store`, and then restores the preview settings. The browser applies the selected 0°/90°/180°/270° rotation before adding a copy to its session gallery.

`GET /api/latest?camera=outward` returns the last photograph made by the physical shutter so the open website can offer a quick download. Video recording and standby endpoints are not part of the final two-button design.

## Card library

`GET /api/files?camera=outward` returns the saved JPEG names and byte sizes from the selected card. Use `camera=selfie` for U4's card. Each board exposes at most 400 matching photographs in one response and reports `"truncated":true` if more are present.

```json
{"camera":"outward","files":[{"name":"OUT_12.JPG","size":284901}],"truncated":false}
```

`GET /api/file?camera=outward&name=OUT_12.JPG` downloads one original JPEG. Add `inline=1` only for an on-demand browser preview. U4 streams outward files from U1 to the browser through an 8-kilobyte buffer, retries partial network writes, and pauses LCD drawing during the transfer. Filenames are strictly limited to `OUT_<number>.JPG` and `SELF_<number>.JPG`.

The website has no live snapshot loop. Card listing and full-resolution transfers therefore do not compete with continuous browser image requests.

## Sensor control

`POST /api/control`

```json
{"camera":"outward","setting":"vflip","value":1}
```

Support `vflip`, `framesize`, and `quality`. `framesize` and `quality` control the camera feed used by LCD1; saved photographs always use UXGA and the fixed still-image quality. Validate every value on U4 and again on the selected camera board.
