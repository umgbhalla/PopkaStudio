// sclock.h — PopkaStudio's own clocks: sixteen looks of the time (Layers → Clock; web/mockups/clocks.html, the
// variants the user picked on 11 Nov 2026), on the round watch and on a rectangle:
//   Hairline   very thin digits, two coral dots for the colon;      Tall      the narrowest Google Sans Flex;
//   Sport      heavy slanted digits, speed streaks, the seconds;    Serif     Instrument Serif: upright hours, italic
//                                                                             minutes, a.m. under them;
//   Terminal   JetBrains Mono in amber, a prompt, a blinking cursor; LCD      seven segments drawn by code, the dark
//                                                                             ones faintly seen, the seconds, AM/PM;
//   Neon       thin tubes over a glow baked into the font;          Sunset    a gradient top to bottom, the reflection
//                                                                             under it as on water;
//   Long shadow an extruded shadow baked into the font;             Liquid    the digits fill with water over the
//                                                                             minute, waves on top;
//   Flip       split-flap cards: half a card turns over;            Comet     the seconds run round the edge (on a
//                                                                             rectangle — round its frame);
//   Stack      the hours over the minutes;                          Poster    huge digits cut by the screen's edges;
//   Hands      an analog dial: hour, minute and second hands;       Bezel     the minute scale turns under a marker,
//                                                                             the hour in the middle (on a rectangle —
//                                                                             a ruler moving sideways).
// One layer kind, FL_SCLOCK: its look in p2 (SC_*), its size k (its fonts are made at that size: studio.py
// SC_LOOKS), colors color, color2, color3; fonts font, font2, font3, font4 (what each look needs). Only the looks a
// project has are in its firmware (FN_SCLOCK in config.h: a bit a look); the editor's page has them all.
// Letters roll as the face's clock does (HaloRoll); the effects (the gradient, the water, the reflection, the cards,
// the hands) are drawn here, each part only where it changes (halo_damage) — the rest of the frame is untouched.
// Included from face.h after widgets.h.

#define SC_HAIRLINE 0
#define SC_TALL 1
#define SC_SPORT 2
#define SC_SERIF 3
#define SC_TERMINAL 4
#define SC_LCD 5
#define SC_NEON 6
#define SC_SUNSET 7
#define SC_SHADOW 8
#define SC_LIQUID 9
#define SC_FLIP 10
#define SC_COMET 11
#define SC_STACK 12
#define SC_POSTER 13
#define SC_HANDS 14
#define SC_BEZEL 15
#define SC_PILOT 16       // four big numerals, a minute track, a framed date, a hollow hour hand (Pixel Pilot's family)
#define SC_ABOLD 17       // two giant numerals (12, 6), weather and date discs at 9 and 3, solid hands
#define SC_CONCENTRIC 18  // a ring of minutes turning so the minute sits in a slot at 3, the hour big in the middle
#define SC_WEAVE 19       // a field of dashes swirling round the minute hand, made again once a minute
#define SC_WORDS 20       // the time written out: the hour bold, the minutes light
#define SC_BIGTYPE 21     // the hours over the minutes, heavy, two tones
#define SC_EYES 22        // texture: dots filling their cells and leaning toward the minute hand's tip
#define SC_PLUS 23        // texture: pluses growing near the tip
#define SC_RINGS 24       // texture: concentric rings of blocks lighting up along the minute hand
#ifdef __EMSCRIPTEN__
#define SC_ON(n) 1
#else
#define SC_ON(n) ((FN_SCLOCK >> (n)) & 1)
#endif
#ifdef HALO_ROUND
#define SCQ(r, q) (r)  // a number on the round watch (466 x 466) or on a rectangle (the 7-inch 1024 x 600)
#else
#define SCQ(r, q) (q)
#endif

#define SC_ITEMS 120
#define SC_CAPS 140  // (the dials: 60 ticks, 12 bars, the subdials' ticks, the hands with their shadows and the trail)
#define SC_RIPPLES 24
#define SC_FLIP_MS 600.0f  // a card turning over; the neon's minute flickering on
static bool face_over_others(const FaceScreen &S, int i);  // (face.h)
struct ScArea {            // a part drawn here: where it was, what it showed (a change — the old place and the new one)
  float b[4] = {0, 0, -1, -1};
  uint32_t hash = 0;
};
struct ScCap {  // a line with round ends (the hands, the ticks): drawn here, smoothed
  float x0, y0, x1, y1, hw, a;
  uint32_t c;
  float o[5];  // where it was drawn (x0, y0, x1, y1, hw); o[4] < 0 — nowhere
  uint32_t hash;
};
struct ScRun {
  HaloRoll b, c;  // the minutes (R.roll has the hours, or the whole time), the seconds
  HaloItem it[SC_ITEMS];
  ScArea area[4];
  ScCap cap[SC_CAPS];
  int ncap;
  char shown[8], hh[4], mm[4], ss[6], old_hh[4], old_mm[4], txt[16][8];
  uint32_t mm_at, hh_at;  // when the minutes, the hours changed (halo_ms)
  int sec_shown;
  float g[8];             // a look's own numbers (the colon's dots, the letters' middle)
  HaloSpring sh;          // the split looks: the colon from the anchor — the whole time centered by its ink (sc_center)
  float gz;               // the split looks: the '0's side bearing (the gap at the colon as it was, now the same both sides)
  // Laid out for draw (screen space, through the view)
  float s, a;             // the view's scale times the layer's, its alpha
  float d[12];            // a look's numbers for draw
  uint32_t col[4];
  HaloMask *mir;          // Sunset: the font's glyphs upside down (one per glyph of font)
  uint8_t *mir_bits;
  HaloPaint *rowp;        // Sunset: a band's rows' colors — per core, the letters' and the reflection's (2 × 2 × HALO_ROWS)
  uint8_t **full;         // Sunset: each glyph's solid runs (halo_mask_full) — the font's, then the upside-down ones
  uint8_t *full_bits;
  int16_t rip[SC_RIPPLES][2];
  int nrip;
  float *wave;            // Liquid: the front and the back water line per column
  int wave_x0, wave_n;
  float last_sec;
  bool vis;               // seen in the last frame (not: a screen over it, hidden — nothing worked out then)
};

static float sc_sec;     // the seconds of the minute the face shows, with the fraction (0..60)
static int sc_sec_i;
static struct tm sc_tm;  // the face's local time (the hands, the bezel): once a second, not once a layer a frame
static void sc_seconds() {
  static time_t last;
  static uint32_t at;
  if (fc_calm) return;  // (Always On: the seconds stand)
  if (fs_drag || fs_off.x != 0 || fs_off.target != 0) return;  // (switching faces: the faces stand still — only the move
                                                               // is drawn, not the hands as well: face.h)
  const time_t now = fc_wall();
  if (now != last) last = now, at = halo_ms(), localtime_r(&now, &sc_tm);
  sc_sec_i = (int)(((now % 60) + 60) % 60);
  sc_sec = sc_sec_i + fminf(0.999f, (halo_ms() - at) / 1000.0f);
}
// The hands' angles now, degrees from 12 o'clock: the hour hand's (the minutes and seconds move it on), the minute's
static inline float sc_hour_deg() { return ((sc_tm.tm_hour % 12) + sc_tm.tm_min / 60.0f + sc_sec / 3600) * 30; }
static inline float sc_min_deg() { return (sc_tm.tm_min + sc_sec / 60) * 6; }

static uint32_t sc_mix(uint32_t a, uint32_t b, float t) {  // color a toward b by t
  t = halo_clamp01(t);
  uint32_t out = 0;
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << s;
  }
  return out;
}
static uint32_t sc_hash(uint32_t h, float v) {
  h ^= (uint32_t)(int32_t)lroundf(v * 16);
  return h * 16777619u;
}
static uint32_t sc_hashu(uint32_t h, uint32_t v) {
  h ^= v;
  return h * 16777619u;
}

// ---------- Drawing helpers (in draw only) ----------
// The band being drawn narrowed to rows [y0, y1] and columns [x0, x1] for the scope's life (lib/Halo draws only within
// hb: its own clip_draw does the same)
struct ScClip {
  HaloBand saved;
  ScClip(int x0, int y0, int x1, int y1) : saved(hb) {
    const int ya = max(y0, hb.y), yb = min(y1, hb.y + hb.rows - 1), xa = max(x0, hb.x0), xb = min(x1, hb.x1);
    if (ya > yb || xa > xb) {
      hb.rows = 0;
      return;
    }
    hb.buf += (ya - hb.y) * hb.w, hb.y = ya, hb.rows = yb - ya + 1, hb.x0 = xa, hb.x1 = xb;
  }
  ~ScClip() { hb = saved; }
};
static inline int sc_nib(const uint8_t *row, int i) {
  const uint8_t b = row[i >> 1];
  return (i & 1) ? (b & 15) : (b >> 4);
}
static inline void sc_565(uint32_t c, int &r5, int &g6, int &b5) {
  r5 = (c >> 19) & 31, g6 = (c >> 10) & 63, b5 = (c >> 3) & 31;
}
// A glyph mask with its top-left at (fx, fy), scale s, squashed by sy round row cy (a turning card): nearest pixels
static void sc_mask_sy(const HaloMask &m, float fx, float fy, float s, float sy, float cy, uint32_t color, float alpha) {
  if (alpha <= 0 || sy <= 0.002f) return;
  const float y0 = cy + (fy - cy) * sy, y1 = cy + (fy + m.h * s - cy) * sy;
  if (!halo_hits(fx - 1, y0 - 1, fx + m.w * s + 1, y1 + 1)) return;
  int r5, g6, b5;
  sc_565(color, r5, g6, b5);
  const int A = (int)(alpha * 256 + 0.5f), stride = (m.w + 1) / 2;
  const int ya = max(hb.y, halo_floor(y0)), yb = min(hb.y + hb.rows - 1, halo_floor(y1));
  const float inv = 1 / s, isy = 1 / sy;
  for (int y = ya; y <= yb; y++) {
    const int j = halo_floor(((cy + (y + 0.5f - cy) * isy) - fy) * inv);
    if (j < 0 || j >= m.h || m.ext[2 * j] > m.ext[2 * j + 1]) continue;
    const int e0 = m.ext[2 * j], e1 = m.ext[2 * j + 1] == 255 ? m.w - 1 : m.ext[2 * j + 1];
    const int xa = max(hb.x0, halo_floor(fx + e0 * s)), xb = min(hb.x1, halo_floor(fx + (e1 + 1) * s));
    const uint8_t *bits = m.bits + j * stride;
    uint16_t *row = halo_row(y);
    for (int x = xa; x <= xb; x++) {
      const int i = halo_floor((x + 0.5f - fx) * inv);
      if (i < 0 || i >= m.w) continue;
      const int v = sc_nib(bits, i);
      if (v) halo_blend(row + x, r5, g6, b5, (v * 17 * A) >> 8);
    }
  }
}
// A string centered on cx by its ink (the letters themselves, not their side bearings), the middle of its letters at my
// (mid — from the line's top), scale s; squashed by sy round row cy (1 — as it is)
static void sc_glyphs(const HaloFont &f, const char *str, float cx, float my, float mid, float s, float sy, float cy,
                      uint32_t color, float alpha) {
  float x = 0, i0 = 1e9f, i1 = -1e9f;
  for (const char *p = str; *p;) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(p));
    if (!g) continue;
    if (g->w) i0 = fminf(i0, x + g->ox), i1 = fmaxf(i1, x + g->ox + g->w);
    x += g->adv;
  }
  float pen = cx - (i0 <= i1 ? i0 + i1 : x) * 0.5f * s;
  const float top = my - mid * s;
  for (const char *p = str; *p;) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(p));
    if (!g) continue;
    if (sy > 0.999f) halo_mask(*g, pen + g->ox * s, top + g->oy * s, color, alpha, s);
    else sc_mask_sy(*g, pen + g->ox * s, top + g->oy * s, s, sy, cy, color, alpha);
    pen += g->adv * s;
  }
}
// A line with round ends, smoothed: only the columns of each row it can touch
static void sc_cap_draw(const ScCap &c) {
  if (c.a <= 0.004f) return;
  const float hw = c.hw, x0 = c.x0, y0 = c.y0, x1 = c.x1, y1 = c.y1;
  const float bx0 = fminf(x0, x1) - hw - 1, bx1 = fmaxf(x0, x1) + hw + 1, by0 = fminf(y0, y1) - hw - 1,
              by1 = fmaxf(y0, y1) + hw + 1;
  if (!halo_hits(bx0, by0, bx1, by1)) return;
  int r5, g6, b5;
  sc_565(c.c, r5, g6, b5);
  const float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy, inv = len2 > 1e-6f ? 1 / len2 : 0;
  const float len = len2 > 1e-6f ? halo_sqrt(len2) : 0;
  const int A = (int)(c.a * 256 + 0.5f);
  const int ya = max(hb.y, halo_floor(by0)), yb = min(hb.y + hb.rows - 1, halo_floor(by1));
  for (int y = ya; y <= yb; y++) {
    const float py = y + 0.5f;
    int xa = max(hb.x0, halo_floor(bx0)), xb = min(hb.x1, halo_floor(bx1));
    if (fabsf(dy) > 0.01f * len) {  // within hw of the line: an interval of this row
      const float xl = x0 + (py - y0) * dx / dy, half = (hw + 1.5f) * len / fabsf(dy);
      xa = max(xa, halo_floor(xl - half)), xb = min(xb, halo_floor(xl + half));
    }
    uint16_t *row = halo_row(y);
    for (int x = xa; x <= xb; x++) {
      const float px = x + 0.5f, t = halo_clamp01(((px - x0) * dx + (py - y0) * dy) * inv);
      const float qx = x0 + t * dx - px, qy = y0 + t * dy - py, d2 = qx * qx + qy * qy;
      if (d2 >= (hw + 0.5f) * (hw + 0.5f)) continue;
      const float cov = halo_clamp01(hw + 0.5f - halo_sqrt(d2));
      if (cov > 0) halo_blend(row + x, r5, g6, b5, (int)(cov * A));
    }
  }
}

