// upager.h — paged app screens (Material 3 Expressive for Wear OS; Google's references and Codex's measured study,
// apps/SPEC.md — 23 Nov 2026): an app is a row of NON-SCROLLING pages swiped left and right (a spring; a third of the
// way or a fling turns the page; rubber past the ends), page dots at the bottom, the time curved along the top, a ring at
// the rim from 26° to 334° (the gap at 12 is the clock's). A swipe from the left edge stays the system's back.
// The parts: the primary button — a filled shape that morphs from a circle to four soft lobes while something runs
// (r(a) = R·(1 − 0.15·t·sin²2a), drawn per pixel, smoothed, only when it changes); tonal secondary buttons; text in one
// type scale (154 / 103 / 35 / 15 px — 0.33, 0.22, 0.075, 0.033 of the dial); picker columns. Each page has its own
// items, laid out through the screen's view shifted by where the page is: a page mid-swipe is the page at rest, moved.
// Round board only. Included from PopkaWatch.ino after ui.h (its helpers) and before clocks.h.
#ifdef HALO_ROUND
extern const HaloFont gl_up_hero, gl_up_disp, gl_up_pick, gl_up_title, gl_up_clock, gl_up_num, gl_up_pre, gl_up_lab,
    gl_up_ic43, gl_up_ic32, gl_up_ic24;  // (studio.py GALLERY_FONTS)
#define UP_PAGES 8
#define UP_IT 64
#define UP_BLOBS 3
#define UP_IC_PLAY "\xEF\x81\x8B"
#define UP_IC_PAUSE "\xEF\x81\x8C"
#define UP_IC_FLAG "\xEF\x80\xA4"
#define UP_IC_RESET "\xEF\x8B\xAA"
#define UP_IC_GRID "\xEF\x80\x8A"
#define UP_IC_X "\xEF\x80\x8D"
#define UP_IC_PLUS "+"
#define UP_IC_SUN "\xEF\x86\x85"
#define UP_IC_MOON "\xEF\x86\x86"
#define UP_IC_GLOBE "\xEF\x82\xAC"
// The study's tokens
#define UP_WHITE 0xF6F5F8
#define UP_MUTED 0xB7B5C1
#define UP_DIM 0x71707D
#define UP_TRACK 0x222228
#define UP_DOT 0x666571
#define UP_ROW 0x19191E
#define UP_RING_R 214.0f
#define UP_RING_W 8.0f
#define UP_A0 (26 * HALO_PI / 180)
#define UP_SWEEP (308 * HALO_PI / 180)
#define UP_TITLE_Y 98.0f
#define UP_DOTS_Y 433.0f

struct UpBlob {  // drawn per pixel, in the page's item order: a primary button's shape, or an app's own painting
  int at;        // drawn before item `at` of its page
  float x, y, r, t, a;  // where (screen, after the view), size (half the box), the morph or a data key, alpha
  uint32_t c;
  void (*paint)(const struct UpBlob &b);  // an app's own (a chart): it paints the band itself; nullptr — the button
  float was[5];  // where it was drawn (x, y, r, t, a): a change — its box drawn again
};
struct UpPage {
  HaloItem it[UP_IT];
  int n, nb;
  UpBlob b[UP_BLOBS];
};
struct UpApp {
  int (*count)();                                              // pages now (1 … UP_PAGES)
  void (*lay)(int p, UpPage &P, float a, const HaloView &v);  // place page p's items (as at rest: v is shifted)
  void (*tap)(int p, float x, float y);                       // a tap on page p (screen coordinates)
  void (*update)(int p, float dt);                            // every frame, the page in front (its own drags)
  float (*ring)();                                             // the rim's progress 0 … 1 (< 0 — the track alone)
  uint32_t accent;
};
static const UpApp *up_app;
static UpPage *up_pg;  // [UP_PAGES], PSRAM
static HaloSpring up_pos;
static HaloSpringK K_UP;
static int up_page, up_owner;
static bool up_drag, up_vis[UP_PAGES];
static float up_x0, up_g0;
static HaloItem up_track, up_fill, up_clock, up_dot[UP_PAGES];
static HaloMask up_clock_mask;
static char up_clock_of[8];
static float up_clock_x, up_clock_y;
static float up_mid_hero, up_mid_disp, up_mid_pick, up_mid_title, up_mid_num, up_mid_pre, up_mid_lab, up_mid_ic;

