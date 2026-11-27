#!/usr/bin/env python3
"""flash.py — build a project's firmware and flash it to the watch, without the studio's window.

    .venv/bin/python tools/watch/flash.py [PROJECT.json] [--build-only] [--reset]

PROJECT.json: a studio project (default: projects/kolkata-full-watch.json — the user's watch; else the board's default
project). studio.py's write_sketch writes the sketch (layers.h, config.h, wifi_net.h, fonts) into build/watch-flash/,
arduino-cli compiles it with the board's passport (fqbn, defines, partitions) and uploads it, then esptool resets the
chip by its watchdog: the RTS reset over the S3's own USB can leave it in download mode (a dark screen).
--reset alone: only that reset (a watch stuck dark after a flash or a stray DTR/RTS).
"""
import glob
import json
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "studio"))
CLI = os.path.join(ROOT, ".tools", "bin", "arduino-cli")
ESPTOOL = next(iter(glob.glob(os.path.join(ROOT, ".tools", "arduino-s3", "toolchain", "esptool*", "esptool"))), "esptool")
ENV = {**os.environ, "ARDUINO_DIRECTORIES_USER": os.path.join(ROOT, ".tools", "arduino-s3")}  # (the ESP32 core 3.3.11)


def port():
    p = os.environ.get("PORT") or next(iter(sorted(glob.glob("/dev/cu.usbmodem*"))), None)
    if not p:
        sys.exit("flash.py: no watch on USB (/dev/cu.usbmodem*)")
    busy = subprocess.run(["lsof", p], capture_output=True, text=True).stdout.strip()
    if busy:
        sys.exit(f"flash.py: {p} is open in another program — agree on the port first:\n{busy}")
    return p


def reset():
    r = subprocess.run([ESPTOOL, "--chip", "esp32s3", "-p", port(), "--after", "watchdog-reset", "read_mac"],
                       capture_output=True, text=True)
    print("reset:", "ok" if r.returncode == 0 else r.stdout[-400:] + r.stderr[-400:])


def main(args):
    if "--reset" in args and len(args) == 1:
        return reset()
    os.chdir(ROOT)
    import studio as s  # noqa: E402 — studio.py as a library: no server starts

    path = next((a for a in args if a.endswith(".json")), "projects/kolkata-full-watch.json")
    project = json.load(open(path)) if os.path.exists(path) else s.default_project()
    board = project.get("board", "amoled175")
    sketch, out = os.path.join(ROOT, "build", "watch-flash", "PopkaWatch"), os.path.join(ROOT, "build", "watch-flash-out")
    os.makedirs(sketch, exist_ok=True)
    s.write_sketch(project, sketch)
    p = s.passport(board)
    print(f"build: {os.path.basename(path) if os.path.exists(path) else 'default project'} for {board}")
    r = subprocess.run([CLI, "compile", "--fqbn", p["fqbn"], *s.build_defines(board, None), *s.own_partitions(board),
                        "--libraries", os.path.join(ROOT, "lib"), "--build-path", out, sketch],
                       capture_output=True, text=True, env=ENV)
    if r.returncode:
        sys.exit("\n".join(l for l in (r.stdout + r.stderr).splitlines() if "error" in l.lower())[:4000] or r.stderr[-4000:])
    print("build:", *re.findall(r"uses \d+ bytes \(\d+%\)", r.stdout)[:1])
    if "--build-only" in args:
        return
    r = subprocess.run([CLI, "upload", "--fqbn", p["fqbn"], "-p", port(), "--input-dir", out, sketch],
                       capture_output=True, text=True, env=ENV)
    if r.returncode or "Hash of data verified" not in r.stdout + r.stderr:
        sys.exit("flash: failed\n" + (r.stdout + r.stderr)[-2000:])
    print("flash: verified")
    reset()


if __name__ == "__main__":
    main(sys.argv[1:])