// ---------- Changes (in layout) ----------
static void sc_area(ScArea &A, bool vis, float x0, float y0, float x1, float y1, uint32_t hash) {
  const bool had = A.b[2] >= A.b[0];
  if (vis && had && A.hash == hash && A.b[0] == x0 && A.b[1] == y0 && A.b[2] == x1 && A.b[3] == y1) return;
  if (had) halo_damage_f(A.b[0], A.b[1], A.b[2], A.b[3]);
  if (!vis) {
    A.b[0] = A.b[1] = 0, A.b[2] = A.b[3] = -1, A.hash = 0;
    return;
  }
  A.b[0] = x0, A.b[1] = y0, A.b[2] = x1, A.b[3] = y1, A.hash = hash;
  halo_damage_f(x0, y0, x1, y1);
}
static void sc_cap_damage(const float *g) {  // a line's place, in pieces (a long hand: not its whole square)
  const float dx = g[2] - g[0], dy = g[3] - g[1], hw = g[4] + 2;
  const int n = 1 + (int)(halo_sqrt(dx * dx + dy * dy) / 40);
  for (int k = 0; k < n; k++) {
    const float ax = g[0] + dx * k / n, ay = g[1] + dy * k / n, bx = g[0] + dx * (k + 1) / n, by = g[1] + dy * (k + 1) / n;
    halo_damage_f(fminf(ax, bx) - hw, fminf(ay, by) - hw, fmaxf(ax, bx) + hw, fmaxf(ay, by) + hw);
  }
}
static void sc_cap_commit(ScCap &c) {
  const bool vis = c.a > 0.004f;
  uint32_t h = 2166136261u;
  h = sc_hash(h, c.x0), h = sc_hash(h, c.y0), h = sc_hash(h, c.x1), h = sc_hash(h, c.y1), h = sc_hash(h, c.hw);
  h = sc_hash(h, c.a * 64), h = sc_hashu(h, c.c);
  const bool had = c.o[4] >= 0;
  if (vis && had && h == c.hash) return;
  if (had) sc_cap_damage(c.o);
  c.o[4] = -1;
  if (!vis) return;
  c.o[0] = c.x0, c.o[1] = c.y0, c.o[2] = c.x1, c.o[3] = c.y1, c.o[4] = c.hw, c.hash = h;
  sc_cap_damage(c.o);
}
// A line through the view (screen space)
static void sc_cap(ScRun &c, int i, float x0, float y0, float x1, float y1, float hw, uint32_t color, float a,
                   const HaloView &v) {
  ScCap &k = c.cap[i];
  k.x0 = halo_view_x(v, x0), k.y0 = halo_view_y(v, y0), k.x1 = halo_view_x(v, x1), k.y1 = halo_view_y(v, y1);
  k.hw = hw * v.s, k.c = color, k.a = a * v.a;
  sc_cap_commit(k);
}
static void sc_hide_caps(ScRun &c, int from) {
  for (int i = from; i < SC_CAPS; i++) c.cap[i].a = 0, sc_cap_commit(c.cap[i]);
}

// Items: on black (nothing under the clock) — the quick way
static bool sc_ob;
static void sc_circle(HaloItem &it, float x, float y, float r, uint32_t c, float a, const HaloView &v) {
  it.kind = HI_CIRCLE, it.x = x, it.y = y, it.r = r, it.scale = 1, it.color = c, it.alpha = a, it.on_black = sc_ob;
  it.on = nullptr;
  halo_view_commit(it, v);
}
static void sc_rrect(HaloItem &it, float x, float y, float w, float h, float r, uint32_t c, float a, const HaloView &v) {
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = 1, it.color = c, it.alpha = a;
  it.on_black = sc_ob, it.on = nullptr;
  halo_view_commit(it, v);
}
// Text: align 0 — from x, 1 — centered, 2 — up to x; the middle of its letters at y (mid — from the line's top). On a
// solid item (a bezel band, a subdial's disc: dl_text's on) its edges are in that item's colors — no dark fringe
static void dl_text(HaloItem &it, const HaloFont *f, const char *str, float x, float y, float mid, int align, uint32_t col,
                    float a, const HaloView &v, const HaloItem *on, int spacing = 0) {
  it.kind = HI_TEXT, it.font = f, it.text = str, it.align = align, it.spacing = spacing, it.scale = 1, it.color = col;
  it.alpha = f ? a : 0, it.on_black = on == &halo_black, it.on = on;
  if (f) it.x = x, it.y = y + (f->line / 2.0f - mid);
  halo_view_commit(it, v);
}
// …over black, or blended over what is under (sc_ob)
static void sc_text(HaloItem &it, const HaloFont *f, const char *s, float x, float y, float mid, int align, uint32_t c,
                    float a, const HaloView &v) {
  dl_text(it, f, s, x, y, mid, align, c, a, v, sc_ob ? &halo_black : nullptr);
}
static void sc_hide(ScRun &c, int from, const HaloView &v) {
  for (int i = from; i < SC_ITEMS; i++)
    if (c.it[i].shown) c.it[i].alpha = 0, halo_view_commit(c.it[i], v);
}
static float sc_mid(const HaloFont *f, const char *ref = "0123456789") {
  return f ? halo_text_mid(*f, ref) : 0;
}


// ---------- The dials (Pilot, Analogue bold, Concentric, Modular, Weave): an analog watch's parts ----------
// (the research: their hands, indices, subdials, measured from Apple's own renders — 23 Nov 2026). Our designs, their
// construction. Order back to front: the dial's items [0, d[10]) → the tick caps [0, d[11]) → the complications' items
// [d[10], n) → the hands' caps [d[11], ncap) with their shadows, pivots and the second hand's light trail.
static void dl_ring(HaloItem &it, float x, float y, float r, float w, float frac, uint32_t col, uint32_t track, float a,
                    const HaloView &v) {
  it.kind = HI_RING, it.x = x, it.y = y, it.r = r, it.w = w, it.v = frac, it.scale = 1, it.color = col, it.color2 = track;
  it.alpha = a, it.on_black = sc_ob, it.on = nullptr;
  halo_view_commit(it, v);
}
static void dl_arc(HaloItem &it, float x, float y, float r, float w, float a0, float a1, uint32_t col, float a,
                   const HaloView &v) {
  it.kind = HI_ARC, it.x = x, it.y = y, it.r = r, it.w = w, it.a0 = a0, it.a1 = a1, it.scale = 1, it.color = col;
  it.alpha = a, it.on_black = false, it.on = nullptr;
  halo_view_commit(it, v);
}
// Optical balance (the dials' rules, 23 Nov 2026): three text sizes a face — the hero (the day), values, labels; labels
// are small caps tracked open (+2 px) and dimmer; a pair of texts is centered as one by its measured width
#define DL_TRACK 2
static float dl_width(const HaloFont *f, const char *str, int spacing = 0) {
  return f ? halo_text_width(*f, str, spacing) : 0;
}
// A point r from (ax, ay) at deg degrees clockwise from 12 o'clock; dl_dir — the direction once, for several points on it
struct DlDir {
  float sn, cs;
};
static inline DlDir dl_dir(float deg) {
  const float an = deg * (HALO_PI / 180);
  return {sinf(an), cosf(an)};
}
static inline void dl_at(float ax, float ay, DlDir d, float r, float &x, float &y) {
  x = ax + r * d.sn, y = ay - r * d.cs;
}
static inline void dl_at(float ax, float ay, float deg, float r, float &x, float &y) {
  dl_at(ax, ay, dl_dir(deg), r, x, y);
}
// A hand of Apple's family: a thin neck from the pivot, then a capsule blade (round ends) to the tip; hollow — a light
// outline round a dark channel. Under it a soft shadow: three wider passes of black, a little lower (it shows over the
// bezel and the subdials and keeps the hands readable over them)
static int dl_hand(ScRun &c, int nc, float ax, float ay, float deg, float neck, float tip, float bw, float ow,
                   uint32_t outline, uint32_t channel, bool hollow, float a, const HaloView &v) {
  float nx, ny, bx, by, tx, ty;
  dl_at(ax, ay, deg, neck + bw / 2, bx, by), dl_at(ax, ay, deg, tip - bw / 2, tx, ty), dl_at(ax, ay, deg, 0, nx, ny);
  static const float SW[3] = {4.5f, 3, 1.5f}, SA[3] = {0.10f, 0.14f, 0.20f};
  for (int j = 0; j < 3; j++) sc_cap(c, nc++, bx, by + 2.5f, tx, ty + 2.5f, bw / 2 + SW[j], 0x000000, a * SA[j], v);
  sc_cap(c, nc++, nx, ny, bx, by, 1.6f, outline, a, v);
  sc_cap(c, nc++, bx, by, tx, ty, bw / 2, outline, a, v);
  sc_cap(c, nc++, bx, by, tx, ty, hollow ? bw / 2 - ow : 0, channel, hollow ? a : 0, v);
  return nc;
}
// The hour and minute hands, their pivot, a colored disc with a dark dot over it (no second hand)
static int dl_hands(ScRun &c, int nc, float ax, float ay, float R, uint32_t light, uint32_t channel, uint32_t sec,
                    float hour_tip, float min_tip, float a, float sa, const HaloView &v, uint32_t hour = 0,
                    bool hour_hollow = true) {
  // The hour and minute hands move by half a pixel at their tips — continuous to the eye, but not redrawn every frame
  // (each redraw is the hand, its shadow and all under it: measured 47 fps when they moved every frame). The second
  // hand alone sweeps every frame
  const float hq = 0.5f / hour_tip * (180 / HALO_PI), mq = 0.5f / min_tip * (180 / HALO_PI);
  const float ha = floorf(sc_hour_deg() / hq) * hq;
  const float ma = floorf(sc_min_deg() / mq) * mq;
  const float sdeg = sc_sec * 6;
  // (Wear OS's analog faces: a thick hour hand in the accent, solid; a thinner light minute hand)
  nc = dl_hand(c, nc, ax, ay, ha, R * 0.13f, hour_tip, R * (hour_hollow ? 0.068f : 0.078f), R * 0.016f, hour ? hour : light,
               channel, hour_hollow, a, v);
  nc = dl_hand(c, nc, ax, ay, ma, R * 0.13f, min_tip, R * 0.062f, R * 0.016f, light, channel, true, a, v);
  sc_cap(c, nc++, ax, ay, ax, ay, R * 0.03f, light, a, v);    // the hands' pivot: a light ring…
  sc_cap(c, nc++, ax, ay, ax, ay, R * 0.013f, channel, a, v);  // …round a dark hole
  // No sweeping second hand (the user's call, 23 Nov 2026: a hand redrawn every frame across the dial was too much
  // work); its accent stays as the pivot's colored disc with a dark dot
  (void)sdeg, (void)sa;
  sc_cap(c, nc++, ax, ay, ax, ay, R * 0.032f, sec, a, v);
  sc_cap(c, nc++, ax, ay, ax, ay, R * 0.011f, 0x000000, a, v);
  return nc;
}