static void up_commands();
static bool up_begin() {
  up_commands();
  if (up_pg) return true;
#ifdef __EMSCRIPTEN__
  up_pg = (UpPage *)calloc(UP_PAGES, sizeof(UpPage));
#else
  up_pg = (UpPage *)heap_caps_calloc(UP_PAGES, sizeof(UpPage), MALLOC_CAP_SPIRAM);
#endif
  if (!up_pg) return false;
  K_UP = halo_spring(HALO_M3X_DEFAULT);
  const char *D = "0123456789";
  up_mid_hero = halo_text_mid(gl_up_hero, D), up_mid_disp = halo_text_mid(gl_up_disp, D);
  up_mid_pick = halo_text_mid(gl_up_pick, D), up_mid_title = halo_text_mid(gl_up_title, "HxkdLT");
  up_mid_num = halo_text_mid(gl_up_num, D), up_mid_pre = halo_text_mid(gl_up_pre, D);
  up_mid_lab = halo_text_mid(gl_up_lab, "MINSEC"), up_mid_ic = halo_text_mid(gl_up_ic43, UP_IC_PLAY);
  return true;
}
static void up_open(const UpApp *app, int page = 0) {
  if (!up_begin() || !ui_begin()) return;
  up_app = app, up_page = page, up_drag = false;
  halo_spring_set(up_pos, page);
  for (int p = 0; p < UP_PAGES; p++) up_vis[p] = true;  // (each hidden once below, if not seen)
}
static inline int up_count() { return up_app && up_app->count ? constrain(up_app->count(), 1, UP_PAGES) : 1; }
static void up_go(int page) { up_page = constrain(page, 0, up_count() - 1), up_pos.target = up_page; }

