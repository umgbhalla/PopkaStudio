// upager.h — paged app screens (Material 3 Expressive for Wear OS, Google's references: paged containers, non-scrolling
// layouts, big icon buttons, the countdown — 23 Nov 2026): an app is a row of NON-SCROLLING pages swiped left and
// right (a spring, a third of the way or a fling turns the page; rubber past the ends), page dots at the bottom edge,
// the time curved along the top, a progress ring at the rim. A swipe from the left edge stays the system's back.
// The parts an app lays out with: big round icon buttons (the primary morphs from a circle to a rounded square while
// something runs), small tonal ones, the big time, picker columns. Each page has its own items, laid through the
// screen's view shifted by where the page is — so a page in the middle of a swipe is exactly the page at rest, moved.
// Round board only. Included from PopkaWatch.ino after ui.h (its helpers, the time text) and before clocks.h.
#ifdef HALO_ROUND
extern const HaloFont gl_up_ic40, gl_up_n96, gl_up_n36;  // (studio.py GALLERY_FONTS)
#define UP_PAGES 8
#define UP_IT 40
#define UP_IC_PLAY "\xEF\x81\x8B"
#define UP_IC_PAUSE "\xEF\x81\x8C"
#define UP_IC_FLAG "\xEF\x80\xA4"
#define UP_IC_RESET "\xEF\x8B\xB1"
#define UP_IC_PLUS "+"
#define UP_IC_X "\xEF\x80\x8D"
#define UP_IC_SUN "\xEF\x86\x85"
#define UP_IC_MOON "\xEF\x86\x86"
#define UP_IC_GLOBE "\xEF\x82\xAC"
#define UP_DOTS_Y 444.0f

struct UpPage {
  HaloItem it[UP_IT];
  int n;  // items used in this frame's layout
};
struct UpApp {
  int (*count)();                                                  // pages now (1 … UP_PAGES)
  void (*lay)(int p, UpPage &P, float a, const HaloView &v);      // place page p's items (as at rest: v is shifted)
  void (*tap)(int p, float x, float y);                           // a tap on page p (screen coordinates)
  void (*update)(int p, float dt);                                // every frame, the page in front (its own drags)
  float (*ring)();                                                 // the rim's progress 0 … 1 (< 0 — none)
  uint32_t accent;
};
static const UpApp *up_app;
static UpPage *up_pg;  // [UP_PAGES], PSRAM
static HaloSpring up_pos;
static HaloSpringK K_UP;
static int up_page, up_owner;
static bool up_drag, up_vis[UP_PAGES];
static float up_x0, up_g0;
static HaloItem up_ring, up_clock, up_dot[UP_PAGES];
static float up_mid_ic, up_mid_n96, up_mid_n36;

static bool up_begin() {
  if (up_pg) return true;
#ifdef __EMSCRIPTEN__
  up_pg = (UpPage *)calloc(UP_PAGES, sizeof(UpPage));
#else
  up_pg = (UpPage *)heap_caps_calloc(UP_PAGES, sizeof(UpPage), MALLOC_CAP_SPIRAM);
#endif
  if (!up_pg) return false;
  K_UP = halo_spring(HALO_M3X_DEFAULT);
  up_mid_ic = halo_text_mid(gl_up_ic40, UP_IC_PLAY), up_mid_n96 = halo_text_mid(gl_up_n96, "0123456789");
  up_mid_n36 = halo_text_mid(gl_up_n36, "0123456789");
  return true;
}
static void up_open(const UpApp *app, int page = 0) {
  if (!up_begin() || !ui_begin()) return;
  up_app = app, up_page = page, up_drag = false;
  halo_spring_set(up_pos, page);
  for (int p = 0; p < UP_PAGES; p++) up_vis[p] = true;  // (each hidden once below, if not seen)
}
static inline int up_count() { return up_app && up_app->count ? constrain(up_app->count(), 1, UP_PAGES) : 1; }
static void up_go(int page) {  // (an app turning its own page: a preset started → the running page)
  up_page = constrain(page, 0, up_count() - 1), up_pos.target = up_page;
}

