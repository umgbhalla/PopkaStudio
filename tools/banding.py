#!/usr/bin/env python3
"""banding.py — find color banding in a screenshot of the watch (a `shot` PNG, or the page engine's render).

Banding is a smooth gradient that the panel shows as a staircase of flat bands with visible steps between them. On a
565 panel the steps are the color levels (R and B 8 apart in 8 bits, G 4 apart); Halo hides them with a 4×4 ordered
dither, so a gradient done right looks smooth once its 4×4 pattern is averaged away, and a banded one does not.

How it measures:
  1. Average every pixel over 4×4 (the dither's period): the dither's texture goes, what the eye sees remains.
  2. Leave out real content: lines, text, hands, ring edges (anything that jumps more than --edge within 5×5) and a
     margin round them, and the black outside the dial.
  3. The smooth trend of what is left: a wide average that only counts the kept pixels (normalized convolution).
  4. The residual (step 1 minus step 3) is the staircase: near 0 on a smooth gradient, a sawtooth of about half a step
     on a banded one. Reported per channel: how many kept pixels are off by more than --tol, the 95th percentile, and
     the bands' typical width (the residual's sign changes along rows and columns).
A heatmap (the screenshot greyed, the bands in red) shows where they are.

  tools/banding.py shot.png [--out heat.png] [--tol 1.2] [--edge 36] [--r 0.96]
Exit code 1 when banded (--fail, the share of off pixels above which it calls it banded: 2 %).
"""
import argparse
import sys

import numpy as np
from PIL import Image


def box(a, k):
    """Mean over a k×k window (same size, edges by the pixels that are there)."""
    pad = k // 2
    p = np.pad(a, ((pad + 1, k - pad), (pad + 1, k - pad)), mode="edge").cumsum(0).cumsum(1)
    s = p[k:, k:] - p[:-k, k:] - p[k:, :-k] + p[:-k, :-k]
    return s[: a.shape[0], : a.shape[1]] / (k * k)


def local_range(a, k):
    """max − min over k×k."""
    pad = k // 2
    p = np.pad(a, pad, mode="edge")
    h, w = a.shape
    mx = np.full(a.shape, -1e9)
    mn = np.full(a.shape, 1e9)
    for dy in range(k):
        for dx in range(k):
            win = p[dy : dy + h, dx : dx + w]
            mx = np.maximum(mx, win)
            mn = np.minimum(mn, win)
    return mx - mn


def dilate(m, k):
    return box(m.astype(float), k) > 0


def band_width(res, keep):
    """The typical length of a run of one sign of the residual along rows and columns, px (the bands' width)."""
    runs = []
    for arr, msk in ((res, keep), (res.T, keep.T)):
        for row, mr in zip(arr, msk):
            s = np.sign(row)
            run, prev = 0, 0
            for v, k in zip(s, mr):
                if not k:
                    if run >= 3:
                        runs.append(run)
                    run, prev = 0, 0
                    continue
                if v == prev or v == 0:
                    run += 1
                else:
                    if run >= 3:
                        runs.append(run)
                    run, prev = 1, v
    return float(np.median(runs)) if runs else 0.0