// ---------- The textures: a field of one primitive driven by the minute hand's tip ----------
// (textures.dev's principle — a regular grid of one primitive whose size, offset, angle and ink follow the distance to
// one point — with the hand's tip as the point: our own fields.) Each field is a list of capsules (a dash, a dot — a
// capsule of no length —, a plus's two arms, a ring's block); at a new minute every capsule eases from where it was to
// its new place over WV_EASE ms (the field breathes once a minute), and in between nothing is redrawn. One textured
// face on the watch at a time
#define WV_MAX 900
#define WV_EASE 650.0f
struct WvEl {
  float x0, y0, x1, y1, hw, a;
};
static WvEl *wv_from, *wv_to;
static ScCap *wv;  // what is drawn (from → to, eased)
static int wv_n;
static uint32_t wv_key, wv_t0;
static bool wv_moving;
static bool wv_alloc() {
  if (!wv) {
#ifdef ESP_PLATFORM
    wv = (ScCap *)heap_caps_calloc(WV_MAX, sizeof(ScCap), MALLOC_CAP_SPIRAM);
    wv_from = (WvEl *)heap_caps_calloc(WV_MAX, sizeof(WvEl), MALLOC_CAP_SPIRAM);
    wv_to = (WvEl *)heap_caps_calloc(WV_MAX, sizeof(WvEl), MALLOC_CAP_SPIRAM);
#else
    wv = (ScCap *)calloc(WV_MAX, sizeof(ScCap));
    wv_from = (WvEl *)calloc(WV_MAX, sizeof(WvEl)), wv_to = (WvEl *)calloc(WV_MAX, sizeof(WvEl));
#endif
  }
  return wv && wv_from && wv_to;
}
static inline void wv_add(int &n, float x0, float y0, float x1, float y1, float hw, float a) {
  if (n < WV_MAX) wv_to[n++] = {x0, y0, x1, y1, hw, a};
}
// The field for a look, the tip at (tx, ty), the hand's angle hd (degrees): into wv_to, its count
static int wv_field(int look, float ax, float ay, float R, float k, float tx, float ty, float hd) {
  int n = 0;
  // (the field runs past the rim: the screen's own circle cuts it — whole cells cut at the edge, never a smaller
  // circle or a fade: the user's call, 23 Nov 2026)
  const float rr = (R + 30 * k) * (R + 30 * k);
  auto inside = [&](float x, float y) { return (x - ax) * (x - ax) + (y - ay) * (y - ay) <= rr; };
  if (look == SC_WEAVE) {  // dashes turned round the tip, a swirl; fainter near it
    const float step = 21 * k, len = 7 * k;
    for (float y = ay - R + step / 2; y < ay + R; y += step)
      for (float x = ax - R + step / 2; x < ax + R; x += step) {
        if (!inside(x, y)) continue;
        const float qx = x - tx, qy = y - ty, d = sqrtf(qx * qx + qy * qy) + 1;
        const float an = atan2f(qy, qx) + HALO_PI / 2 + 0.6f * expf(-d / (R * 0.5f));
        const float ux = cosf(an) * len, uy = sinf(an) * len;
        wv_add(n, x - ux, y - uy, x + ux, y + uy, 2.2f * k, 0.35f + 0.65f * halo_clamp01((d - R * 0.12f) / (R * 0.5f)));
      }
  } else if (look == SC_EYES) {  // dots in cells: small at rest, filling their cells and leaning toward the tip near it
    const float step = 26 * k, cell = 22 * k;
    for (float y = ay - R + step / 2; y < ay + R; y += step)
      for (float x = ax - R + step / 2; x < ax + R; x += step) {
        if (!inside(x, y)) continue;
        const float qx = tx - x, qy = ty - y, d = sqrtf(qx * qx + qy * qy) + 0.01f;
        const float f = powf(halo_clamp01(1 - d / (R * 0.62f)), 1.6f);
        const float r = 2.2f * k + (cell / 2 - 2.2f * k) * f, room = (cell / 2 - r) * 0.85f * halo_clamp01(f * 3);
        const float cx = x + qx / d * room, cy = y + qy / d * room;
        wv_add(n, cx, cy, cx, cy, r, 0.30f + 0.70f * f);
      }
  } else if (look == SC_PLUS) {  // pluses: grow near the tip and slide a little toward it
    const float step = 30 * k;
    for (float y = ay - R + step / 2; y < ay + R; y += step)
      for (float x = ax - R + step / 2; x < ax + R; x += step) {
        if (!inside(x, y)) continue;
        const float qx = tx - x, qy = ty - y, d = sqrtf(qx * qx + qy * qy) + 0.01f;
        const float f = powf(halo_clamp01(1 - d / (R * 0.7f)), 1.4f);
        const float L = 3 * k + 7 * k * f, hw = 1.3f * k + 1.4f * k * f, sl = 5 * k * f;
        const float cx = x + qx / d * sl, cy = y + qy / d * sl, a = 0.32f + 0.68f * f;
        wv_add(n, cx - L, cy, cx + L, cy, hw, a), wv_add(n, cx, cy - L, cx, cy + L, hw, a);
      }
  } else if (look == SC_RINGS) {  // concentric rings of blocks; along the minute hand they lengthen and light up
    for (float r = 34 * k; r < R - 14 * k; r += 21 * k) {
      const int m = max(6, (int)(2 * HALO_PI * r / (22 * k)));
      for (int j = 0; j < m; j++) {
        const float an = j * 360.0f / m, da = fmodf(an - hd + 540, 360) - 180;
        const float g = expf(-(da / 34) * (da / 34));
        const float half = (2.6f + 4.6f * g) * k / r * (180 / HALO_PI);  // (the block's half length as an angle)
        float x0, y0, x1, y1;
        dl_at(ax, ay, an - half, r, x0, y0), dl_at(ax, ay, an + half, r, x1, y1);
        wv_add(n, x0, y0, x1, y1, (1.6f + 1.4f * g) * k, 0.28f + 0.72f * g);
      }
    }
  }
  return n;
}
// Each frame (layout): a new minute (or look, size, colour) — a new field, eased in from the one shown; returns whether
// it is still moving (the circle is drawn again then)
static void wv_update(int look, float ax, float ay, float R, float k, uint32_t col) {
  if (!wv_alloc()) return;
  const uint32_t key = (uint32_t)look * 2654435761u ^ (uint32_t)(sc_tm.tm_hour * 60 + sc_tm.tm_min) * 40503u ^
                       (uint32_t)(k * 1000) ^ col * 31u ^ (uint32_t)(ax * 7 + ay * 13);
  if (key != wv_key) {
    const bool same_look = wv_n > 0 && (wv_key >> 0) != 0 && wv_from;
    float tx, ty;
    const float hd = sc_min_deg();
    dl_at(ax, ay, hd, R * 0.55f, tx, ty);
    const int n = wv_field(look, ax, ay, R, k, tx, ty, hd);
    for (int i = 0; i < n; i++)  // (from: what is shown now — or, a new field of another size, itself)
      wv_from[i] = same_look && n == wv_n ? WvEl{wv[i].x0, wv[i].y0, wv[i].x1, wv[i].y1, wv[i].hw, wv[i].a} : wv_to[i];
    wv_n = n, wv_key = key, wv_t0 = halo_ms(), wv_moving = true;
    for (int i = 0; i < n; i++) wv[i].c = col;
  }
  if (!wv_moving) return;
  const float t = halo_clamp01((halo_ms() - wv_t0) / WV_EASE), e = 1 - (1 - t) * (1 - t) * (1 - t);
  for (int i = 0; i < wv_n; i++) {
    const WvEl &a = wv_from[i], &b = wv_to[i];
    ScCap &c = wv[i];
    c.x0 = a.x0 + (b.x0 - a.x0) * e, c.y0 = a.y0 + (b.y0 - a.y0) * e, c.x1 = a.x1 + (b.x1 - a.x1) * e;
    c.y1 = a.y1 + (b.y1 - a.y1) * e, c.hw = a.hw + (b.hw - a.hw) * e, c.a = a.a + (b.a - a.a) * e;
  }
  halo_damage_f(ax - R, ay - R, ax + R, ay + R);
  if (t >= 1) wv_moving = false;
}

// ---------- Built once a change (face_layer_build) ----------
static void sc_layer_free(FaceRun &R) {
  ScRun *c = R.sc;
  if (!c) return;
  free(c->mir), free(c->mir_bits), free(c->wave), free(c->rowp), free(c->full), free(c->full_bits);
  c->~ScRun();
  free(c);
  R.sc = nullptr;
}
static ScRun *sc_new() {
  void *p = psram_calloc(1, sizeof(ScRun));
  if (!p) return nullptr;
  ScRun *c = new (p) ScRun();
  for (auto &k : c->cap) k.o[4] = -1, k.a = 0;
  c->sec_shown = -1, c->last_sec = -1;
  return c;
}
static void sc_roll(HaloRoll &r, const HaloFont *f, int align, bool tabular) {
  if (!f) return;
  r.tabular = tabular;
  halo_roll_init(r, *f, "0123456789", align, 0);
}
// The colon of font f as two dots: the radius, the dots' middles from the letters' middle, half its advance
static void sc_colon(ScRun &c, const HaloFont *f, float mid) {
  const HaloMask *g = f ? halo_glyph(*f, ':') : nullptr;
  if (!g) return (void)(c.g[0] = c.g[1] = c.g[2] = c.g[3] = 0);
  const float r = g->w / 2.0f;
  c.g[0] = r, c.g[1] = g->oy + r - mid, c.g[2] = g->oy + g->h - r - mid, c.g[3] = g->adv / 2.0f;
}
// The split looks (the hours | the colon | the minutes; 12 Nov 2026, the user: "centered, but the margins differ"): the
// colon stood on the anchor, so "12:00" sat 68 px right of the middle (a narrow 1, wide zeros). Now the hours' ink ends
// and the minutes' starts the same gap from the colon (their rolls: ink), and the colon goes where the whole time's ink
// is centered on the anchor; a new width — it moves with the rolls' own spring (lib/Halo halo_roll.cpp: K_X)
static HaloSpringK sc_kx;
static void sc_split(const HaloFont *f, HaloRoll &hh, HaloRoll &mm, ScRun &c) {
  const HaloMask *z = f ? halo_glyph(*f, '0') : nullptr;
  hh.ink = mm.ink = true, c.gz = z ? z->ox : 0;
}
static void sc_center(ScRun &c, const HaloRoll &hh, bool animate) {
  float h0, h1, m0, m1;
  if (!halo_roll_ink(hh, h0, h1) || !halo_roll_ink(c.b, m0, m1)) return;
  const float off = ((h1 - h0) - (m1 - m0)) * 0.5f;
  if (animate) c.sh.target = off;
  else halo_spring_set(c.sh, off);
}
#if SC_ON(SC_SUNSET)
static void sc_mirror(ScRun &c, const HaloFont &f) {  // every glyph of the font upside down (the reflection)
  size_t bytes = 0;
  for (int i = 0; i < f.count; i++) bytes += (size_t)f.glyphs[i].h * ((f.glyphs[i].w + 1) / 2 + 2);
  c.mir = (HaloMask *)calloc(f.count, sizeof(HaloMask));
  c.mir_bits = (uint8_t *)malloc(bytes);
#ifdef __EMSCRIPTEN__
  c.rowp = (HaloPaint *)calloc(4 * HALO_ROWS, sizeof(HaloPaint));
#else
  c.rowp = (HaloPaint *)heap_caps_calloc(4 * HALO_ROWS, sizeof(HaloPaint), MALLOC_CAP_SPIRAM);
#endif
  if (!c.mir || !c.mir_bits) return;
  uint8_t *o = c.mir_bits;
  for (int i = 0; i < f.count; i++) {
    const HaloMask &g = f.glyphs[i];
    const int stride = (g.w + 1) / 2;
    uint8_t *bits = o, *ext = o + g.h * stride;
    for (int r = 0; r < g.h; r++) {
      memcpy(bits + r * stride, g.bits + (g.h - 1 - r) * stride, stride);
      ext[2 * r] = g.ext[2 * (g.h - 1 - r)], ext[2 * r + 1] = g.ext[2 * (g.h - 1 - r) + 1];
    }
    c.mir[i] = g, c.mir[i].bits = bits, c.mir[i].ext = ext;
    o += g.h * (stride + 2);
  }
  // Their solid runs (a letter's middle — spans, not sampled pixel by pixel), 8 bytes a row
  size_t fb = 0;
  for (int i = 0; i < f.count; i++) fb += (size_t)f.glyphs[i].h * 8 * 2;
#ifdef __EMSCRIPTEN__
  c.full = (uint8_t **)calloc(f.count * 2, sizeof(uint8_t *)), c.full_bits = (uint8_t *)malloc(fb);
#else
  c.full = (uint8_t **)heap_caps_calloc(f.count * 2, sizeof(uint8_t *), MALLOC_CAP_SPIRAM);
  c.full_bits = (uint8_t *)heap_caps_malloc(fb, MALLOC_CAP_SPIRAM);
#endif
  if (!c.full || !c.full_bits) {
    free(c.full), free(c.full_bits), c.full = nullptr, c.full_bits = nullptr;
    return;
  }
  uint8_t *q = c.full_bits;
  for (int i = 0; i < f.count; i++) {
    c.full[i] = q, halo_mask_full(f.glyphs[i], q), q += f.glyphs[i].h * 8;
    c.full[f.count + i] = q, halo_mask_full(c.mir[i], q), q += f.glyphs[i].h * 8;
  }
}
#endif

static void sc_texts(const FaceLayer &L, FaceRun &R, bool animate);
static void sc_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  ScRun *c = R.sc = sc_new();
  if (!c) return;
  static bool kx;
  if (!kx) sc_kx = halo_spring(HALO_M3X_DEFAULT), kx = true;
  const int look = (int)L.p2;
  switch (look) {
    case SC_HAIRLINE:
    case SC_TALL:
    case SC_SERIF:
    case SC_SHADOW:
    case SC_STACK:
      sc_roll(R.roll, L.font, look == SC_STACK ? 1 : 2, false);
      sc_roll(c->b, look == SC_SERIF ? L.font2 : L.font, look == SC_STACK ? 1 : 0, false);
      sc_colon(*c, L.font, R.roll.mid);
      sc_split(L.font, R.roll, c->b, *c);  // (the stack: each line centered by its ink)
      break;
    case SC_POSTER:  // (cut by the screen's edges on purpose: as it was)
      sc_roll(R.roll, L.font, 0, false);
      sc_roll(c->b, L.font, 2, false);
      break;
    case SC_NEON:  // (the glow's font lays the letters out: its boxes hold the glow; the tubes — on the same pens)
      sc_roll(R.roll, L.font2, 2, false);
      sc_roll(c->b, L.font2, 0, false);
      sc_split(L.font2, R.roll, c->b, *c);
      break;
    case SC_TERMINAL:  // (the glow's font lays the time out, as the neon's)
      sc_roll(R.roll, L.font4 ? L.font4 : L.font, 0, true);
      sc_roll(c->c, L.font2, 0, true);
      c->g[0] = sc_mid(L.font3, "$ clock"), c->g[1] = L.font2 ? halo_glyph(*L.font2, '0') ? halo_glyph(*L.font2, '0')->adv : 0 : 0;
      break;
    case SC_SPORT:
      sc_roll(R.roll, L.font, 1, true);
      sc_roll(c->c, L.font2, 2, true);
      break;
    case SC_LCD:
      c->g[0] = sc_mid(L.font, "AMP");
      break;
    case SC_FLIP:
      c->g[0] = sc_mid(L.font);
      break;
    case SC_BEZEL:
      sc_roll(R.roll, L.font, 1, false);
      R.roll.ink = true;  // (the hour: centered by its ink over the ruler)
      c->g[0] = sc_mid(L.font2), c->g[1] = sc_mid(L.font3);
      break;
    case SC_HANDS:
      break;
    case SC_PILOT:
    case SC_ABOLD:
    case SC_CONCENTRIC:
    case SC_WEAVE:
    case SC_EYES:
    case SC_PLUS:
    case SC_RINGS:
    case SC_BIGTYPE:
      c->g[0] = sc_mid(L.font), c->g[1] = sc_mid(L.font2), c->g[2] = sc_mid(L.font3, "MONTUEWEDFRI0123");
      break;
    case SC_WORDS:
      c->g[0] = sc_mid(L.font, "Twelve"), c->g[1] = sc_mid(L.font2, "oh twenty");
      break;
    default:  // Sunset, Liquid, Comet: the whole time in one line, the colon in the middle
      sc_roll(R.roll, L.font, 1, true);
      R.roll.ink = true;  // (centered by its ink: the first digit's to the last one's)
#if SC_ON(SC_SUNSET)
      if (look == SC_SUNSET && L.font) sc_mirror(*c, *L.font);
#endif
  }
  sc_texts(L, R, false);
}

