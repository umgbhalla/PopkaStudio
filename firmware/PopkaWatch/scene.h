// scene.h — a flat illustration behind the face (PopkaStudio's layer Scene, face.h: FL_SCENE): dunes, a lake, a coast,
// a city at night, a forest, peaks at dusk, confetti round the rim, concentric arcs. The art is drawn as SVG to one
// style (tools/scenes/STYLE.md: three planes over true black, flat fills, a calm zone for the time in the upper middle),
// rendered 4× and downsampled in linear light, quantized to the panel's RGB565 — then packed as a palette and runs
// (tools/scene_pack.py → src/scenes.cpp: 6 … 16 KB a scene). Drawing it decodes only the rows of the band being drawn,
// span by span, into the band: no picture in RAM, no per-pixel maths.
// The layer (FaceLayer): p2 — which scene (SCENE_*, studio.py SCENE_LOOKS). It never changes by itself: the screen is
// drawn whole when it appears or its scene changes, and after that only what the layers over it change is redrawn.
// The bottom layer: the clock, the date and the rest go over it.
// Only in a firmware with a Scene layer (FN_SCENE — the editor writes it into layers.h); the editor's page has it always.
// Included from face.h after stars.h.
#if HAS_SCENE
#include "src/scenes.h"

static int scene_shown = -1;  // the scene on screen (a change: drawn whole)

static void scene_layout(const FaceScreen &S, int i, float a) {
  const FaceLayer &L = S.layers[i];
  const int want = a > 0 ? constrain((int)L.p2, 0, SCENE_COUNT - 1) : -1;
  if (want != scene_shown) scene_shown = want, halo_damage_all();
}

static void scene_draw(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  if (L.hidden) return;
  const ScenePic &P = SCENE_PICS[constrain((int)L.p2, 0, SCENE_COUNT - 1)];
  uint16_t pal[256];
  for (int k = 0; k < P.n; k++) pal[k] = HALO_PX(P.pal[k]);  // (the panel's byte order)
  const int x0 = hb.x0, x1 = hb.x1;
  for (int y = max(0, hb.y); y < min(HALO_H, hb.y + hb.rows); y++) {
    uint16_t *row = halo_row(y);
    const uint8_t *r = P.runs + P.rows[y], *end = P.runs + P.rows[y + 1];
    int x = 0;
    for (; r < end && x <= x1; r += 2) {
      const int n = r[0] + 1, xe = x + n - 1;  // (this run: x … xe)
      if (xe >= x0) {
        const uint16_t c = pal[r[1]];
        for (int k = max(x, x0), ke = min(xe, x1); k <= ke; k++) row[k] = c;
      }
      x += n;
    }
  }
}
#endif
