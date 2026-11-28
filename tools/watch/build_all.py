#!/usr/bin/env python3
"""build_all.py — compile the firmware for every board (or the ones named), two at a time, the way the editor builds it.

    .venv/bin/python tools/watch/build_all.py              # all five boards
    .venv/bin/python tools/watch/build_all.py lcd4b p4lcd7

Each board: studio.py write_sketch(default_project(board)) into /tmp/allb/BOARD, then arduino-cli with the passport's
fqbn, its defines and its own partition table (the raw firmware/PopkaWatch folder does not build on the rectangular
boards). The core and its tools live in .tools/arduino-s3 (hardware/esp32/esp32/platform.local.txt points at them:
Xtensa + S3 libs, RISC-V + P4 (pre-v3, esp32p4_es) libs, esptool).
"""
import concurrent.futures as cf
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "studio"))
import studio as S  # noqa: E402

os.environ["ARDUINO_DIRECTORIES_USER"] = os.path.join(ROOT, ".tools", "arduino-s3")
only = sys.argv[1:]


def build(b):
    name = b["id"]
    if only and name not in only:
        return name, None
    out = f"/tmp/allb/{name}/PopkaWatch"
    os.makedirs(out, exist_ok=True)
    S.write_sketch(S.default_project(name), out)
    p = S.passport(name)
    cmd = [os.path.join(ROOT, ".tools", "bin", "arduino-cli"), "compile", "--fqbn", p["fqbn"], *S.build_defines(name),
           *S.own_partitions(name), "--libraries", os.path.join(ROOT, "lib"), "--build-path", f"/tmp/allb/{name}/build", out]
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    lines = (r.stdout + r.stderr).splitlines()
    keep = [l for l in lines if "error" in l.lower()][:6] or [l for l in lines if "Sketch uses" in l]
    return name, ("OK   " if r.returncode == 0 else "FAIL ") + " | ".join(keep)


failed = False
with cf.ThreadPoolExecutor(2) as ex:
    for n, res in ex.map(build, S.BOARDS):
        if res:
            print(f"{n:12} {res}", flush=True)
            failed |= res.startswith("FAIL")
sys.exit(1 if failed else 0)
