#!/usr/bin/env python3
"""Render 466 px watch artwork with resvg or CairoSVG, NumPy, and Pillow."""

import argparse
import io
import shutil
import subprocess
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
SIZE = 466
SCALE = 4
CALM_X = slice(110, 357)
CALM_Y = slice(293)
SCENES = ("dunes", "lake", "coast", "night", "forest", "peaks", "confetti", "arcs")
FONT = Path("/Users/zeus/hub/PopkaStudio/tools/fonts/GoogleSansFlex.ttf")
TIME_BASELINE = 204
DATE_GAP = 18
SHEET_GAP = 24
LABEL_HEIGHT = 36


def rasterize(source: Path, resvg_js: Path | None) -> np.ndarray:
    size = SIZE * SCALE
    if resvg_js is not None:
        script = """
const { readFileSync } = require('node:fs');
const { Resvg } = require(process.argv[1]);
const svg = readFileSync(process.argv[2]);
const options = { fitTo: { mode: 'width', value: Number(process.argv[3]) } };
process.stdout.write(new Resvg(svg, options).render().asPng());
"""
        png = subprocess.run(
            ["node", "-e", script, str(resvg_js.resolve()), str(source), str(size)],
            check=True,
            stdout=subprocess.PIPE,
        ).stdout
    elif resvg := shutil.which("resvg"):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "raster.png"
            subprocess.run(
                [resvg, "--width", str(size), str(source), str(output)], check=True
            )
            png = output.read_bytes()
    else:
        try:
            from cairosvg import svg2png
        except ImportError as error:
            raise RuntimeError("Install resvg or CairoSVG, or pass --resvg-js MODULE.") from error
        png = svg2png(url=str(source), output_width=size, output_height=size)

    with Image.open(io.BytesIO(png)) as image:
        if image.size != (size, size):
            raise ValueError(f"{source} rendered at {image.size}, expected {size} x {size}")
        return np.array(image.convert("RGBA"))


def circle_mask(size: int) -> np.ndarray:
    radius = size / 2
    offsets = np.arange(size) + 0.5 - radius
    return offsets[:, None] ** 2 + offsets ** 2 <= radius ** 2


def srgb_to_linear(srgb: np.ndarray) -> np.ndarray:
    return np.where(srgb <= 0.04045, srgb / 12.92, ((srgb + 0.055) / 1.055) ** 2.4)


def render(source: Path, resvg_js: Path | None = None) -> tuple[np.ndarray, np.ndarray]:
    rgba = rasterize(source, resvg_js)
    linear = srgb_to_linear(rgba[:, :, :3].astype(np.float32) / 255)
    # Composite over black in linear light and clip at the supersampled scale.
    alpha = rgba[:, :, 3] * circle_mask(SIZE * SCALE)
    linear *= (alpha.astype(np.float32) / 255)[:, :, None]
    average = linear.reshape(SIZE, SCALE, SIZE, SCALE, 3).mean(axis=(1, 3))
    srgb = np.where(
        average <= 0.0031308, 12.92 * average, 1.055 * average ** (1 / 2.4) - 0.055
    )
    srgb[~circle_mask(SIZE)] = 0

    levels = np.floor(srgb * np.array([31, 63, 31], dtype=np.float32) + 0.5).astype(np.uint16)
    red, green, blue = levels.transpose(2, 0, 1)
    packed = (red << 11) | (green << 5) | blue
    decoded = np.stack(
        ((red << 3) | (red >> 2), (green << 2) | (green >> 4), (blue << 3) | (blue >> 2)),
        axis=-1,
    ).astype(np.uint8)
    return packed, decoded


def contrast_on_white(decoded: np.ndarray) -> float:
    zone = decoded[CALM_Y, CALM_X]
    luminance = srgb_to_linear(zone / 255) @ np.array([0.2126, 0.7152, 0.0722])
    return float(1.05 / (luminance.max() + 0.05))


def preview_font(path: Path, size: int, **axes: int) -> ImageFont.FreeTypeFont:
    from fontTools.ttLib import TTFont
    from fontTools.varLib.instancer import instantiateVariableFont

    with TTFont(path) as variable:
        default_axes = {axis.axisTag: axis.defaultValue for axis in variable["fvar"].axes}
        instance = instantiateVariableFont(variable, default_axes | axes)
        data = io.BytesIO()
        instance.save(data)
    data.seek(0)
    return ImageFont.truetype(data, size)


