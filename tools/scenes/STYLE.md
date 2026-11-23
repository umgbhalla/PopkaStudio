# Smartwatch face library style

## Reference cues

- Vista shows how broad cutouts and black gaps can suggest depth.
- Shapes shows how rounded geometry and spaced accents can read at a glance.
- Weave shows how short marks can change direction together to suggest motion.
- Draw original silhouettes, layouts, and patterns for each face.

## Shape and edges

- Use two art layers over black. Give one the largest silhouette and add a
  third only for an accent. Keep a pattern within one layer.
- Use long, gently changing curves for silhouettes. Use rounded ends on small
  marks and a few deliberate corners on large forms. Use crisp filled edges.
- Separate broad masses with black channels. Change the direction of repeated
  marks gradually across the field.

## Palette

- Use one or two hue families per scene, with an optional third for a small
  accent. Keep large fills muted on AMOLED. Use vivid color for one focal mark.
- Give the main silhouettes a clear value step above black. Check that adjacent
  fills still separate in grayscale after RGB565 conversion.
- Use white or near-white for the time and date. Check for at least 7:1 contrast
  against the brightest decoded RGB565 background anywhere in the calm zone.
- Use flat fills. Avoid long ramps in saturated reds and blues, subtle
  near-black shades, and adjacent colors that round to the same RGB565 value.
  RGB565 gives red and blue 5 bits each and green 6.
- If a ramp is essential, dither only its interior at final size. Keep the
  background, black gaps, and exterior at RGB565 `0x0000`.

## 466 × 466 composition

- Center the 233 px radius circle at (233, 233). Keep the outer edges of
  accents within a 218 px radius.
- Keep x 90–376, y 90–250 calm. Center the light-weight time near (233, 155)
  with about 100 px of glyph height. Fit the small date in y 220–245.
- Use black or one flat dark fill in the calm zone. Keep pattern marks and
  strong edges outside it. Check the widest time and date strings.
- Place a landscape horizon near y 255–275 and a foreground break near
  y 300–340. For abstract faces, use these bands for the main changes in mass.
- Leave at least 8 px of black between floating art and the rim. For an
  intentional crop, cross the rim at a clear angle and clip to the circle.

## Detail scale at final size

- Keep artwork strokes, gaps, and the readable parts of tapered tips at least 5 px wide.
- Make isolated accents at least 10 px across. Use pattern marks 5–8 px thick
  and 12–24 px long, with at least 5 px between them.

## Production

- Hand-author SVG paths on a `0 0 466 466` viewBox. Keep the clock and date
  separate from the artwork so the watch can draw them at runtime.
- Render paths and the circle clip at 4× over opaque black. Use Python to
  average each 4 × 4 pixel block in linear light, then encode back to sRGB.
- Set pixels with centers outside the circle to `#000000`. Quantize to RGB565
  after downsampling and export in the byte order expected by the ESP32 renderer.
- Preview the decoded RGB565 frame at 1× with the time and date overlaid.