// The time into the rolls: what the face shows now (fc_time "10:24"), the seconds
static void sc_texts(const FaceLayer &L, FaceRun &R, bool animate) {
  ScRun *c = R.sc;
  if (!c) return;
  const int look = (int)L.p2;
  if (strcmp(c->shown, fc_time)) {
    const bool first = !*c->shown;
    strlcpy(c->old_hh, c->hh, sizeof(c->old_hh)), strlcpy(c->old_mm, c->mm, sizeof(c->old_mm));
    char hh[4] = {fc_time[0], fc_time[1], 0}, mm[4] = {fc_time[3], fc_time[4], 0};
    if (strcmp(hh, c->hh)) c->hh_at = first ? 0 : halo_ms();
    if (strcmp(mm, c->mm)) c->mm_at = first ? 0 : halo_ms();
    if (first) strlcpy(c->old_hh, hh, 4), strlcpy(c->old_mm, mm, 4);
    strlcpy(c->hh, hh, 4), strlcpy(c->mm, mm, 4), strlcpy(c->shown, fc_time, sizeof(c->shown));
    switch (look) {
      case SC_HAIRLINE:
      case SC_TALL:
      case SC_SERIF:
      case SC_SHADOW:
      case SC_STACK:
      case SC_POSTER:
      case SC_NEON:
        halo_roll_set(R.roll, hh, animate), halo_roll_set(c->b, mm, animate);
        if (look != SC_STACK && look != SC_POSTER) sc_center(*c, R.roll, animate);
        break;
      case SC_BEZEL:
        halo_roll_set(R.roll, hh, animate);
        break;
      case SC_LCD:
      case SC_FLIP:
      case SC_HANDS:
      case SC_PILOT:
      case SC_ABOLD:
      case SC_CONCENTRIC:
        case SC_WEAVE:
      case SC_EYES:
      case SC_PLUS:
      case SC_RINGS:
      case SC_WORDS:
      case SC_BIGTYPE:
        break;
      default:
        halo_roll_set(R.roll, fc_time, animate);
    }
  }
  if ((look == SC_SPORT || look == SC_TERMINAL) && sc_sec_i != c->sec_shown) {
    c->sec_shown = sc_sec_i;
    snprintf(c->ss, sizeof(c->ss), look == SC_TERMINAL ? ":%02d" : "%02d", sc_sec_i);
    halo_roll_set(c->c, c->ss, animate && !fc_calm);
  }
}

// Every frame (face_tick): the seconds, a new minute into the rolls, the rolls' springs
static void sc_tick(float dt) {
  sc_seconds();
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) {
      const FaceLayer &L = face_scr[k].layers[i];
      FaceRun &R = face_scr[k].run[i];
      if (L.kind != FL_SCLOCK || !R.sc) continue;
      sc_texts(L, R, !fc_still);
      halo_roll_update(R.sc->b, dt), halo_roll_update(R.sc->c, dt);
      halo_spring_run(R.sc->sh, sc_kx, dt);
    }
}

// ---------- Each frame ----------
static void sc_box_roll(float *box, const HaloRoll &r) {
  for (int c = 0; c < r.n; c++) box_add(box, r.c[c].in), box_add(box, r.c[c].out);
}
static void sc_box_add(float *box, float x0, float y0, float x1, float y1) {
  if (x1 < x0) return;
  if (box[2] < box[0]) {
    box[0] = x0, box[1] = y0, box[2] = x1, box[3] = y1;
    return;
  }
  box[0] = fminf(box[0], x0), box[1] = fminf(box[1], y0), box[2] = fmaxf(box[2], x1), box[3] = fmaxf(box[3], y1);
}
// A roll's letters with the colors of three: the items get one color that changes when any of them does (they are
// drawn here, the item's color is never painted — but its change is what redraws them)
static uint32_t sc_key(const FaceLayer &L) {
  return (L.color ^ (L.color2 * 31u) ^ (L.color3 * 131u)) & 0xFFFFFF;
}
// The box a glyph of item it covers, and where a glyph g of another font of the same letter goes (its top-left)
static void sc_at(const HaloItem &it, const HaloMask &g, float &x, float &y) {
  const HaloMask &m = *it.mask;
  const float s = it.scale;
  x = it.x - m.w * s / 2 + (g.ox - m.ox) * s, y = it.y - m.h * s / 2 + (g.oy - m.oy) * s;
}