def text_ink(
    text: str, font: ImageFont.FreeTypeFont, tracking: int = 0
) -> tuple[Image.Image, int]:
    """Return tightly cropped ink and the offset of its top from the baseline."""
    left, top, right, bottom = font.getbbox(text, anchor="ls")
    mask = Image.new("L", (right - left + tracking * (len(text) - 1), bottom - top))
    draw = ImageDraw.Draw(mask)
    if tracking:
        for index, char in enumerate(text):
            advance = font.getlength(text[: index + 1]) - font.getlength(char)
            draw.text((advance + index * tracking - left, -top), char, 255, font, anchor="ls")
    else:
        draw.text((-left, -top), text, 255, font, anchor="ls")
    bounds = mask.getbbox()
    return mask.crop(bounds), top + bounds[1]


def save_sheet(frames: list[tuple[str, Image.Image]], path: Path, font_path: Path) -> None:
    time = preview_font(font_path, 134, wght=300, wdth=100, opsz=100, ROND=0)
    date = preview_font(font_path, 22, wght=600)
    time_ink, time_top = text_ink("10:09", time)
    date_ink, _ = text_ink("MON 23", date, tracking=1)
    overlay = ((time_ink, TIME_BASELINE + time_top), (date_ink, TIME_BASELINE + DATE_GAP))
    coverage = circle_mask(SIZE * SCALE).reshape(SIZE, SCALE, SIZE, SCALE).mean(axis=(1, 3))
    mask = Image.fromarray(np.rint(coverage * 255).astype(np.uint8))

    columns = 4
    rows = (len(frames) + columns - 1) // columns
    row_height = SIZE + LABEL_HEIGHT
    width = columns * SIZE + (columns + 1) * SHEET_GAP
    height = rows * row_height + (rows + 1) * SHEET_GAP
    sheet = Image.new("RGB", (width, height), "#1a1a1a")
    for index, (name, frame) in enumerate(frames):
        tile = frame.copy()
        for ink, y in overlay:
            x = (SIZE - ink.width) // 2
            tile.paste("white", (x, y, x + ink.width, y + ink.height), ink)
        x = SHEET_GAP + index % columns * (SIZE + SHEET_GAP)
        y = SHEET_GAP + index // columns * (row_height + SHEET_GAP)
        sheet.paste(tile, (x, y), mask)
        label, _ = text_ink(name.replace("_", " ").title(), date)
        left = x + (SIZE - label.width) // 2
        top = y + SIZE + 12
        sheet.paste("#cccccc", (left, top, left + label.width, top + label.height), label)
    sheet.save(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("svg", nargs="*", type=Path, help="SVG sources (default: all eight library scenes)")
    parser.add_argument("--out", type=Path, default=ROOT / "out", help="frame output directory")
    parser.add_argument("--sheet", type=Path, help="save a 1x, four-column preview with the time and date")
    parser.add_argument("--font", type=Path, default=FONT, help="Google Sans Flex font for the preview sheet")
    parser.add_argument("--resvg-js", type=Path, metavar="MODULE", help="use an installed @resvg/resvg-js module")
    args = parser.parse_args()
    sources = args.svg or [ROOT / "art" / f"{name}.svg" for name in SCENES]
    if len({source.stem for source in sources}) != len(sources):
        parser.error("SVG sources must have distinct file names")

    args.out.mkdir(parents=True, exist_ok=True)
    frames = []
    print(f"{'Scene':10} {'Contrast':>12} {'RGB565 total':>14} {'Over 0.5%':>12}")
    for source in sources:
        packed, decoded = render(source, args.resvg_js)
        frame = Image.fromarray(decoded)
        frame.save(args.out / f"{source.stem}.png")
        (args.out / f"{source.stem}.rgb565").write_bytes(packed.astype(">u2").tobytes())
        if args.sheet:
            frames.append((source.stem, frame))
        ratio = contrast_on_white(decoded)
        verdict = "PASS" if ratio >= 7 else "FAIL"
        _, usage = np.unique(packed, return_counts=True)
        common = np.count_nonzero(usage * 200 > packed.size)
        contrast = f"{ratio:.2f}:1 {verdict}"
        print(f"{source.stem:10} {contrast:>12} {usage.size:14} {common:12}")

    if args.sheet:
        save_sheet(frames, args.sheet, args.font)


if __name__ == "__main__":
    main()
