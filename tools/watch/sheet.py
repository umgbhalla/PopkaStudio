#!/usr/bin/env python3
"""sheet.py — screenshots of the round watch side by side, each cut to its circle (what the panel really shows).

    python3 tools/watch/sheet.py OUT.png A.png B.png …   [--guides: the middle (red) and the optical centre (green)]
"""
import sys

from PIL import Image, ImageDraw

args = [a for a in sys.argv[1:] if not a.startswith("--")]
if len(args) < 2:
    sys.exit(__doc__)
out, files = args[0], args[1:]
cols = min(4, len(files))
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (476 * cols, 476 * rows), (60, 60, 60))
mask = Image.new("L", (466, 466), 0)
ImageDraw.Draw(mask).ellipse((0, 0, 465, 465), fill=255)
for i, f in enumerate(files):
    im = Image.open(f).convert("RGB").resize((466, 466))
    if "--guides" in sys.argv:
        d = ImageDraw.Draw(im)
        d.line((0, 233, 466, 233), fill=(255, 0, 0))
        d.line((0, 217, 466, 217), fill=(0, 200, 0))
    cell = Image.new("RGB", (466, 466), (28, 28, 28))
    cell.paste(im, (0, 0), mask)
    sheet.paste(cell, ((i % cols) * 476, (i // cols) * 476))
sheet.save(out)
print(out)