// ---------- The parts ----------
static HaloItem &up_item(UpPage &P) {
  HaloItem &it = P.it[min(P.n, UP_IT - 1)];
  P.n = min(P.n + 1, UP_IT);
  return it;
}
static void up_commit(HaloItem &it, const HaloView &v) { halo_view_commit(it, v); }
// A round icon button: the primary (filled with the accent, a dark glyph; morph 0 … 1 — circle → rounded square, as
// M3 Expressive's buttons do while something runs) or a tonal one (the accent's dark container, a light glyph)
static void up_button(UpPage &P, float x, float y, float r, const char *icon, bool primary, float morph, uint32_t acc,
                      float a, const HaloView &v) {
  HaloItem &b = up_item(P), &g = up_item(P);
  const float rr = r * (1 - 0.42f * halo_clamp01(morph));
  b.kind = HI_RRECT, b.x = x, b.y = y, b.w = 2 * r, b.h = 2 * r, b.r = rr, b.scale = 1;
  b.color = primary ? acc : ui_container(acc), b.alpha = a, b.on_black = true, b.on = nullptr;
  up_commit(b, v);
  const float s = (primary ? 1.0f : 0.78f) * r / 56;
  ui_text(g, gl_up_ic40, icon, up_mid_ic, x, y, s, 1, primary ? ui_on_primary(acc) : ui_on_container(acc), a, &b);
  up_commit(g, v);
}
static inline bool up_hit(float x, float y, float cx, float cy, float r) {
  return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= (r + 10) * (r + 10);  // (the target a little past the shape)
}
// Text, centred at (x, y) by its digits' middle
static void up_text(UpPage &P, const HaloFont &f, const char *s, float mid, float x, float y, float sc, int align,
                    uint32_t c, float a, const HaloView &v) {
  HaloItem &t = up_item(P);
  ui_text(t, f, s, mid, x, y, sc, align, c, a, &halo_black);
  t.on_black = true;
  up_commit(t, v);
}
// The big time: the main part big, a small tail (".34") in the accent, centred as one by their widths
static void up_time(UpPage &P, const char *main, const char *tail, float y, uint32_t acc, float a, const HaloView &v) {
  const float w1 = halo_text_width(gl_up_n96, main), w2 = tail && *tail ? halo_text_width(gl_up_n36, tail) + 6 : 0;
  const float x0 = CX - (w1 + w2) / 2;
  up_text(P, gl_up_n96, main, up_mid_n96, x0, y, 1, 0, GL_TEXT, a, v);
  if (w2 > 0) up_text(P, gl_up_n36, tail, up_mid_n36, x0 + w1 + 6, y + 22, 1, 0, acc, a, v);
}