// ---------- The parts ----------
static HaloItem &up_item(UpPage &P) {
  HaloItem &it = P.it[min(P.n, UP_IT - 1)];
  P.n = min(P.n + 1, UP_IT);
  return it;
}
static inline void up_commit(HaloItem &it, const HaloView &v) { halo_view_commit(it, v); }
// Text centred at (x, y) by its digits' middle (align: 0 — from x, 1 — centred, 2 — up to x)
static void up_text(UpPage &P, const HaloFont &f, const char *s, float mid, float x, float y, int align, uint32_t c,
                    float a, const HaloView &v, const HaloItem *on = &halo_black) {
  HaloItem &t = up_item(P);
  ui_text(t, f, s, mid, x, y, 1, align, c, a, on);
  t.on_black = on == &halo_black;
  up_commit(t, v);
}
static void up_rrect(UpPage &P, float x0, float y0, float x1, float y1, float r, uint32_t c, float a, const HaloView &v) {
  HaloItem &b = up_item(P);
  b.kind = HI_RRECT, b.x = (x0 + x1) / 2, b.y = (y0 + y1) / 2, b.w = x1 - x0, b.h = y1 - y0, b.r = r, b.scale = 1;
  b.color = c, b.alpha = a, b.on_black = true, b.on = nullptr;
  up_commit(b, v);
}
static HaloItem &up_disc(UpPage &P, float x, float y, float r, uint32_t c, float a, const HaloView &v) {
  HaloItem &b = up_item(P);
  b.kind = HI_CIRCLE, b.x = x, b.y = y, b.r = r, b.scale = 1, b.color = c, b.alpha = a, b.on_black = true, b.on = nullptr;
  up_commit(b, v);
  return b;
}
static void up_arc(UpPage &P, float r, float w, float a0, float a1, uint32_t c, float a, const HaloView &v) {
  HaloItem &it = up_item(P);
  it.kind = HI_ARC, it.x = CX, it.y = CY, it.r = r, it.w = w, it.a0 = a0, it.a1 = a1, it.scale = 1, it.color = c;
  it.alpha = a1 > a0 ? a : 0, it.on_black = true, it.on = nullptr;
  up_commit(it, v);
}
// The primary: a filled shape (circle → four lobes by t) and its glyph in the dark ink (f — an app's own glyph size;
// nullptr — the study's 43 px)
static void up_primary(UpPage &P, float x, float y, float r, float t, const char *icon, uint32_t fill, uint32_t ink,
                       float a, const HaloView &v, const HaloFont *f = nullptr) {
  if (P.nb < UP_BLOBS) {
    UpBlob &b = P.b[P.nb++];
    b.at = P.n, b.x = halo_view_x(v, x), b.y = halo_view_y(v, y), b.r = r * v.s, b.t = t, b.a = a * v.a, b.c = fill;
    b.paint = nullptr;
    const float nw[5] = {b.x, b.y, b.r, b.t, b.a};
    bool same = true;
    for (int k = 0; k < 5; k++) same = same && fabsf(nw[k] - b.was[k]) < (k == 3 || k == 4 ? 0.004f : 0.25f);
    if (!same) {
      if (b.was[2] > 0) halo_damage_f(b.was[0] - b.was[2] - 2, b.was[1] - b.was[2] - 2, b.was[0] + b.was[2] + 2, b.was[1] + b.was[2] + 2);
      halo_damage_f(b.x - b.r - 2, b.y - b.r - 2, b.x + b.r + 2, b.y + b.r + 2);
      for (int k = 0; k < 5; k++) b.was[k] = nw[k];
    }
  }
  const bool play = !strcmp(icon, UP_IC_PLAY);  // (the play glyph sits 3 px right: its mass balanced)
  up_text(P, f ? *f : gl_up_ic43, icon, f ? halo_text_mid(*f, icon) : up_mid_ic, x + (play ? 3 : 0), y, 1, ink, a, v, nullptr);
}
// A secondary: a tonal disc, its glyph in the accent
static void up_secondary(UpPage &P, float x, float y, float r, const char *icon, uint32_t bg, uint32_t fg, float a,
                         const HaloView &v, const HaloFont &f = gl_up_ic32) {
  HaloItem &d = up_disc(P, x, y, r, bg, a, v);
  up_text(P, f, icon, halo_text_mid(f, icon), x, y, 1, fg, a, v, &d);
}
static inline bool up_hit(float x, float y, float cx, float cy, float r) {
  return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= (r + 6) * (r + 6);
}
static inline bool up_in(float x, float y, float x0, float y0, float x1, float y1) {
  return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}
// A page's title under the curved clock
static inline void up_title(UpPage &P, const char *s, float a, const HaloView &v) {
  up_text(P, gl_up_title, s, up_mid_title, CX, UP_TITLE_Y, 1, UP_WHITE, a, v);
}

// An app's own painting in a box (centre x, y, half size r) at its place in the page: redrawn when the box, the key or
// the alpha change (the key: a hash of what it shows)
static void up_paint(UpPage &P, float x, float y, float r, uint32_t key, void (*paint)(const UpBlob &), float a,
                     const HaloView &v) {
  if (P.nb >= UP_BLOBS) return;
  UpBlob &b = P.b[P.nb++];
  b.at = P.n, b.x = halo_view_x(v, x), b.y = halo_view_y(v, y), b.r = r * v.s, b.t = (float)(key % 100000), b.a = a * v.a;
  b.paint = paint;
  const float nw[5] = {b.x, b.y, b.r, b.t, b.a};
  bool same = true;
  for (int k = 0; k < 5; k++) same = same && fabsf(nw[k] - b.was[k]) < (k == 4 ? 0.004f : 0.25f);
  if (!same) {
    if (b.was[2] > 0) halo_damage_f(b.was[0] - b.was[2] - 2, b.was[1] - b.was[2] - 2, b.was[0] + b.was[2] + 2, b.was[1] + b.was[2] + 2);
    halo_damage_f(b.x - b.r - 2, b.y - b.r - 2, b.x + b.r + 2, b.y + b.r + 2);
    for (int k = 0; k < 5; k++) b.was[k] = nw[k];
  }
}
// The blob, per pixel and smoothed, into this band
static void up_blob_draw(const UpBlob &b) {
  if (b.paint) return b.a > 0.004f ? b.paint(b) : (void)0;
  if (b.a < 0.004f || b.r <= 0) return;
  const int y0 = max(hb.y, (int)floorf(b.y - b.r - 1)), y1 = min(hb.y + hb.rows - 1, (int)ceilf(b.y + b.r + 1));
  const int x0 = max(hb.x0, (int)floorf(b.x - b.r - 1)), x1 = min(hb.x1, (int)ceilf(b.x + b.r + 1));
  const int cr = (b.c >> 16 & 255) >> 3, cg = (b.c >> 8 & 255) >> 2, cb = (b.c & 255) >> 3;
  for (int y = y0; y <= y1; y++) {
    uint16_t *row = halo_row(y);
    const float dy = y + 0.5f - b.y;
    for (int x = x0; x <= x1; x++) {
      const float dx = x + 0.5f - b.x, d = sqrtf(dx * dx + dy * dy);
      float R = b.r;
      if (b.t > 0.001f && d > 0.5f) {
        const float s2 = 2 * dx * dy / (d * d);  // sin 2a
        R = b.r * (1 - 0.15f * b.t * s2 * s2);
      }
      const float cov = halo_clamp01(R - d + 0.5f);
      if (cov <= 0) continue;
      halo_blend(row + x, cr, cg, cb, (int)(cov * b.a * 256));
    }
  }
}