static void sc_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  ScRun *cp = R.sc;
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!cp) return;
  ScRun &c = *cp;
  const int look = (int)L.p2;
  const float k = L.k > 0 ? L.k : 1, ax = CX + L.dx, ay = CY + L.dy;
  R.ax = ax, R.ay = ay;
  if (a * v.a <= 0.004f) {  // not seen (a screen open over the face, hidden): its parts go away once, nothing else
    c.a = 0;                // (what draw paints by itself — the cards — goes with it)
    if (c.vis) {
      for (HaloRoll *r : {&R.roll, &c.b, &c.c}) halo_roll_layout(*r, ax, ay, L.color, 0, nullptr, v);
      sc_hide(c, 0, v), sc_hide_caps(c, 0);
      for (ScArea &q : c.area) sc_area(q, false, 0, 0, -1, -1, 0);
      c.vis = false, c.last_sec = -1;
    }
    return;
  }
  c.vis = true;
  sc_ob = !face_over_others(S, i);
  const bool ob = sc_ob;  // (the looks that set sc_ob go back to this, never to "on black" by themselves)
  const HaloItem *on = sc_ob ? &halo_black : nullptr;
  c.s = v.s, c.a = a * v.a;
  const float sa = fc_calm ? 0 : a;  // what shows the seconds: not in Always On
  int n = 0;                         // items used
  float *box = R.box;
  switch (look) {
#if SC_ON(SC_HAIRLINE) || SC_ON(SC_TALL) || SC_ON(SC_SERIF)
    case SC_HAIRLINE:
    case SC_TALL:
    case SC_SERIF: {
      // the hours' ink up to the colon, the minutes' from it (the same gap); the colon — two dots (color2) — where the
      // whole time is centered on the anchor (sc_center)
      const float cy = ay + (look == SC_SERIF ? SCQ(-22, -32) * k : 0);
      const float g = look == SC_HAIRLINE ? SCQ(24, 52) * k : c.g[3], gi = g + c.gz, cx = ax + c.sh.x;
      halo_roll_layout(R.roll, cx - gi, cy, L.color, a, on, v);
      halo_roll_layout(c.b, cx + gi, cy, L.color, a, on, v);
      if (look == SC_HAIRLINE) {
        const float r = SCQ(7, 14) * k;
        sc_circle(c.it[n++], cx, cy - g * 0.95f, r, L.color2, a, v), sc_circle(c.it[n++], cx, cy + g * 0.95f, r, L.color2, a, v);
      } else {
        sc_circle(c.it[n++], cx, cy + c.g[1], c.g[0], L.color2, a, v), sc_circle(c.it[n++], cx, cy + c.g[2], c.g[0], L.color2, a, v);
      }
      if (look == SC_SERIF) {
        strlcpy(c.txt[0], fc_hour < 12 ? "a.m." : "p.m.", 8);
        sc_text(c.it[n++], L.font3, c.txt[0], ax, ay + SCQ(102, 219) * k, sc_mid(L.font3, "am"), 1, L.color3, fc_h24 ? 0 : a, v);
      }
      sc_box_roll(box, R.roll), sc_box_roll(box, c.b);
      break;
    }
#endif
#if SC_ON(SC_STACK) || SC_ON(SC_POSTER)
    case SC_STACK:
    case SC_POSTER:
      if (look == SC_STACK) {
        const float d = SCQ(80, 125) * k;
        halo_roll_layout(R.roll, ax, ay - d, L.color, a, on, v);
        halo_roll_layout(c.b, ax, ay + d, L.color2, a, on, v);
      } else {
        halo_roll_layout(R.roll, ax + SCQ(-237, -542) * k, ay + SCQ(-147, -139) * k, L.color, a, on, v);
        halo_roll_layout(c.b, ax + SCQ(259, 548) * k, ay + SCQ(121, 191) * k, L.color2, a, on, v);
      }
      sc_box_roll(box, R.roll), sc_box_roll(box, c.b);
      break;
#endif
#if SC_ON(SC_SPORT)
    case SC_SPORT: {
      // dark streaks behind (color3), the time, coral streaks (color2) leading to the seconds
#ifdef HALO_ROUND
      static const float DARK[3][4] = {{-44, -273, -43, 7}, {-18, -273, 97, 10}, {12, -273, 27, 7}};
      static const float FAST[2][5] = {{62, -63, 119, 7, 0.9f}, {80, 3, 119, 7, 0.45f}};
#else
      static const float DARK[3][4] = {{-114, -552, -92, 14}, {-58, -552, 248, 20}, {6, -552, 88, 14}};
      static const float FAST[2][5] = {{152, -212, 288, 14, 0.9f}, {190, -42, 288, 14, 0.45f}};
#endif
      for (auto &q : DARK)
        sc_rrect(c.it[n++], ax + (q[1] + q[2]) / 2 * k, ay + q[0] * k, (q[2] - q[1]) * k, q[3] * k, q[3] * k / 2, L.color3, a, v);
      halo_roll_layout(R.roll, ax + SCQ(4, 4) * k, ay + SCQ(-14, -38) * k, L.color, a, nullptr, v);
      for (auto &q : FAST)
        sc_rrect(c.it[n++], ax + (q[1] + q[2]) / 2 * k, ay + q[0] * k, (q[2] - q[1]) * k, q[3] * k, q[3] * k / 2, L.color2, sa * q[4], v);
      halo_roll_layout(c.c, ax + SCQ(205, 450) * k, ay + SCQ(72, 170) * k, L.color2, sa, on, v);
      sc_box_roll(box, R.roll), sc_box_roll(box, c.c);
      break;
    }
#endif
#if SC_ON(SC_TERMINAL)
    case SC_TERMINAL: {
      const float x0 = ax + SCQ(-177, -420) * k;
      strlcpy(c.txt[0], SCQ("$ clock", "$ clock --live"), 16);
      sc_text(c.it[n++], L.font3, c.txt[0], x0, ay + SCQ(-91, -197) * k, c.g[0], 0, L.color2, a, v);
      halo_roll_layout(R.roll, x0, ay + SCQ(-4, 0) * k, L.color, a, nullptr, v);
      const float sy = ay + SCQ(85, 225) * k, cw = c.g[1], s2 = L.font2 ? L.font2->size : 0;
      halo_roll_layout(c.c, x0, sy, L.color, sa, on, v);
      const bool blink = (halo_ms() / 530) % 2 == 0;
      sc_rrect(c.it[n++], x0 + 3 * cw + 0.3f * cw + cw * 0.475f, sy + 0.06f * s2, cw * 0.95f, s2 * 0.9f, 0, L.color, blink ? sa : 0, v);
      sc_box_roll(box, R.roll), sc_box_roll(box, c.c);
      break;
    }
#endif
#if SC_ON(SC_LCD)
    case SC_LCD: {
      // six digits of seven segments (rounded bars): lit — color with a soft glow, dark — color2
      char d[7];
      const bool blank = !fc_h24 && c.hh[0] == '0';
      d[0] = blank ? ' ' : c.hh[0], d[1] = c.hh[1], d[2] = c.mm[0], d[3] = c.mm[1];
      d[4] = '0' + sc_sec_i / 10, d[5] = '0' + sc_sec_i % 10;
      static const char *const SEGS[11] = {"abcdef", "bc", "abged", "abgcd", "fgbc", "afgcd", "afgedc", "abc", "abcdefg", "abcdfg", ""};
#ifdef HALO_ROUND
      const float X0 = -174, Y0 = -96, W0 = 70, H0 = 140, T0 = 14, GAP = 14, CW = 44, SX = 121, SY = 72, SW = 28, SH = 54, ST = 7;
#else
      const float X0 = -422, Y0 = -208, W0 = 170, H0 = 340, T0 = 32, GAP = 34, CW = 96, SX = 288, SY = 166, SW = 54, SH = 104, ST = 13;
#endif
      float x = ax + X0 * k;
      for (int j = 0; j < 6; j++) {
        const bool small = j >= 4;
        const float w = (small ? SW : W0) * k, h = (small ? SH : H0) * k, th = (small ? ST : T0) * k;
        float x_at = x, y = ay + Y0 * k;
        if (small) x_at = ax + (SX + (j - 4) * (SW + ST * 0.9f)) * k, y = ay + SY * k;
        const char ch = d[j];
        const char *on_segs = SEGS[ch >= '0' && ch <= '9' ? ch - '0' : 10];
        const float hx0 = x_at + th / 2, hx1 = x_at + w - th / 2, vy0 = y + th / 2, vm = y + h / 2, vy1 = y + h - th / 2, gp = th * 0.62f;
        for (int s = 0; s < 7; s++) {
          const char sg = "abcdefg"[s];
          float cx, cy, sw, sh;
          if (sg == 'a' || sg == 'g' || sg == 'd') {
            cx = (hx0 + hx1) / 2, cy = sg == 'a' ? vy0 : sg == 'g' ? vm : vy1, sw = hx1 - hx0 - 2 * gp, sh = th;
          } else {
            const bool right = sg == 'b' || sg == 'c', upper = sg == 'b' || sg == 'f';
            cx = right ? hx1 : hx0, cy = upper ? (vy0 + vm) / 2 : (vm + vy1) / 2, sw = th, sh = vm - vy0 - 2 * gp;
          }
          const bool lit = strchr(on_segs, sg) != nullptr;
          const float al = small ? sa : a, gl = th * 0.45f;
          sc_rrect(c.it[n++], cx, cy, sw + 2 * gl, sh + 2 * gl, th / 2 + gl, L.color, lit ? al * 0.22f : 0, v);
          sc_rrect(c.it[n++], cx, cy, sw, sh, th / 2, lit ? L.color : L.color2, al, v);
        }
        if (!small) x += w + GAP * k;
        if (j == 1) {  // the colon: two squares in the middle of its cell
          const float q = T0 * k * 1.05f, cx = x - GAP * k + CW * k / 2;
          for (int e = 0; e < 2; e++) {
            const float cy = ay + (Y0 + H0 * (e ? 0.7f : 0.3f)) * k;
            sc_rrect(c.it[n++], cx, cy, q * 1.9f, q * 1.9f, q * 0.6f, L.color, a * 0.22f, v);
            sc_rrect(c.it[n++], cx, cy, q, q, q * 0.2f, L.color, a, v);
          }
          x += (CW - GAP) * k;
        }
      }
      strlcpy(c.txt[0], fc_hour < 12 ? "AM" : "PM", 8);
      sc_text(c.it[n++], L.font, c.txt[0], ax + SCQ(-174, -408) * k, ay + SCQ(111, 243) * k, c.g[0], 0, L.color, fc_h24 ? 0 : a, v);
      break;
    }
#endif
#if SC_ON(SC_NEON)
    case SC_NEON: {
      static const float FLICK[10] = {0.08f, 1, 0.15f, 0.9f, 0.05f, 1, 0.5f, 1, 1, 1};
      const float g = SCQ(20, 46) * k, t = c.mm_at && !fc_still ? (halo_ms() - c.mm_at) / SC_FLIP_MS : 1;
      const float fm = t < 1 ? FLICK[min(9, (int)(t * 10))] : 1, gi = g + c.gz, cx = ax + c.sh.x;  // (sc_center)
      halo_roll_layout(R.roll, cx - gi, ay, L.color, a, nullptr, v);
      halo_roll_layout(c.b, cx + gi, ay, L.color2, a * fm, nullptr, v);
      const bool colon = (halo_ms() / 500) % 2 == 0 || fc_calm;
      const float r = SCQ(6, 13) * k;
      for (int e = 0; e < 2; e++) {
        const float y = ay + (e ? 0.95f : -0.95f) * g;
        sc_circle(c.it[n++], cx, y, r * 3.2f, L.color, colon ? a * 0.16f : 0, v);
        sc_circle(c.it[n++], cx, y, r * 1.9f, L.color, colon ? a * 0.38f : 0, v);
        sc_circle(c.it[n++], cx, y, r, sc_mix(L.color, 0xFFFFFF, 0.82f), colon ? a : 0, v);
      }
      c.col[0] = sc_mix(L.color, 0xFFFFFF, 0.85f), c.col[1] = sc_mix(L.color2, 0xFFFFFF, 0.85f);
      sc_box_roll(box, R.roll), sc_box_roll(box, c.b);
      break;
    }
#endif
#if SC_ON(SC_SUNSET)
    case SC_SUNSET: {
      // the time in a gradient (color → color2 → color3, top to bottom), its reflection below, fading, with ripples
      const float cy = ay + SCQ(-40, -64) * k, size = L.font ? L.font->size : 0;
      halo_roll_layout(R.roll, ax, cy, sc_key(L), a, nullptr, v);
      const float H = size * 0.728f * v.s, top = halo_view_y(v, cy) - H / 2, base = top + H, gap = SCQ(10, 18) * k * v.s;
      c.d[0] = top, c.d[1] = H, c.d[2] = gap, c.col[0] = L.color, c.col[1] = L.color2, c.col[2] = L.color3;
      c.nrip = 0;
      const float fs = size * v.s;
      float y = 4;
      for (int j = 0; j < SC_RIPPLES && y < H; j++) {  // the ripples: lines a little wider and further apart downwards
        const float th = fs * (0.01f + j * 0.002f);
        c.rip[c.nrip][0] = (int16_t)lroundf(base + gap + y), c.rip[c.nrip][1] = (int16_t)lroundf(base + gap + y + fmaxf(1, th));
        c.nrip++;
        y += fs * (0.035f + j * 0.006f);
      }
      // the reflection: the letters' boxes upside down under the line (while they roll — every frame)
      float rb[4] = {0, 0, -1, -1};
      for (int j = 0; j < R.roll.n; j++)
        for (const HaloItem *it : {&R.roll.c[j].in, &R.roll.c[j].out})
          if (it->shown) sc_box_add(rb, it->b[0], 2 * base + gap - it->b[3], it->b[2], 2 * base + gap - it->b[1]);
      uint32_t h = sc_hash(sc_hash(sc_hash(sc_hashu(2166136261u, sc_key(L)), top), gap), c.a * 64);
      if (halo_roll_busy(R.roll)) h = sc_hashu(h, halo_ms());
      for (int j = 0; j < R.roll.n; j++) h = sc_hashu(h, R.roll.c[j].code);
      sc_area(c.area[0], c.a > 0.004f && rb[2] >= rb[0], rb[0], rb[1], rb[2], fminf(rb[3], base + gap + H * 0.72f), h);
      sc_box_roll(box, R.roll), sc_box_add(box, rb[0], rb[1], rb[2], rb[3]);
      break;
    }
#endif
#if SC_ON(SC_SHADOW)
    case SC_SHADOW: {
      // the hours, the colon (color2), the minutes, each with its long shadow (color3) from font2
      // (the colon: where the time is centered by its ink, sc_center — less half its shadow, as before)
      const float x = ax + c.sh.x + SCQ(-14, -20) * k, y = ay + SCQ(-16, -26) * k, g = c.g[3] + c.gz;
      halo_roll_layout(R.roll, x - g, y, L.color, a, nullptr, v);
      halo_roll_layout(c.b, x + g, y, L.color, a, nullptr, v);
      HaloItem &cl = c.it[n++];
      const HaloMask *cg = L.font ? halo_glyph(*L.font, ':') : nullptr;
      cl.kind = HI_MASK, cl.mask = cg, cl.color = L.color2, cl.scale = 1, cl.on = nullptr, cl.on_black = false;
      cl.alpha = cg ? a : 0;
      if (cg) cl.x = x, cl.y = y - R.roll.mid + cg->oy + cg->h / 2.0f;
      halo_view_commit(cl, v);
      float sb[4] = {0, 0, -1, -1};
      uint32_t h = sc_hashu(sc_hash(2166136261u, c.a * 64), L.color3);
      auto shadow_box = [&](const HaloItem &it, uint32_t code) {
        const HaloMask *sg = it.shown && L.font2 && code ? halo_glyph(*L.font2, code) : nullptr;
        if (!sg) return;
        float sx, sy;
        sc_at(it, *sg, sx, sy);
        sc_box_add(sb, sx - 1, sy - 1, sx + sg->w * it.scale + 1, sy + sg->h * it.scale + 1);
        h = sc_hash(sc_hash(sc_hashu(h, code), sx), sy), h = sc_hash(h, it.alpha * 64);
      };
      for (const HaloRoll *r : {&R.roll, &c.b})
        for (int j = 0; j < r->n; j++) shadow_box(r->c[j].in, r->c[j].code), shadow_box(r->c[j].out, r->c[j].old);
      shadow_box(cl, ':');
      sc_area(c.area[0], sb[2] >= sb[0], sb[0], sb[1], sb[2], sb[3], h);
      sc_box_roll(box, R.roll), sc_box_roll(box, c.b), sc_box_add(box, sb[0], sb[1], sb[2], sb[3]);
      break;
    }
#endif
#if SC_ON(SC_LIQUID)
    case SC_LIQUID: {
      // the digits fill with water over the minute: color (its top) → color2 (its bottom), empty — color3; waves
      const float size = L.font ? L.font->size : 0;
      halo_roll_layout(R.roll, ax, ay, sc_key(L), a, nullptr, v);
      float x0, x1;
      if (!halo_roll_ink(R.roll, x0, x1)) x0 = x1 = 0;
      const float H = size * 0.728f * v.s, top = halo_view_y(v, ay) - H / 2, bot = top + H, amp = SCQ(5, 10) * k * v.s;
      const int X0 = halo_floor(halo_view_x(v, ax + x0)) - 2, X1 = halo_floor(halo_view_x(v, ax + x1)) + 3;
      const float lvl = bot + amp - (sc_sec / 60) * (H + 2 * amp);
      // (the phase from the clock modulo 24.166 s — 10 turns of the front wave, 13 of the back one: sinf of a huge angle
      // is slow, it grew with the uptime)
      const float ph = (halo_ms() % 24166u) / 1000.0f * 2.6f, kw = 2 * HALO_PI / (size * k * v.s * 1.1f + 1);
      const int wn = max(0, X1 - X0 + 1);
      if (wn > c.wave_n) free(c.wave), c.wave = (float *)malloc(sizeof(float) * 2 * wn), c.wave_n = c.wave ? wn : 0;
      float lo = 1e9f, hi = -1e9f;
      if (c.wave && wn <= c.wave_n) {
        c.wave_x0 = X0;
        // both waves along the columns by turning a unit vector a column at a time
        const float cs = cosf(kw), sn = sinf(kw), a0 = kw * X0 + ph, a1 = kw * X0 + 1.3f * ph + 2;
        float s0 = sinf(a0), c0 = cosf(a0), s1 = sinf(a1), c1 = cosf(a1);
        for (int x = 0; x < wn; x++) {
          const float f = lvl + amp * s0, b = lvl - 0.7f * amp + 0.8f * amp * s1;
          c.wave[2 * x] = f, c.wave[2 * x + 1] = b, lo = fminf(lo, fminf(f, b)), hi = fmaxf(hi, fmaxf(f, b));
          const float t0 = s0 * cs + c0 * sn, t1 = s1 * cs + c1 * sn;
          c0 = c0 * cs - s0 * sn, c1 = c1 * cs - s1 * sn, s0 = t0, s1 = t1;
        }
      }
      c.d[0] = top, c.d[1] = H, c.d[2] = wn, c.col[0] = L.color, c.col[1] = L.color2, c.col[2] = L.color3;
      c.col[3] = sc_mix(sc_mix(L.color, L.color2, 0.5f), L.color3, 0.5f);
      // what changes each frame: the band of the water line since the last frame; a new minute (the level back at the
      // bottom) — all of it
      if (c.a > 0.004f && !fc_calm) {
        if (sc_sec < c.last_sec || c.d[4] < c.d[3]) halo_damage_f(X0, top - 2, X1, bot + 2);
        else halo_damage_f(X0, fmaxf(top, fminf(lo, c.d[3])) - 2, X1, fminf(bot, fmaxf(hi, c.d[4])) + 2);
      }
      c.last_sec = sc_sec, c.d[3] = lo, c.d[4] = hi;
      sc_box_roll(box, R.roll);
      break;
    }
#endif
#if SC_ON(SC_FLIP)
    case SC_FLIP: {
      // two cards (color2), the digits (color); a change — the top half turns down over 0.6 s
      for (int e = 0; e < 2; e++) {
        const float x = ax + (e ? SCQ(7, 15) : SCQ(-193, -435)) * k, y = ay + SCQ(-107, -235) * k;
        const float w = SCQ(186, 420) * k, hh = SCQ(214, 470) * k;
        const float X = halo_view_x(v, x), Y = halo_view_y(v, y), Wd = w * v.s, Hd = hh * v.s;
        const uint32_t at = e ? c.mm_at : c.hh_at;
        const float p = !at || fc_still ? 1 : halo_clamp01((halo_ms() - at) / SC_FLIP_MS);
        c.d[e * 5 + 0] = X, c.d[e * 5 + 1] = Y, c.d[e * 5 + 2] = Wd, c.d[e * 5 + 3] = Hd, c.d[e * 5 + 4] = p;
        uint32_t h = sc_hashu(sc_hashu(sc_hashu(sc_hash(sc_hash(2166136261u, X), Y), (uint32_t)lroundf(Wd)), L.color), L.color2);
        h = sc_hash(sc_hash(h, p * 512), c.a * 64);
        for (const char *q = e ? c.mm : c.hh; *q; q++) h = sc_hashu(h, (uint8_t)*q);
        for (const char *q = e ? c.old_mm : c.old_hh; *q; q++) h = sc_hashu(h, (uint8_t)*q);
        sc_area(c.area[e], c.a > 0.004f, X - 2, Y - 2, X + Wd + 2, Y + Hd + 2, h);
        sc_box_add(box, X, Y, X + Wd, Y + Hd);
      }
      c.d[10] = SCQ(20, 38) * k * v.s, c.d[11] = v.s;  // (the font is made at its size already)
      break;
    }
#endif
#if SC_ON(SC_COMET)
    case SC_COMET: {
      // the time; sixty dots round the edge (color3), the seconds — a comet with its tail (color2)
      halo_roll_layout(R.roll, ax, ay, L.color, a, on, v);
      const uint32_t major = sc_mix(L.color3, 0xFFFFFF, 0.25f);
#ifdef HALO_ROUND
      const int TAIL = 40;
      auto at = [&](float u, float &x, float &y) { x = ax + 218 * k * sinf(u * 2 * HALO_PI), y = ay - 218 * k * cosf(u * 2 * HALO_PI); };
      const float dot = 1.6f * k, big = 2.8f * k, du = 1.9f / 360, hr = 8 * k, head_r = 6.5f * k, halo_r = 18 * k;
#else
      const int TAIL = 50;
      // the screen's frame: inset 30, corners 58, from the top middle clockwise
      auto at = [&](float u, float &x, float &y) {
        const float x0 = 30, y0 = 30, x1 = W - 30, y1 = HALO_H - 30, r = 58, q = HALO_PI * r / 2;
        const float sides[8] = {x1 - r - CX, q, y1 - y0 - 2 * r, q, x1 - x0 - 2 * r, q, y1 - y0 - 2 * r, q};
        float per = x1 - x0 - 2 * r - (x1 - r - CX);
        for (float s : sides) per += s;
        float d = (u - floorf(u)) * per;
        const float lead = x1 - r - CX;
        if (d < lead) return (void)(x = CX + d, y = y0);
        d -= lead;
        const float cxs[4] = {x1 - r, x1 - r, x0 + r, x0 + r}, cys[4] = {y0 + r, y1 - r, y1 - r, y0 + r};
        for (int s = 0; s < 4; s++) {
          if (d < q) {
            return dl_at(cxs[s], cys[s], s * 90 + d / q * 90, r, x, y);
          }
          d -= q;
          const float len = s % 2 == 0 ? y1 - y0 - 2 * r : x1 - x0 - 2 * r;
          if (d < len || s == 3) {
            if (s == 0) return (void)(x = x1, y = y0 + r + d);
            if (s == 1) return (void)(x = x1 - r - d, y = y1);
            if (s == 2) return (void)(x = x0, y = y1 - r - d);
            return (void)(x = x0 + r + d, y = y0);
          }
          d -= len;
        }
        x = CX, y = y0;
      };
      const float dot = 2.4f * k, big = 4 * k, du = 0.0034f, hr = 13 * k, head_r = 9.5f * k, halo_r = 30 * k;
#endif
      for (int j = 0; j < 60; j++) {
        float x, y;
        at(j / 60.0f, x, y);
#ifndef HALO_ROUND
        x = ax + (x - CX) * k, y = ay + (y - CY) * k;  // (the frame round the anchor, its size times k)
#endif
        sc_circle(c.it[n++], x, y, j % 5 ? dot : big, j % 5 ? L.color3 : major, a, v);
      }
      // (a pixel at a time along its path: it moves ~23 px a second — redrawn then, not every frame; on the round watch
      // a frame follows the panel's scan, and the tail's rows cost it)
      const float steps = SCQ(2 * HALO_PI * 218, 2908) * k, u = floorf(sc_sec / 60 * steps) / steps;
      for (int j = TAIL; j >= 1; j--) {
        float x, y;
        at(u - j * du, x, y);
#ifndef HALO_ROUND
        x = ax + (x - CX) * k, y = ay + (y - CY) * k;  // (the frame round the anchor, its size times k)
#endif
        const float t = 1 - (float)j / TAIL;
        sc_circle(c.it[n++], x, y, k + (head_r - k) * t, sc_mix(sc_mix(L.color2, 0x000000, 0.55f), L.color2, t), sa * t * t, v);
      }
      float hx, hy;
      at(u, hx, hy);
#ifndef HALO_ROUND
      hx = ax + (hx - CX) * k, hy = ay + (hy - CY) * k;
#endif
      sc_circle(c.it[n++], hx, hy, halo_r, L.color2, sa * 0.22f, v);
      sc_circle(c.it[n++], hx, hy, hr * 1.6f, sc_mix(L.color2, 0xFFF1DA, 0.6f), sa * 0.45f, v);
      sc_circle(c.it[n++], hx, hy, hr, 0xFFF1DA, sa, v);
      sc_box_roll(box, R.roll);
      break;
    }
#endif
#if SC_ON(SC_HANDS)
    case SC_HANDS: {
      // a dial: hour marks (color; 12 o'clock — color2), minute dots (color3), the hands (color), the second hand (color2)
      const float r = SCQ(226, 284) * k;
      int nc = 0;
      for (int j = 0; j < 60; j++) {
        const DlDir d = dl_dir(j * 6);
        float x0, y0, x1, y1;
        if (j % 5) {
          dl_at(ax, ay, d, r * 0.93f, x0, y0);
          sc_circle(c.it[n++], x0, y0, r * 0.009f, L.color3, a, v);
        } else {
          dl_at(ax, ay, d, r * (j % 15 ? 0.86f : 0.81f), x0, y0), dl_at(ax, ay, d, r * 0.95f, x1, y1);
          sc_cap(c, nc++, x0, y0, x1, y1, r * (j % 15 ? 0.011f : 0.015f), j ? L.color : L.color2, a, v);
        }
      }
      const float ha = sc_hour_deg(), ma = sc_min_deg();
      auto hand = [&](float deg, float r0, float r1, float hw, uint32_t col, float al) {
        const DlDir d = dl_dir(deg);
        float x0, y0, x1, y1;
        dl_at(ax, ay, d, r0, x0, y0), dl_at(ax, ay, d, r1, x1, y1);
        sc_cap(c, nc++, x0, y0, x1, y1, hw, col, al, v);
      };
      hand(ha, -r * 0.06f, r * 0.5f, r * 0.032f, L.color, a);
      hand(ma, -r * 0.06f, r * 0.78f, r * 0.021f, L.color, a);
      const float tip = 2 * HALO_PI * r * 0.86f;  // (the second hand: its tip a pixel at a time, ~20 times a second)
      hand(floorf(sc_sec / 60 * tip) / tip * 360, -r * 0.17f, r * 0.86f, r * 0.0065f, L.color2, sa);
      c.ncap = nc;
      sc_circle(c.it[n++], ax, ay, r * 0.055f, L.color, a, v);
      sc_circle(c.it[n++], ax, ay, r * 0.034f, L.color2, sa, v);
      sc_circle(c.it[n++], ax, ay, r * 0.012f, 0x000000, sa, v);
      sc_hide_caps(c, nc);
      sc_box_add(box, halo_view_x(v, ax - r), halo_view_y(v, ay - r), halo_view_x(v, ax + r), halo_view_y(v, ay + r));
      break;
    }
#endif
#if SC_ON(SC_PILOT)
    case SC_PILOT: {
      // A pilot's dial (Pixel Pilot's family, our own): four big numerals at 12 3 6 9, a minute track at the rim with
      // the five-minute marks in the accent, a framed date between the middle and 3, a hollow hour hand and a solid
      // minute hand. color — the numerals and hands; color2 — the accent (marks, the date's frame, the pivot); color3 —
      // the fine ticks
      const float R = 228 * k;
      const uint32_t light = L.color, acc = L.color2;
      sc_ob = ob;  // (over a scene or another layer: blended)
      const int dial = n;
      int nc = 0;
      for (int j = 0; j < 60; j++) {
        if (j % 15 == 0) continue;  // (the numerals stand there)
        const bool big = j % 5 == 0;
        float x0, y0, x1, y1;
        dl_at(ax, ay, j * 6, R * (big ? 0.90f : 0.935f), x0, y0), dl_at(ax, ay, j * 6, R * 0.975f, x1, y1);
        sc_cap(c, nc++, x0, y0, x1, y1, (big ? 2.6f : 1.0f) * k, big ? acc : L.color3, a, v);
      }
      const int ticks = nc;
      static const char *const NUM[4] = {"12", "3", "6", "9"};
      for (int q = 0; q < 4; q++) {
        float x, y;
        dl_at(ax, ay, q * 90, R * (q % 2 ? 0.80f : 0.78f), x, y);
        dl_text(c.it[n++], L.font, NUM[q], x, y, c.g[0], 1, light, a, v, &halo_black);
      }
      snprintf(c.txt[0], 8, "%d", sc_tm.tm_mday);
      const float dx = ax + R * 0.50f, dw = R * 0.31f, dh = R * 0.22f;  // (the date reads at a glance)
      sc_rrect(c.it[n++], dx, ay, dw, dh, R * 0.035f, acc, a, v);
      sc_rrect(c.it[n++], dx, ay, dw - 5 * k, dh - 5 * k, R * 0.025f, 0x000000, a, v);
      dl_text(c.it[n++], L.font2, c.txt[0], dx, ay, c.g[1], 1, light, a, v, &c.it[n - 1]);
      nc = dl_hands(c, nc, ax, ay, R, light, 0x000000, acc, R * 0.52f, R * 0.84f, a, sa, v);
      c.ncap = nc, c.d[10] = dial, c.d[11] = ticks;
      sc_hide_caps(c, nc);
      sc_box_add(box, halo_view_x(v, ax - R), halo_view_y(v, ay - R), halo_view_x(v, ax + R), halo_view_y(v, ay + R));
      break;
    }
#endif
#if SC_ON(SC_ABOLD)
    case SC_ABOLD: {
      // Analogue bold (our own): two giant numerals, 12 and 6, in the accent's light tone behind slim solid hands; a
      // round complication either side — the weather at 9, the date at 3. color — the numerals; color2 — the hands'
      // pivot and the labels; color3 — the hands and values
      const float R = 228 * k, cr = R * 0.19f;
      const uint32_t light = L.color3, acc = L.color2;
      sc_ob = ob;  // (over a scene or another layer: blended)
      dl_text(c.it[n++], L.font, "12", ax, ay - R * 0.56f, c.g[0], 1, L.color, a, v, &halo_black);
      dl_text(c.it[n++], L.font, "6", ax, ay + R * 0.58f, c.g[0], 1, L.color, a, v, &halo_black);
      HaloItem *disc[2];
      for (int s = 0; s < 2; s++) {
        disc[s] = &c.it[n++];
        sc_circle(*disc[s], ax + (s ? 1 : -1) * R * 0.60f, ay, cr, 0x1E1F22, a, v);
      }
      const WxCity &w = wx_city[0];
      if (w.now == WX_NONE) strlcpy(c.txt[0], "--", 8);
      else snprintf(c.txt[0], 8, "%d\xC2\xB0", (int)w.now);
      strlcpy(c.txt[1], MONTH[sc_tm.tm_mon % 12], 8), snprintf(c.txt[2], 8, "%d", sc_tm.tm_mday);
      const float lx = ax - R * 0.60f, rx = ax + R * 0.60f;
      // (one value a disc, nothing else: the weather now, the day — the user's rule, 23 Nov 2026)
      dl_text(c.it[n++], L.font2, c.txt[0], lx, ay, c.g[1], 1, light, a, v, disc[0]);
      dl_text(c.it[n++], L.font2, c.txt[2], rx, ay, c.g[1], 1, acc, a, v, disc[1]);
      sc_ob = false;
      const int dial = n;
      int nc = dl_hands(c, 0, ax, ay, R, light, 0x000000, acc, R * 0.50f, R * 0.86f, a, sa, v, 0, false);
      c.ncap = nc, c.d[10] = dial, c.d[11] = 0;
      sc_hide_caps(c, nc);
      sc_box_add(box, halo_view_x(v, ax - R), halo_view_y(v, ay - R), halo_view_x(v, ax + R), halo_view_y(v, ay + R));
      break;
    }
#endif
#if SC_ON(SC_CONCENTRIC)
    case SC_CONCENTRIC: {
      // Concentric (our own): a ring of minutes that turns so the minute now sits at 3 o'clock, in a framed slot; the
      // hour big in the middle. It moves once a minute (each number its own place: the ring is redrawn then, never in
      // between). color — the hour and the slot's minutes; color2 — the slot's frame; color3 — the ring
      const float R = 228 * k;
      const int mnow = sc_tm.tm_min;
      sc_ob = ob;  // (over a scene or another layer: blended)
      const float sw = R * 0.40f, sh = R * 0.27f, sx = ax + R * 0.74f;
      sc_rrect(c.it[n++], sx, ay, sw, sh, sh / 2, L.color2, a, v);
      sc_rrect(c.it[n++], sx, ay, sw - 6 * k, sh - 6 * k, sh / 2 - 3 * k, 0x000000, a, v);
      snprintf(c.txt[0], 8, "%02d", mnow);
      dl_text(c.it[n++], L.font3, c.txt[0], sx, ay, c.g[2], 1, L.color, a, v, &c.it[1]);
      snprintf(c.txt[1], 8, "%02d", fc_shown_hour(sc_tm.tm_hour));
      dl_text(c.it[n++], L.font, c.txt[1], ax - R * 0.17f, ay, c.g[0], 1, L.color, a, v, &halo_black);
      int t = 2;
      for (int m = 0; m < 60; m += 5) {  // the numbers, upright, every five minutes — none in the slot
        const float d = 90 + (m - mnow) * 6;
        float off = fmodf(d - 90 + 540, 360) - 180;
        if (fabsf(off) < 16) continue;
        snprintf(c.txt[t], 8, "%02d", m);
        float x, y;
        dl_at(ax, ay, d, R * 0.84f, x, y);
        dl_text(c.it[n++], L.font2, c.txt[t++], x, y, c.g[1], 1, L.color3, a, v, &halo_black);
      }
      sc_ob = false;
      const int dial = n;
      int nc = 0;
      for (int m = 0; m < 60; m++) {  // the minute ticks between them at the rim
        if (m % 5 == 0) continue;
        const float d = 90 + (m - mnow) * 6;
        if (fabsf(fmodf(d - 90 + 540, 360) - 180) < 16) continue;
        float x0, y0, x1, y1;
        dl_at(ax, ay, d, R * 0.95f, x0, y0), dl_at(ax, ay, d, R * 0.985f, x1, y1);
        sc_cap(c, nc++, x0, y0, x1, y1, 1.1f * k, L.color3, a * 0.6f, v);
      }
      c.ncap = nc, c.d[10] = dial, c.d[11] = nc;
      sc_hide_caps(c, nc);
      sc_box_add(box, halo_view_x(v, ax - R), halo_view_y(v, ay - R), halo_view_x(v, ax + R), halo_view_y(v, ay + R));
      break;
    }
#endif
#if SC_ON(SC_WEAVE) || SC_ON(SC_EYES) || SC_ON(SC_PLUS) || SC_ON(SC_RINGS)
    case SC_WEAVE:
    case SC_EYES:
    case SC_PLUS:
    case SC_RINGS: {
      // The textures (wv_*): a field driven by the minute hand's tip, eased once a minute; the hands on top.
      // color — the field; color2 — the pivot; color3 — the hands
      const float R = 228 * k;
      wv_update(look, ax, ay, R, k, L.color);
      c.d[12] = a;
      sc_ob = false;
      const int dial = n;
      int nc = dl_hands(c, 0, ax, ay, R, L.color3, 0x000000, L.color2, R * 0.48f, R * 0.80f, a, sa, v, 0, false);
      c.ncap = nc, c.d[10] = dial, c.d[11] = 0;
      sc_hide_caps(c, nc);
      sc_box_add(box, halo_view_x(v, ax - R), halo_view_y(v, ay - R), halo_view_x(v, ax + R), halo_view_y(v, ay + R));
      break;
    }
#endif
#if SC_ON(SC_WORDS)
    case SC_WORDS: {
      // Words (our own): the time written out, left-aligned — the hour bold in the accent, the minutes in a light weight
      // under it ("Ten / oh / nine", "Four / forty / two", "Six / o'clock"). color — the minutes; color2 — the hour
      static const char *const ONES[20] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
                                           "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
                                           "seventeen", "eighteen", "nineteen"};
      static const char *const TENS[6] = {"", "", "twenty", "thirty", "forty", "fifty"};
      static const char *const HOURS[12] = {"Twelve", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight",
                                            "Nine", "Ten", "Eleven"};
      const int h = sc_tm.tm_hour % 12, m = sc_tm.tm_min;
      const char *l2 = "o'clock", *l3 = nullptr;
      if (m > 0 && m < 10) l2 = "oh", l3 = ONES[m];
      else if (m >= 10 && m < 20) l2 = ONES[m];
      else if (m >= 20) l2 = TENS[m / 10], l3 = m % 10 ? ONES[m % 10] : nullptr;
      const float lh = 80 * k, x = ax - 170 * k, y0 = ay - (l3 ? lh : lh / 2);
      sc_ob = ob;  // (over a scene or another layer: blended)
      dl_text(c.it[n++], L.font, HOURS[h], x, y0, c.g[0], 0, L.color2, a, v, &halo_black);
      dl_text(c.it[n++], L.font2, l2, x, y0 + lh, c.g[1], 0, L.color, a, v, &halo_black);
      dl_text(c.it[n++], L.font2, l3 ? l3 : "", x, y0 + 2 * lh, c.g[1], 0, L.color, l3 ? a : 0, v, &halo_black);
      c.ncap = 0, c.d[10] = n, c.d[11] = 0;
      sc_hide_caps(c, 0);
      break;
    }
