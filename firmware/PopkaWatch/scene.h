// scene.h — a flat illustration behind the face (PopkaStudio's layer Scene, face.h: FL_SCENE): dunes, a lake, a coast,
// a city at night, a forest, peaks at dusk, concentric arcs. The art is SVG drawn to one style (tools/scenes/STYLE.md:
// three planes over true black, flat fills, a calm zone for the time in the upper middle); the build renders it 4×,
// downsamples in linear light, quantizes to the panel's RGB565 and packs it as a palette and runs (tools/scene_pack.py
// → src/scenes.cpp, generated). Drawing decodes only the rows of the band being drawn: no picture in RAM.
// The layer (FaceLayer): p2 — which scene (studio.py SCENE_LOOKS). It goes through the screen's view like every layer:
//   - not seen (a screen open over the face: an app, Settings, … — a = 0): nothing at all is drawn;
//   - at rest (the view is the identity): the runs straight into the band;
//   - moving (Apps opening over the face, the way back, a swipe): scaled about the view's centre (nearest pixel: one
//     source row a band row) and faded by the view's a — blended over what is under it.
// Black in the art is transparent (the screen under the face is black anyway): a scene never paints black over
// another screen. Its place is the whole circle (R.box): the layers over it blend with it, not with black.
// It redraws only when its picture or its view changes. Included from face.h after stars.h.
#if HAS_SCENE
#include "src/scenes.h"

static void scene_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  const float va = a * v.a;
  const int want = va > 0.004f ? constrain((int)L.p2, 0, SCENE_COUNT - 1) : -1;
  const float nv[4] = {v.x, v.y, v.s, va};
  const bool same = want == R.scn && (want < 0 || (nv[0] == R.scv[0] && nv[1] == R.scv[1] && nv[2] == R.scv[2] &&
                                                   fabsf(nv[3] - R.scv[3]) < 0.004f));
  if (!same) {  // the old place and the new one
    if (R.scn >= 0) {
      const float s = R.scv[2], r = HALO_W / 2.0f * s;
      halo_damage_f(HALO_W / 2.0f + R.scv[0] - r, HALO_H / 2.0f + R.scv[1] - r, HALO_W / 2.0f + R.scv[0] + r,
                    HALO_H / 2.0f + R.scv[1] + r);
    }
    if (want >= 0) {
      const float r = HALO_W / 2.0f * v.s;
      halo_damage_f(HALO_W / 2.0f + v.x - r, HALO_H / 2.0f + v.y - r, HALO_W / 2.0f + v.x + r, HALO_H / 2.0f + v.y + r);
    }
    R.scn = want;
    for (int k = 0; k < 4; k++) R.scv[k] = nv[k];
  }
  // (its place: the layers over it blend with it — face_over_others)
  if (want >= 0) R.box[0] = 0, R.box[1] = 0, R.box[2] = HALO_W - 1, R.box[3] = HALO_H - 1;
  else R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
}

static void scene_draw(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (R.scn < 0 || HALO_W != 466) return;  // (not seen; the pictures are the round watch's)
  const ScenePic &P = SCENE_PICS[R.scn];
  const float vx = R.scv[0], vy = R.scv[1], vs = R.scv[2], va = R.scv[3];
  const bool still = vx == 0 && vy == 0 && vs == 1, opaque = va >= 0.996f;
  const int A = (int)(va * 256);
  uint16_t pal[256];
  int pr[256], pg[256], pb[256];
  for (int k = 0; k < P.n; k++) {
    pal[k] = HALO_PX(P.pal[k]);
    pr[k] = P.pal[k] >> 11, pg[k] = (P.pal[k] >> 5) & 63, pb[k] = P.pal[k] & 31;
  }
  auto put = [&](uint16_t *px, int idx) {
    if (!P.pal[idx]) return;  // (black: transparent)
    if (opaque) *px = pal[idx];
    else halo_blend(px, pr[idx], pg[idx], pb[idx], A);
  };
  const int x0 = hb.x0, x1 = hb.x1;
  if (still) {  // at rest: the runs straight into the band
    for (int y = max(0, hb.y); y < min(HALO_H, hb.y + hb.rows); y++) {
      uint16_t *row = halo_row(y);
      const uint8_t *r = P.runs + P.rows[y], *end = P.runs + P.rows[y + 1];
      for (int x = 0; r < end && x <= x1; r += 2) {
        const int n = r[0] + 1, xe = x + n - 1;
        if (xe >= x0 && P.pal[r[1]])
          for (int k = max(x, x0), ke = min(xe, x1); k <= ke; k++) put(row + k, r[1]);
        x += n;
      }
    }
    return;
  }
  // Moving: each band row samples one source row (nearest), decoded into a line of indices
  uint8_t line[466];
  const float cx = HALO_W / 2.0f + vx, cy = HALO_H / 2.0f + vy, inv = vs > 0.01f ? 1 / vs : 0;
  if (!inv) return;
  for (int y = max(0, hb.y); y < min(HALO_H, hb.y + hb.rows); y++) {
    const int sy = (int)floorf((y + 0.5f - cy) * inv + HALO_H / 2.0f);
    if (sy < 0 || sy >= HALO_H) continue;
    const uint8_t *r = P.runs + P.rows[sy], *end = P.runs + P.rows[sy + 1];
    for (int x = 0; r < end; r += 2) {
      const int n = r[0] + 1;
      memset(line + x, r[1], min(n, 466 - x));
      x += n;
    }
    uint16_t *row = halo_row(y);
    for (int x = x0; x <= x1; x++) {
      const int sx = (int)floorf((x + 0.5f - cx) * inv + HALO_W / 2.0f);
      if (sx >= 0 && sx < HALO_W) put(row + x, line[sx]);
    }
  }
}
#endif