// ---------- Each frame ----------
static void up_update(float dt) {
  if (!up_app) return;
  const HaloFinger &f = halo_finger;
  const int n = up_count();
  if (up_page >= n) up_page = n - 1, up_pos.target = up_page;
  if (n > 1 && f.drag_start && f.horiz && f.edge != HALO_EDGE_LEFT && halo_finger_claim(&up_owner)) {
    up_drag = true, up_x0 = f.x, up_g0 = up_pos.x;
  }
  if (up_drag) {
    if (f.down && f.owner == &up_owner) {
      float o = up_g0 + (up_x0 - f.x) / W;
      if (o < 0) o = -halo_rubber(-o * W) / W;
      else if (o > n - 1) o = n - 1 + halo_rubber((o - (n - 1)) * W) / W;
      up_pos.x = o, up_pos.v = -f.vx / W, up_pos.target = o;
    } else {
      up_drag = false;
      const float d = up_pos.x - up_page, v = up_pos.v;
      int to = up_page + (fabsf(d) > 0.33f ? (d > 0 ? 1 : -1) : 0);
      if (fabsf(v) > 0.9f) to = up_page + (v > 0 ? 1 : -1);
      up_page = constrain(to, 0, n - 1), up_pos.target = up_page;
    }
  } else {
    halo_spring_run(up_pos, K_UP, dt);
  }
  if (!up_drag && f.tap && !f.owner && fabsf(up_pos.x - up_page) < 0.05f && up_app->tap) up_app->tap(up_page, f.x, f.y);
  if (up_app->update) up_app->update(up_page, dt);
}

