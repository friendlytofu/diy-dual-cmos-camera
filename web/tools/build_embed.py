#!/usr/bin/env python3
"""Build the self-contained page and a compressed U4 firmware header."""

import gzip
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "camera"
OUTPUT = ROOT / "dist" / "index.html"
HEADER = ROOT.parent / "firmware" / "final" / "u4_controller" / "camera_web.h"


def main() -> None:
    html = (SOURCE / "index.html").read_text(encoding="utf-8")
    css = (SOURCE / "styles.css").read_text(encoding="utf-8")
    js = (SOURCE / "app.js").read_text(encoding="utf-8")
    html = html.replace('<link rel="stylesheet" href="styles.css">', f"<style>\n{css}\n</style>")
    html = html.replace('<script src="app.js"></script>', f"<script>\n{js}\n</script>")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(html, encoding="utf-8")
    compressed = gzip.compress(html.encode("utf-8"), compresslevel=9, mtime=0)
    HEADER.parent.mkdir(parents=True, exist_ok=True)
    rows = [", ".join(f"0x{byte:02x}" for byte in compressed[i:i + 16])
            for i in range(0, len(compressed), 16)]
    body = ",\n  ".join(rows)
    HEADER.write_text(
        "#pragma once\n#include <Arduino.h>\n\n"
        "const uint8_t CAMERA_WEB_GZIP[] PROGMEM = {\n  " + body + "\n};\n"
        f"constexpr size_t CAMERA_WEB_GZIP_SIZE = {len(compressed)};\n",
        encoding="utf-8",
    )
    print(f"Built {OUTPUT.relative_to(ROOT)} ({OUTPUT.stat().st_size} bytes)")
    print(f"Built {HEADER.relative_to(ROOT.parent)} ({len(compressed)} compressed bytes)")


if __name__ == "__main__":
    main()