#endif
#if SC_ON(SC_BIGTYPE)
    case SC_BIGTYPE: {
      // Big type (our own): the hours over the minutes, heavy and round, filling the dial — two tones of one hue.
      // color — the hours; color2 — the minutes
      snprintf(c.txt[0], 8, "%02d", fc_shown_hour(sc_tm.tm_hour));
      snprintf(c.txt[1], 8, "%02d", sc_tm.tm_min);
      sc_ob = ob;  // (over a scene or another layer: blended)
      dl_text(c.it[n++], L.font, c.txt[0], ax, ay - 94 * k, c.g[0], 1, L.color, a, v, &halo_black);
      dl_text(c.it[n++], L.font, c.txt[1], ax, ay + 96 * k, c.g[0], 1, L.color2, a, v, &halo_black);
      c.ncap = 0, c.d[10] = n, c.d[11] = 0;
      sc_hide_caps(c, 0);
      break;
    }
#endif
#if SC_ON(SC_BEZEL)
    case SC_BEZEL: {
      // the minute scale (color3) turns so that the minute is under the marker (color2); the hour (color) in the middle
      const float mm = sc_tm.tm_min + sc_sec / 60;
      strlcpy(c.txt[15], c.mm, 8);
#ifdef HALO_ROUND
      const float rot = -lroundf(mm * 60) / 10.0f;  // (degrees, in tenths: it moves 0.1° a second — redrawn then)
      const uint32_t minor = sc_mix(L.color3, 0x000000, 0.6f);
      int nc = 0;
      for (int j = 0; j < 60; j++) {
        const DlDir d = dl_dir(j * 6 + rot);
        const bool big = j % 5 == 0;
        float x0, y0, x1, y1;
        dl_at(ax, ay, d, (big ? 198 : 210) * k, x0, y0), dl_at(ax, ay, d, 222 * k, x1, y1);
        sc_cap(c, nc++, x0, y0, x1, y1, big ? 1.6f * k : 1.0f * k, big ? L.color3 : minor, a, v);
        if (big) {
          snprintf(c.txt[j / 5], 8, "%02d", j);
          dl_at(ax, ay, d, 176 * k, x0, y0);
          sc_text(c.it[n++], L.font2, c.txt[j / 5], x0, y0, c.g[0], 1, L.color3, a, v);
        }
      }
      sc_cap(c, nc++, ax, ay - 231 * k, ax, ay - 190 * k, 2.5f * k, L.color2, a, v);
      c.ncap = nc;
      sc_hide_caps(c, nc);
      sc_text(c.it[n++], L.font3, c.txt[15], ax, ay - 118 * k, c.g[1], 1, L.color2, a, v);
      halo_roll_layout(R.roll, ax, ay + 18 * k, L.color, a, on, v);
      sc_box_add(box, halo_view_x(v, ax - 224 * k), halo_view_y(v, ay - 224 * k), halo_view_x(v, ax + 224 * k), halo_view_y(v, ay + 224 * k));
#else
      // a ruler under the hour: the minutes slide sideways (in quarter pixels)
      const float px = 26 * k, yb = ay + 186 * k, off = lroundf(mm * px * 4) / 4.0f / px;
      const uint32_t minor = sc_mix(L.color3, 0x000000, 0.6f);
      int words = 0;
      for (int j = (int)floorf(off - 21); j <= (int)ceilf(off + 21) && n < 60; j++) {
        const float x = ax + (j - off) * px, dd = fabsf(x - ax) / (520 * k), op = fmaxf(0, 1 - dd * dd * 1.15f);
        const bool big = ((j % 5) + 5) % 5 == 0;
        const float h = (big ? 40 : 20) * k, w = (big ? 4 : 3) * k;
        sc_rrect(c.it[n++], x, yb - h / 2, w, h, w / 2, big ? L.color3 : minor, a * op, v);
        if (big && words < 11) {
          snprintf(c.txt[words], 8, "%02d", ((j % 60) + 60) % 60);
          sc_text(c.it[60 + words], L.font2, c.txt[words], x, yb + 38 * k, c.g[0], 1, L.color3, a * op, v);
          words++;
        }
      }
      for (; n < 60; n++) c.it[n].alpha = 0, halo_view_commit(c.it[n], v);
      for (; words < 11; words++) c.it[60 + words].alpha = 0, halo_view_commit(c.it[60 + words], v);
      n = 71;
      sc_rrect(c.it[n++], ax, yb - 30 * k, 6 * k, 72 * k, 3 * k, L.color2, a, v);
      sc_text(c.it[n++], L.font3, c.txt[15], ax, yb - 104 * k, c.g[1], 1, L.color2, a, v);
      halo_roll_layout(R.roll, ax, ay - 95 * k, L.color, a, on, v);
      sc_box_roll(box, R.roll);
#endif
      break;
    }