static void up_layout(const HaloView &v) {
  if (!up_app || !up_pg) return;
  const int n = up_count();
  const uint32_t acc = up_app->accent;
  // The rim: the track, the progress over it (fixed: it does not move with the pages)
  const float fr = up_app->ring ? up_app->ring() : -2;
  for (HaloItem *it : {&up_track, &up_fill}) {
    it->kind = HI_ARC, it->x = CX, it->y = CY, it->r = UP_RING_R, it->w = UP_RING_W, it->scale = 1, it->on_black = true;
    it->on = nullptr;
  }
  up_track.a0 = UP_A0, up_track.a1 = UP_A0 + UP_SWEEP, up_track.color = UP_TRACK, up_track.alpha = fr > -1.5f ? 1 : 0;
  up_fill.a0 = UP_A0, up_fill.a1 = UP_A0 + UP_SWEEP * halo_clamp01(fr), up_fill.color = acc;
  up_fill.alpha = fr > 0.001f ? 1 : 0;
  halo_view_commit(up_track, v), halo_view_commit(up_fill, v);
  for (int p = 0; p < UP_PAGES; p++) {
    UpPage &P = up_pg[p];
    const float dx = (p - up_pos.x) * W;
    const bool seen = p < n && fabsf(dx) < W;
    if (!seen) {  // (gone off screen: its items and shapes hidden once)
      if (up_vis[p]) {
        for (HaloItem &it : P.it) it.alpha = 0, halo_view_commit(it, v);
        for (int k = 0; k < P.nb; k++) {
          UpBlob &b = P.b[k];
          if (b.was[2] > 0) halo_damage_f(b.was[0] - b.was[2] - 2, b.was[1] - b.was[2] - 2, b.was[0] + b.was[2] + 2, b.was[1] + b.was[2] + 2);
          b.was[2] = 0;
        }
        P.nb = 0, up_vis[p] = false;
      }
      continue;
    }
    up_vis[p] = true;
    HaloView pv = v;
    pv.x += dx * v.s;
    const int was = P.n, wasb = P.nb;
    P.n = 0, P.nb = 0;
    up_app->lay(p, P, 1, pv);
    for (int i = P.n; i < was; i++) P.it[i].alpha = 0, halo_view_commit(P.it[i], pv);  // (fewer than last frame)
    for (int k = P.nb; k < wasb; k++) {  // (a shape no longer there)
      UpBlob &b = P.b[k];
      if (b.was[2] > 0) halo_damage_f(b.was[0] - b.was[2] - 2, b.was[1] - b.was[2] - 2, b.was[0] + b.was[2] + 2, b.was[1] + b.was[2] + 2);
      b.was[2] = 0;
    }
  }
  // The time along the top: 35 px on a 197 px arc, its ink centred at (233, 38) — one for every page
  if (strcmp(up_clock_of, fc_time)) {
    halo_text_arc(up_clock_mask, gl_up_clock, fc_time, halo_text_mid(gl_up_clock, "0123456789"), 197, 0, false, 1,
                  &up_clock_x, &up_clock_y);
    strlcpy(up_clock_of, fc_time, sizeof(up_clock_of));
  }
  HaloItem &c = up_clock;
  c.kind = HI_MASK, c.mask = &up_clock_mask, c.color = UP_MUTED, c.on_black = true, c.on = &halo_black, c.scale = 1;
  c.x = CX, c.y = 38, c.alpha = up_clock_mask.bits ? 1 : 0;
  halo_view_commit(c, v);
  // The page dots: the one in front r5 in the accent, the others r4 (none with one page)
  for (int p = 0; p < UP_PAGES; p++) {
    HaloItem &d = up_dot[p];
    const float on = halo_clamp01(1 - fabsf(up_pos.x - p));
    d.kind = HI_CIRCLE, d.x = CX + (p - (n - 1) / 2.0f) * 19, d.y = UP_DOTS_Y, d.r = 4 + on, d.scale = 1;
    d.color = ui_mix(UP_DOT, acc, on), d.alpha = n > 1 && p < n ? 1 : 0, d.on_black = true, d.on = nullptr;
    halo_view_commit(d, v);
  }
}

static void up_draw() {
  if (!up_app || !up_pg) return;
  halo_item_draw(up_track), halo_item_draw(up_fill);
  for (int p = 0; p < UP_PAGES; p++) {
    if (!up_vis[p]) continue;
    const UpPage &P = up_pg[p];
    int k = 0;
    for (int i = 0; i < P.n; i++) {
      while (k < P.nb && P.b[k].at == i) up_blob_draw(P.b[k++]);
      halo_item_draw(P.it[i]);
    }
    while (k < P.nb) up_blob_draw(P.b[k++]);
  }
  halo_item_draw(up_clock);
  for (HaloItem &d : up_dot) halo_item_draw(d);
}

