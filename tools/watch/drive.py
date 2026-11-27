#!/usr/bin/env python3
"""drive.py — drive the watch on USB from the command line: send commands, wait, take screenshots of the real panel.

    .venv/bin/python tools/watch/drive.py STEP...

Steps, in order:
    c:CMD        a line to the watch's serial console ("wake", "back", "tap 233 360", "drag 233 300 233 40",
                 "boot long" — unlock, "upcheck" — the paged apps' overlap audit, "sys", …; PopkaWatch.ino lists them)
    w:SECONDS    wait, printing what the watch logs meanwhile
    s:FILE.png   a screenshot of the panel (the firmware's `shot` command: RGB565 over serial)

Example — open Weather from the face and capture it:
    tools/watch/drive.py c:wake w:1 "c:tap 233 360" w:3 s:/tmp/wx.png

The port: the first /dev/cu.usbmodem* (or PORT=…). Opening it does not reset the board; never toggle DTR/RTS by hand —
that puts the ESP32-S3 into download mode (a dark screen; tools/watch/flash.py --reset brings it back).
"""
import glob
import os
import struct
import sys
import time

import serial
from PIL import Image


def port():
    p = os.environ.get("PORT") or next(iter(sorted(glob.glob("/dev/cu.usbmodem*"))), None)
    if not p:
        sys.exit("drive.py: no watch on USB (/dev/cu.usbmodem*)")
    return p


def main(steps):
    s = serial.Serial(port(), 115200, timeout=0.2)
    time.sleep(0.3)
    s.reset_input_buffer()
    log = b""

    def pump(sec):
        nonlocal log
        t = time.time()
        while time.time() - t < sec:
            log += s.read(65536)

    def flush():
        nonlocal log
        if log:
            print(log.decode(errors="replace"), end="")
        log = b""

    def shot(path):
        pump(0.05)
        flush()
        s.write(b"shot\n")
        buf, t0 = b"", time.time()
        while b"SHOT END" not in buf and time.time() - t0 < 15:
            buf += s.read(65536)
        i = buf.find(b"SHOT ")
        if i < 0:
            sys.exit("drive.py: no picture came back (is the watch locked or asleep? try c:wake / \"c:boot long\")")
        e = buf.index(b"\n", i)
        w, h = map(int, buf[i + 5:e].split())
        d = buf[e + 1:e + 1 + w * h * 2]
        while len(d) < w * h * 2:
            d += s.read(w * h * 2 - len(d))
        px = struct.unpack("<%dH" % (w * h), d)
        im = Image.new("RGB", (w, h))
        im.putdata([((p >> 11 & 31) * 255 // 31, (p >> 5 & 63) * 255 // 63, (p & 31) * 255 // 31) for p in px])
        im.save(path)
        print(f"[shot {path}]")

    for st in steps:
        k, _, v = st.partition(":")
        if k == "c":
            s.write((v + "\n").encode())
            pump(0.05)
        elif k == "w":
            pump(float(v))
        elif k == "s":
            shot(v)
        else:
            sys.exit(f"drive.py: unknown step {st!r} (c:CMD, w:SECONDS, s:FILE.png)")
    pump(0.2)
    flush()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    main(sys.argv[1:])