// ---------- Each frame ----------
static void up_update(float dt) {
  if (!up_app) return;
  const HaloFinger &f = halo_finger;
  const int n = up_count();
  if (up_page >= n) up_page = n - 1, up_pos.target = up_page;
  // Paging: a sideways drag not from the left edge (that one is the system's back); the page goes under the finger
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
  for (int p = 0; p < UP_PAGES; p++) {
    UpPage &P = up_pg[p];
    const float dx = (p - up_pos.x) * W;
    const bool seen = p < n && fabsf(dx) < W;
    if (!seen) {  // (gone off screen: its items hidden once)
      if (up_vis[p]) {
        for (HaloItem &it : P.it) it.alpha = 0, halo_view_commit(it, v);
        up_vis[p] = false;
      }
      continue;
    }
    up_vis[p] = true;
    HaloView pv = v;
    pv.x += dx * v.s;
    const int was = P.n;
    P.n = 0;
    up_app->lay(p, P, 1, pv);
    for (int i = P.n; i < was; i++) P.it[i].alpha = 0, halo_view_commit(P.it[i], pv);  // (fewer than last frame)
  }
  // The time along the top (one for every page: it does not move with them)
  if (strcmp(ui_clock_of, fc_time)) {
    ui_mid_clock = halo_text_mid(gl_t20, "0123456789");
    halo_text_arc(ui_clock_mask, gl_t20, fc_time, ui_mid_clock, 206, 0, false, 1, &ui_clock_x, &ui_clock_y);
    strlcpy(ui_clock_of, fc_time, sizeof(ui_clock_of));
  }
  HaloItem &c = up_clock;
  c.kind = HI_MASK, c.mask = &ui_clock_mask, c.color = GL_TEXT, c.on_black = true, c.on = &halo_black, c.scale = 1;
  c.x = ui_clock_x + ui_clock_mask.w / 2.0f, c.y = ui_clock_y + ui_clock_mask.h / 2.0f, c.alpha = ui_clock_mask.bits ? 1 : 0;
  halo_view_commit(c, v);
  // The rim's progress
  const float fr = up_app->ring ? up_app->ring() : -1;
  HaloItem &rg = up_ring;
  rg.kind = HI_RING, rg.x = CX, rg.y = CY, rg.r = UI_RING_R, rg.w = UI_RING_W, rg.v = halo_clamp01(fr), rg.scale = 1;
  rg.color = acc, rg.color2 = 0x262629, rg.alpha = fr >= 0 ? 1 : 0, rg.on_black = true, rg.on = nullptr;
  halo_view_commit(rg, v);
  // The page dots: the one in front a short pill, the others small dots (none with one page)
  const float gap = 14, x0 = CX - (n - 1) * gap / 2;
  for (int p = 0; p < UP_PAGES; p++) {
    HaloItem &d = up_dot[p];
    const float on = halo_clamp01(1 - fabsf(up_pos.x - p));
    d.kind = HI_RRECT, d.x = x0 + p * gap, d.y = UP_DOTS_Y, d.w = 6 + 8 * on, d.h = 6, d.r = 3, d.scale = 1;
    d.color = ui_mix(0x5C5C60, 0xF1F0F4, on), d.alpha = n > 1 && p < n ? 1 : 0, d.on_black = true, d.on = nullptr;
    halo_view_commit(d, v);
  }
}

static void up_draw() {
  if (!up_app || !up_pg) return;
  for (int p = 0; p < UP_PAGES; p++)
    if (up_vis[p])
      for (int i = 0; i < up_pg[p].n; i++) halo_item_draw(up_pg[p].it[i]);
  halo_item_draw(up_ring), halo_item_draw(up_clock);
  for (HaloItem &d : up_dot) halo_item_draw(d);
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
      k.pos.x = constrain(k.g0 + (k.y0 - f.y) / 64, (float)k.lo, (float)k.hi), k.pos.target = k.pos.x;
    } else {
      k.drag = false, k.pos.target = constrain(roundf(k.pos.x), (float)k.lo, (float)k.hi);
    }
  } else {
    halo_spring_run(k.pos, K_UP, dt);
  }
}
static inline int up_picker_value(const UpPicker &k) { return constrain((int)roundf(k.pos.target), k.lo, k.hi); }
// Its numbers: the value big in the middle, the one before and after smaller and dim above and below, sliding
static void up_picker_lay(UpPage &P, UpPicker &k, float cx, float cy, char (*buf)[4], float a, const HaloView &v) {
  const float x = k.pos.x, base = floorf(x);
  for (int j = -1; j <= 2; j++) {
    const int val = (int)base + j;
    const float d = val - x;  // (0 — in the middle)
    const float ad = fabsf(d);
    const bool ok = val >= k.lo && val <= k.hi && ad < 1.6f;
    snprintf(buf[j + 1], 4, "%02d", ok ? val : 0);
    const float sc = 1 - 0.45f * halo_clamp01(ad);
    up_text(P, gl_up_n96, buf[j + 1], up_mid_n96, cx, cy + d * 74, sc, 1, ad < 0.5f ? GL_TEXT : 0x8E9099,
            ok ? a * (1 - 0.55f * halo_clamp01(ad)) : 0, v);
  }
}
#endif