// ---------- The audit (serial: upcheck) — what overlaps on the page in front, what comes near the rim ----------
// Every two shown items whose boxes cross by more than 2 px, unless one is the other's ground (on) or holds it whole
// (a text on its card); every text or shape whose box reaches past r 221 (12 px from the rim). Measured, not eyeballed
static void up_audit() {
  if (!up_app || !up_pg) return (void)LOG("upcheck: no paged app open\n");
  const UpPage &P = up_pg[up_page];
  int bad = 0;
  auto name = [&](const HaloItem &it) -> const char * {
    return it.kind == HI_TEXT && it.text ? it.text : it.kind == HI_RRECT ? "[card]" : it.kind == HI_CIRCLE ? "[disc]" : "[shape]";
  };
  // A text's box by its ink (its glyphs' tops and bottoms), not its line: a 154 px line box overlaps everything near it
  auto ink = [](const HaloItem &it, float *o) {
    memcpy(o, it.b, sizeof(it.b));
    if (it.kind != HI_TEXT || !it.font || !it.text) return;
    int top = 9999, bot = -9999;
    for (const char *q = it.text; *q;) {
      const HaloMask *g = halo_glyph(*it.font, halo_utf8_next(q));
      if (g && g->h > 0) top = min(top, (int)g->oy), bot = max(bot, g->oy + g->h);
    }
    if (top > bot) return;
    const float t0 = it.y - it.font->line * it.scale / 2;  // (the line's top: the item's y is its middle)
    o[1] = t0 + top * it.scale, o[3] = t0 + bot * it.scale;
  };
  auto holds = [](const HaloItem &a, const HaloItem &b) {  // (a's box holds b's whole)
    return a.b[0] <= b.b[0] + 1 && a.b[1] <= b.b[1] + 1 && a.b[2] >= b.b[2] - 1 && a.b[3] >= b.b[3] - 1;
  };
  for (int i = 0; i < P.n; i++) {
    const HaloItem &a = P.it[i];
    if (!a.shown || a.alpha < 0.05f) continue;
    float far = 0;
    for (int c = 0; c < 4; c++) {
      const float x = (c & 1 ? a.b[2] : a.b[0]) - CX, y = (c & 2 ? a.b[3] : a.b[1]) - CY;
      far = fmaxf(far, sqrtf(x * x + y * y));
    }
    if (a.kind == HI_TEXT && far > 221) LOG("upcheck: near the rim: \"%s\" reaches r %.0f\n", name(a), far), bad++;
    for (int j = i + 1; j < P.n; j++) {
      const HaloItem &b = P.it[j];
      if (!b.shown || b.alpha < 0.05f || a.kind != HI_TEXT && b.kind != HI_TEXT) continue;
      if (b.on == &a || a.on == &b || holds(a, b) || holds(b, a)) continue;
      float A[4], B[4];
      ink(a, A), ink(b, B);
      const float ox = fminf(A[2], B[2]) - fmaxf(A[0], B[0]), oy = fminf(A[3], B[3]) - fmaxf(A[1], B[1]);
      if (ox > 2 && oy > 2) LOG("upcheck: \"%s\" overlaps \"%s\" (%.0f x %.0f px)\n", name(a), name(b), ox, oy), bad++;
    }
  }
  LOG("upcheck: page %d — %d problem%s\n", up_page + 1, bad, bad == 1 ? "" : "s");
}
static void up_commands() {
  static bool done;
  if (done) return;
  done = true;
  halo_command("upcheck", [](const char *) { up_audit(); });
}

// ---------- A picker column: a value dragged up and down, snapping (the Timer's minutes, seconds) ----------
struct UpPicker {
  HaloSpring pos;  // the value, fractional while dragged
  int lo, hi, owner;
  float y0, g0;
  bool drag;
};
static void up_picker_update(UpPicker &k, float cx, float half_w, float dt) {
  const HaloFinger &f = halo_finger;
  if (f.drag_start && !f.horiz && fabsf(f.x0 - cx) < half_w && halo_finger_claim(&k.owner)) k.drag = true, k.y0 = f.y, k.g0 = k.pos.x;
  if (k.drag) {
    if (f.down && f.owner == &k.owner) {
      k.pos.x = constrain(k.g0 + (k.y0 - f.y) / 62, (float)k.lo, (float)k.hi), k.pos.target = k.pos.x;
    } else {
      k.drag = false, k.pos.target = constrain(roundf(k.pos.x), (float)k.lo, (float)k.hi);
    }
  } else {
    halo_spring_run(k.pos, K_UP, dt);
  }
}
static inline int up_picker_value(const UpPicker &k) { return constrain((int)roundf(k.pos.target), k.lo, k.hi); }
static inline void up_picker_step(UpPicker &k, int d) {
  k.pos.target = constrain(up_picker_value(k) + d, k.lo, k.hi);
}
#endif