#endif
    default:
      break;
  }
  sc_hide(c, n, v);
  for (int j = 0; j < n; j++) box_add(box, c.it[j]);
}

// ---------- Drawing ----------
static void sc_draw_items(const ScRun &c, int from, int to) {
  for (int j = from; j < to; j++) halo_item_draw(c.it[j]);
}
// A roll's letters drawn by fn(item, its letter): the old ones first
template <typename F> static void sc_each(const HaloRoll &r, F fn) {
  for (int j = 0; j < HALO_ROLL_MAX; j++) {
    const HaloRollCell &cl = r.c[j];
    if (cl.out.shown && cl.out.mask) fn(cl.out, cl.old);
    if (cl.in.shown && cl.in.mask) fn(cl.in, cl.code);
  }
}
static inline bool sc_hits(const HaloItem &it) {
  return halo_hits(it.b[0], it.b[1], it.b[2], it.b[3]);
}
#if SC_ON(SC_SUNSET)
static uint32_t sc_grad(const ScRun &c, float y) {  // color → color2 → color3 down the digits' rows
  const float t = halo_clamp01((y - c.d[0]) / fmaxf(1, c.d[1]));
  return t < 0.55f ? sc_mix(c.col[0], c.col[1], t / 0.55f) : sc_mix(c.col[1], c.col[2], (t - 0.55f) / 0.45f);
}
// The same pixels as sc_draw_sunset_slow, a letter a call a band (not a call a row a letter): each row's color is laid
// out once a band, and over the black sky a letter's edge is a ready color, not blending (in a transition the whole clock
// is drawn every frame — this look was the face's heaviest layer: ~7 ms a core a frame)
static void sc_draw_sunset_slow(const FaceLayer &L, const FaceRun &R, const ScRun &c);
static void sc_draw_sunset(const FaceLayer &L, const FaceRun &R, const ScRun &c) {
  if (!c.rowp || hb.rows > HALO_ROWS) return sc_draw_sunset_slow(L, R, c);
  const float top = c.d[0], H = c.d[1], gap = c.d[2], base = top + H;
  HaloPaint *rows = c.rowp + (xPortGetCoreID() & 1) * 2 * HALO_ROWS, *mrows = rows + HALO_ROWS;
  const int y0 = max(hb.y, halo_floor(top) - 1), y1 = min(hb.y + hb.rows - 1, halo_floor(base) + 1);
  // the letters, each row in its color (rows for the letters' alpha: rolling ones fade — rows again for theirs)
  if (y0 <= y1) {
    float ra = -1;
    sc_each(R.roll, [&](const HaloItem &it, uint32_t) {
      if (!sc_hits(it)) return;
      if (it.alpha != ra) {
        ra = it.alpha;
        for (int y = hb.y; y < hb.y + hb.rows; y++) {
          HaloPaint &p = rows[y - hb.y];
          if (y < y0 || y > y1 || ra <= 0) p.A = 0;
          else halo_paint(p, sc_grad(c, y + 0.5f), ra);
        }
      }
      // (only the rows its box touches — as the row-by-row check did)
      const int ya = max(y0, halo_floor(it.b[1])), yb = min(y1, halo_floor(it.b[3]));
      const int gi = L.font ? (int)(it.mask - L.font->glyphs) : -1;
      const uint8_t *full = c.full && gi >= 0 && gi < L.font->count ? c.full[gi] : nullptr;
      if (ya <= yb)
        halo_mask_rows(*it.mask, it.x - it.mask->w * it.scale / 2, it.y - it.mask->h * it.scale / 2, it.scale,
                       rows + (ya - hb.y), ya, yb - ya + 1, full);
    });
  }
  // the reflection: the letters upside down under the line, fading out downwards, cut by the ripples
  if (!c.mir || !L.font) return;
  const int r0 = max(hb.y, halo_floor(base + gap)), r1 = min(hb.y + hb.rows - 1, halo_floor(base + gap + H * 0.72f));
  if (r0 > r1) return;
  float ra = -1;
  sc_each(R.roll, [&](const HaloItem &it, uint32_t) {
    const int gi = (int)(it.mask - L.font->glyphs);
    if (gi < 0 || gi >= L.font->count) return;
    if (it.alpha != ra) {
      ra = it.alpha;
      int rp = 0;
      for (int y = hb.y; y < hb.y + hb.rows; y++) {
        HaloPaint &p = mrows[y - hb.y];
        p.A = 0;
        if (y < r0 || y > r1) continue;
        while (rp < c.nrip && c.rip[rp][1] <= y) rp++;
        if (rp < c.nrip && y >= c.rip[rp][0]) continue;
        const float fade = 0.38f * halo_clamp01(1 - (y + 0.5f - base - gap) / (0.7f * H));
        if (fade <= 0.004f || ra * fade <= 0) continue;
        halo_paint(p, sc_grad(c, 2 * base + gap - (y + 0.5f)), ra * fade);
      }
    }
    const HaloMask &m = c.mir[gi];
    const float s = it.scale, fx = it.x - m.w * s / 2, fy = 2 * base + gap - (it.y + m.h * s / 2);
    halo_mask_rows(m, fx, fy, s, mrows, hb.y, hb.rows, c.full ? c.full[L.font->count + gi] : nullptr);
  });
}
static void sc_draw_sunset_slow(const FaceLayer &L, const FaceRun &R, const ScRun &c) {
  const float top = c.d[0], H = c.d[1], gap = c.d[2], base = top + H;
  const int y0 = max(hb.y, halo_floor(top) - 1), y1 = min(hb.y + hb.rows - 1, halo_floor(base) + 1);
  // the letters: a row at a time, each in its color
  for (int y = y0; y <= y1; y++) {
    ScClip cl(-100000, y, 100000, y);
    const uint32_t col = sc_grad(c, y + 0.5f);
    sc_each(R.roll, [&](const HaloItem &it, uint32_t) {
      if (sc_hits(it)) halo_mask(*it.mask, it.x - it.mask->w * it.scale / 2, it.y - it.mask->h * it.scale / 2, col, it.alpha, it.scale);
    });
  }
  // the reflection: the letters upside down under the line, fading out downwards, cut by the ripples
  if (!c.mir || !L.font) return;
  const int r0 = max(hb.y, halo_floor(base + gap)), r1 = min(hb.y + hb.rows - 1, halo_floor(base + gap + H * 0.72f));
  int rp = 0;
  for (int y = r0; y <= r1; y++) {
    while (rp < c.nrip && c.rip[rp][1] <= y) rp++;
    if (rp < c.nrip && y >= c.rip[rp][0]) continue;
    const float fade = 0.38f * halo_clamp01(1 - (y + 0.5f - base - gap) / (0.7f * H));
    if (fade <= 0.004f) continue;
    ScClip cl(-100000, y, 100000, y);
    const uint32_t col = sc_grad(c, 2 * base + gap - (y + 0.5f));
    sc_each(R.roll, [&](const HaloItem &it, uint32_t) {
      const int gi = (int)(it.mask - L.font->glyphs);
      if (gi < 0 || gi >= L.font->count) return;
      const HaloMask &m = c.mir[gi];
      const float s = it.scale, fx = it.x - m.w * s / 2, fy = 2 * base + gap - (it.y + m.h * s / 2);
      halo_mask(m, fx, fy, col, it.alpha * fade, s);
    });
  }
}
#endif
#if SC_ON(SC_LIQUID)
// A letter of the Liquid look: each pixel — water (by its row), the back wave or the empty glass, by the water lines of
// its column
static void sc_liquid_mask(const ScRun &c, const HaloMask &m, float fx, float fy, float s, float alpha) {
  if (alpha <= 0 || !halo_hits(fx - 1, fy - 1, fx + m.w * s + 1, fy + m.h * s + 1) || !c.wave) return;
  int er, eg, eb, br, bg, bb;
  sc_565(c.col[2], er, eg, eb), sc_565(c.col[3], br, bg, bb);
  const int A = (int)(alpha * 256 + 0.5f), stride = (m.w + 1) / 2, wn = (int)c.d[2];
  const int ya = max(hb.y, halo_floor(fy)), yb = min(hb.y + hb.rows - 1, halo_floor(fy + m.h * s));
  const float inv = 1 / s;
  for (int y = ya; y <= yb; y++) {
    const int j = halo_floor((y + 0.5f - fy) * inv);
    if (j < 0 || j >= m.h || m.ext[2 * j] > m.ext[2 * j + 1]) continue;
    const int e0 = m.ext[2 * j], e1 = m.ext[2 * j + 1] == 255 ? m.w - 1 : m.ext[2 * j + 1];
    const int xa = max(hb.x0, halo_floor(fx + e0 * s)), xb = min(hb.x1, halo_floor(fx + (e1 + 1) * s));
    int wr, wg, wb;
    sc_565(sc_mix(c.col[0], c.col[1], (y + 0.5f - c.d[0]) / fmaxf(1, c.d[1])), wr, wg, wb);
    const uint8_t *bits = m.bits + j * stride;
    uint16_t *row = halo_row(y);
    const float py = y + 0.5f;
    for (int x = xa; x <= xb; x++) {
      const int i = s > 0.999f && s < 1.001f ? x - halo_floor(fx + 0.5f) : halo_floor((x + 0.5f - fx) * inv);
      if (i < 0 || i >= m.w) continue;
      const int v = sc_nib(bits, i);
      if (!v) continue;
      const int w = x - c.wave_x0;
      const float f = w >= 0 && w < wn ? c.wave[2 * w] : 1e9f, b = w >= 0 && w < wn ? c.wave[2 * w + 1] : 1e9f;
      const int a = (v * 17 * A) >> 8;
      if (py >= f) halo_blend(row + x, wr, wg, wb, a);
      else if (py >= b) halo_blend(row + x, br, bg, bb, a);
      else halo_blend(row + x, er, eg, eb, a);
    }
  }
}
#endif
#if SC_ON(SC_FLIP)
static void sc_draw_flip(const FaceLayer &L, const ScRun &c) {
  if (!L.font || c.a <= 0.004f) return;
  const uint32_t top_c = L.color2, bot_c = sc_mix(L.color2, 0x000000, 0.12f);
  const float r = c.d[10], s = c.d[11], mid_ref = c.g[0];
  for (int e = 0; e < 2; e++) {
    const float X = c.d[e * 5], Y = c.d[e * 5 + 1], Wd = c.d[e * 5 + 2], Hd = c.d[e * 5 + 3], p = c.d[e * 5 + 4];
    if (!halo_hits(X - 2, Y - 2, X + Wd + 2, Y + Hd + 2)) continue;
    const float mid = Y + Hd / 2, cx = X + Wd / 2;
    const char *now = e ? c.mm : c.hh, *old = e ? c.old_mm : c.old_hh;
    const int xa = halo_floor(X) - 2, xb = halo_floor(X + Wd) + 2, ym = halo_floor(mid);
    {  // the top half: the new digits
      ScClip cl(xa, halo_floor(Y) - 2, xb, ym - 1);
      halo_rrect(X, Y, X + Wd, Y + Hd, r, top_c, c.a);
      sc_glyphs(*L.font, now, cx, mid, mid_ref, s, 1, mid, L.color, c.a);
    }
    {  // the bottom half: the old ones until the flap lands on them
      ScClip cl(xa, ym, xb, halo_floor(Y + Hd) + 2);
      halo_rrect(X, Y, X + Wd, Y + Hd, r, bot_c, c.a);
      sc_glyphs(*L.font, p < 1 ? old : now, cx, mid, mid_ref, s, 1, mid, L.color, c.a);
    }
    if (p < 1) {  // the flap: the old top half folding down to the line, then the new bottom half opening from it
      const bool first = p < 0.5f;
      const float q = first ? cosf(p / 0.5f * HALO_PI / 2) : sinf((p - 0.5f) / 0.5f * HALO_PI / 2);
      const float y0 = first ? mid - Hd / 2 * q : mid, y1 = first ? mid : mid + Hd / 2 * q;
      ScClip cl(xa, halo_floor(y0), xb, halo_floor(y1) - (first ? 1 : 0));
      halo_rrect(X, mid - Hd / 2 * q, X + Wd, mid + Hd / 2 * q, r * fmaxf(q, 0.2f), first ? top_c : bot_c, c.a);
      sc_glyphs(*L.font, first ? old : now, cx, mid, mid_ref, s, q, mid, L.color, c.a);
      halo_rrect(X, mid - Hd / 2 * q, X + Wd, mid + Hd / 2 * q, 0, 0x000000, c.a * (1 - q) * (first ? 0.55f : 0.45f));
    }
    // the split and its hinges
    halo_rrect(X, mid - Hd * 0.007f, X + Wd, mid + Hd * 0.007f, 0, 0x000000, c.a);
    const float hw = Wd * 0.028f, hh = Hd * 0.09f;
    halo_rrect(X - hw * 0.4f, mid - hh / 2, X + hw * 0.6f, mid + hh / 2, hw / 2, 0x0E0F0F, c.a);
    halo_rrect(X + Wd - hw * 0.6f, mid - hh / 2, X + Wd + hw * 0.4f, mid + hh / 2, hw / 2, 0x0E0F0F, c.a);
  }
}
#endif