def analyze(img, tol=1.2, edge=36.0, r_keep=0.96):
    h, w, _ = img.shape
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy, R = (w - 1) / 2, (h - 1) / 2, min(w, h) / 2
    inside = (xx - cx) ** 2 + (yy - cy) ** 2 <= (R * r_keep) ** 2
    lum = img @ np.array([0.299, 0.587, 0.114])
    # Content — lines, text, hands, the edges of flat shapes: a jump within 3×3 bigger than a dither's (one 565 level: 8
    # in R and B, 4 in G — and the dither's neighbors differ by one), in any channel, or a big one in brightness; out,
    # with a margin (a dark flat shape on black jumps less in brightness than its channels do)
    chan = np.max(np.stack([local_range(img[:, :, c].astype(float), 3) for c in range(3)]), 0)
    content = dilate((chan > 24) | (local_range(lum, 5) > edge), 13)
    lit = box(lum, 4) > 6                                  # (black: nothing to band)
    keep = inside & ~content & lit
    out = {"kept_px": int(keep.sum())}
    heat = np.zeros((h, w))
    if keep.sum() < 500:
        return out, heat, keep
    km = keep.astype(float)
    # The trend: a plane fitted (least squares) to the kept pixels of a 31×31 window round each pixel — it follows a
    # gradient's slope exactly, so only what departs from a smooth ramp is left (a plain mean lags on a slope and a
    # curve: an ideal gradient scored 12 % with it)
    K = 31
    X, Y = (xx - cx) / K, (yy - cy) / K
    S = {k: box(v * km, K) for k, v in {"1": 1, "x": X, "y": Y, "xx": X * X, "yy": Y * Y, "xy": X * Y}.items()}
    for ci, name in enumerate("RGB"):
        ch = img[:, :, ci].astype(float)
        seen = box(ch, 7)                                  # what the eye sees: any dither averaged (wider than its texture,
                                                           # narrower than a band)
        sv, sxv, syv = box(seen * km, K), box(seen * km * X, K), box(seen * km * Y, K)
        M = np.stack([np.stack([S["1"], S["x"], S["y"]], -1), np.stack([S["x"], S["xx"], S["xy"]], -1),
                      np.stack([S["y"], S["xy"], S["yy"]], -1)], -2) + np.eye(3) * 1e-6
        coef = np.linalg.solve(M, np.stack([sv, sxv, syv], -1)[..., None])[..., 0]
        trend = coef[..., 0] + coef[..., 1] * X + coef[..., 2] * Y
        res = np.where(keep, seen - trend, 0.0)
        a = np.abs(res[keep])
        out[name] = {"off_pct": round(100.0 * float((a > tol).mean()), 2), "p95": round(float(np.percentile(a, 95)), 2),
                     "band_px": band_width(res, keep)}
        heat = np.maximum(heat, np.abs(res))
    out["off_pct"] = max(out[c]["off_pct"] for c in "RGB")
    return out, heat, keep


def main():
    ap = argparse.ArgumentParser(description="Find color banding in a watch screenshot")
    ap.add_argument("png")
    ap.add_argument("--out", help="heatmap PNG: the bands in red")
    ap.add_argument("--tol", type=float, default=1.2, help="off by more than this (8-bit levels) is banding")
    ap.add_argument("--edge", type=float, default=36.0, help="a jump this big within 5×5 is content, not a gradient")
    ap.add_argument("--r", type=float, default=0.96, help="the part of the dial's radius looked at")
    ap.add_argument("--fail", type=float, default=5.0, help="banded when more than this %% of the kept pixels are off (calibrated: an ideal or a dithered gradient ~1.5 %%, 565 with no dither ~23 %%)")
    a = ap.parse_args()
    img = np.asarray(Image.open(a.png).convert("RGB"), dtype=float)
    out, heat, keep = analyze(img, a.tol, a.edge, a.r)
    banded = out.get("off_pct", 0) > a.fail
    print(f"{a.png}: {'BANDED' if banded else 'smooth'} — {out.get('off_pct', 0)} % of {out['kept_px']} gradient px off by > {a.tol}")
    for c in "RGB":
        if c in out:
            print(f"  {c}: off {out[c]['off_pct']} %, p95 {out[c]['p95']}, band width ~{out[c]['band_px']:.0f} px")
    if a.out:
        g = (img @ np.array([0.299, 0.587, 0.114])) * 0.45
        rgb = np.stack([g, g, g], -1)
        k = np.clip(heat / 3.0, 0, 1)[..., None]
        rgb = rgb * (1 - k) + np.array([255, 40, 40]) * k
        rgb[~keep] *= 0.5
        Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).save(a.out)
        print(f"  heatmap: {a.out}")
    sys.exit(1 if banded else 0)


if __name__ == "__main__":
    main()