static void sc_draw(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  const FaceRun &R = S.run[i];
  const ScRun *cp = R.sc;
  if (!cp) return;
  const ScRun &c = *cp;
  switch ((int)L.p2) {
    case SC_HAIRLINE:
    case SC_TALL:
    case SC_SERIF:
    case SC_STACK:
    case SC_POSTER:
      halo_roll_draw(R.roll), halo_roll_draw(c.b);
      sc_draw_items(c, 0, SC_ITEMS);
      break;
#if SC_ON(SC_SPORT)
    case SC_SPORT:
      sc_draw_items(c, 0, 3), halo_roll_draw(R.roll), sc_draw_items(c, 3, 5), halo_roll_draw(c.c);
      break;
#endif
#if SC_ON(SC_TERMINAL)
    case SC_TERMINAL:  // the glow (font4) under each letter, the letter (font) on it
      sc_draw_items(c, 0, 1);
      sc_each(R.roll, [&](const HaloItem &it, uint32_t code) {
        if (!sc_hits(it)) return;
        const float s = it.scale;
        if (L.font4) halo_mask(*it.mask, it.x - it.mask->w * s / 2, it.y - it.mask->h * s / 2, it.color, it.alpha * 0.5f, s);
        const HaloMask *g = L.font ? halo_glyph(*L.font, code) : nullptr;
        if (!g) return;
        float x, y;
        sc_at(it, *g, x, y);
        halo_mask(*g, x, y, it.color, it.alpha, s);
      });
      halo_roll_draw(c.c), sc_draw_items(c, 1, 2);
      break;
#endif
#if SC_ON(SC_NEON)
    case SC_NEON:  // the glow in the tube's color, the tube (pale) on it
      sc_draw_items(c, 0, 6);
      for (int e = 0; e < 2; e++)
        sc_each(e ? c.b : R.roll, [&](const HaloItem &it, uint32_t code) {
          if (!sc_hits(it)) return;
          const float s = it.scale;
          halo_mask(*it.mask, it.x - it.mask->w * s / 2, it.y - it.mask->h * s / 2, it.color, it.alpha, s);
          const HaloMask *g = L.font ? halo_glyph(*L.font, code) : nullptr;
          if (!g) return;
          float x, y;
          sc_at(it, *g, x, y);
          halo_mask(*g, x, y, c.col[e], it.alpha, s);
        });
      break;
#endif
#if SC_ON(SC_SUNSET)
    case SC_SUNSET:
      sc_draw_sunset(L, R, c);
      break;
#endif
#if SC_ON(SC_SHADOW)
    case SC_SHADOW: {
      auto shadow = [&](const HaloItem &it, uint32_t code) {
        const HaloMask *g = L.font2 && code ? halo_glyph(*L.font2, code) : nullptr;
        if (!g || !it.shown) return;
        float x, y;
        sc_at(it, *g, x, y);
        halo_mask(*g, x, y, L.color3, it.alpha, it.scale);
      };
      sc_each(R.roll, shadow), sc_each(c.b, shadow);
      if (c.it[0].shown && c.it[0].mask) shadow(c.it[0], ':');
      halo_roll_draw(R.roll), halo_roll_draw(c.b), sc_draw_items(c, 0, 1);
      break;
    }
#endif
#if SC_ON(SC_LIQUID)
    case SC_LIQUID:
      sc_each(R.roll, [&](const HaloItem &it, uint32_t) {
        const float s = it.scale;
        sc_liquid_mask(c, *it.mask, it.x - it.mask->w * s / 2, it.y - it.mask->h * s / 2, s, it.alpha);
      });
      break;
#endif
#if SC_ON(SC_FLIP)
    case SC_FLIP:
      sc_draw_flip(L, c);
      break;
#endif
    case SC_PILOT:
    case SC_ABOLD:
    case SC_CONCENTRIC:
    case SC_WEAVE:
    case SC_WORDS:
    case SC_EYES:
    case SC_PLUS:
    case SC_RINGS:
    case SC_BIGTYPE: {  // the dial's items, its ticks, the complications, the hands (dl_*: the order); Weave — its field first
#if SC_ON(SC_WEAVE) || SC_ON(SC_EYES) || SC_ON(SC_PLUS) || SC_ON(SC_RINGS)
      if (((int)L.p2 == SC_WEAVE || (int)L.p2 >= SC_EYES) && wv && c.d[12] > 0.004f)
        for (int j = 0; j < wv_n; j++) {
          ScCap d = wv[j];
          d.a *= c.d[12];
          sc_cap_draw(d);
        }
#endif
      const int dial = (int)c.d[10], ticks = (int)c.d[11];
      sc_draw_items(c, 0, dial);
      for (int j = 0; j < ticks && j < c.ncap; j++) sc_cap_draw(c.cap[j]);
      sc_draw_items(c, dial, SC_ITEMS);
      for (int j = ticks; j < c.ncap; j++) sc_cap_draw(c.cap[j]);
      break;
    }
#if SC_ON(SC_HANDS) || SC_ON(SC_BEZEL)
    case SC_HANDS:
      sc_draw_items(c, 0, 48);
      for (int j = 0; j < c.ncap; j++) {
        if (j == c.ncap - 1) sc_draw_items(c, 48, 49);  // (the hands' middle under the second hand)
        sc_cap_draw(c.cap[j]);
      }
      sc_draw_items(c, 49, SC_ITEMS);
      break;
    case SC_BEZEL:
      for (int j = 0; j < c.ncap; j++) sc_cap_draw(c.cap[j]);
      sc_draw_items(c, 0, SC_ITEMS), halo_roll_draw(R.roll);
      break;
#endif
    default:  // (LCD, Comet)
      sc_draw_items(c, 0, SC_ITEMS), halo_roll_draw(R.roll);
  }
}
