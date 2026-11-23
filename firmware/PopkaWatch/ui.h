// ui.h — the watch's UI kit (round board): screens as data, drawn one way everywhere — Material 3 for Wear OS, from the
// study of Google's images (scratchpad wear-os-guide.md, 23 Nov 2026).
//
// A page is a title and rows; a row is one of the kinds below. Each draws itself: a tonal surface pill (GL_ITEM_W ×
// its height, a full pill), a pastel icon chip with a dark glyph of its hue, a title and a line under it (cut to the
// width with an ellipsis), and what the kind has at its end — a chevron (opens a page), a switch, − and + round a bar.
// The page scrolls as the gallery does (HaloList: one to one with the finger, rows shrink toward the edges), a tap acts,
// a row that opens a page opens it over this one (lib/Halo's stack: back — the swipe from the left edge).
//   UiPage p = {"Display", {ur_slider(...), ur_toggle(...), ...}, n, accent};   ui_open(&p) — over the screen on top
// The first user: the Settings app (sysset.h). Included from sysset.h.
#ifdef HALO_ROUND
enum : uint8_t { UR_HEAD, UR_NAV, UR_TOGGLE, UR_SLIDER, UR_CHOICE, UR_INFO, UR_ACTION, UR_CUSTOM };
#define UI_ROWS 14
#define UI_LEVELS 3  // pages open at once: a root and two deep (lib/Halo's stack has 4 over the gallery)
// Material 3 for Wear OS, by role (the references: app-layout-scrolling-settings, -app-list, -mixed-list): rows are a quiet
// grey surface; the page's one accent is its primary — the icons (a glyph, no disc), a switch, a slider's fill, the edge
// button; an ON row fills with the accent's deep container (selection is fill, not a border); labels are small, left
#define UI_SURF 0x2A2A2E   // a row (neutral: one 565 level for its three channels — no tint)
#define UI_TRACK 0x48474C  // a switch off, a slider's empty part
#define UI_DIM 0xC4C7C5    // a row's second line, a section's label
#define UI_EB_FULL 104.0f  // the edge button's height (52 dp), its width (Wear OS: 247 × 126 at 466 px)
#define UI_EB_W 272.0f
#define UI_EB_R 227.0f     // its bottom follows the screen's circle 3 dp in (the gallery's EB_R)

struct UiPage;
struct UiRow {
  uint8_t kind;
  const char *icon, *title;  // the chip's glyph (gl_ic24) — nullptr: no chip
  int (*get)();              // TOGGLE 0/1, SLIDER the value, CHOICE the index
  void (*set)(int);
  int lo, hi;                // SLIDER: the range; CHOICE: how many
  const char *const *choices;          // CHOICE: the labels
  void (*text)(char *out, int n);      // NAV: the line under; INFO: the value; ACTION: the label (else title)
  const UiPage *page;                  // NAV: what it opens
  void (*act)();                       // ACTION; NAV with no page
  uint32_t accent;                     // the chip and the controls (0 — the page's)
  // CUSTOM: a row the app draws itself — its height; layout: where it is now (it damages what changed), draw: in the band
  float ch;
  void (*clay)(float x0, float y0, float w, float h, float s, float a, const HaloView &v);
  void (*cdraw)();
};
struct UiPage {
  const char *title;
  UiRow rows[UI_ROWS];
  int n;
  uint32_t accent;  // the primary (a pastel): its deep container and light on-container come from it
  UiRow edge;       // the page's main action, the edge button hugging the bottom (kind UR_ACTION; act nullptr — none)
};
static inline UiRow ur_head(const char *title) {
  UiRow r = {};
  r.kind = UR_HEAD, r.title = title;
  return r;
}
static inline UiRow ur_nav(const char *icon, const char *title, const UiPage *page, void (*text)(char *, int) = nullptr,
                           uint32_t accent = 0) {
  UiRow r = {};
  r.kind = UR_NAV, r.icon = icon, r.title = title, r.page = page, r.text = text, r.accent = accent;
  return r;
}
static inline UiRow ur_toggle(const char *icon, const char *title, int (*get)(), void (*set)(int),
                              void (*text)(char *, int) = nullptr) {
  UiRow r = {};
  r.kind = UR_TOGGLE, r.icon = icon, r.title = title, r.get = get, r.set = set, r.text = text;
  return r;
}
static inline UiRow ur_slider(const char *title, int lo, int hi, int (*get)(), void (*set)(int)) {
  UiRow r = {};
  r.kind = UR_SLIDER, r.title = title, r.lo = lo, r.hi = hi, r.get = get, r.set = set;
  return r;
}
static inline UiRow ur_choice(const char *icon, const char *title, const char *const *choices, int n, int (*get)(),
                              void (*set)(int)) {
  UiRow r = {};
  r.kind = UR_CHOICE, r.icon = icon, r.title = title, r.choices = choices, r.hi = n, r.get = get, r.set = set;
  return r;
}
static inline UiRow ur_info(const char *icon, const char *title, void (*text)(char *, int)) {
  UiRow r = {};
  r.kind = UR_INFO, r.icon = icon, r.title = title, r.text = text;
  return r;
}
static inline UiRow ur_action(const char *title, void (*act)(), uint32_t accent = 0) {
  UiRow r = {};
  r.kind = UR_ACTION, r.title = title, r.act = act, r.accent = accent;
  return r;
}

static inline UiRow ur_custom(float h, void (*clay)(float, float, float, float, float, float, const HaloView &),
                              void (*cdraw)()) {
  UiRow r = {};
  r.kind = UR_CUSTOM, r.ch = h, r.clay = clay, r.cdraw = cdraw;
  return r;
}

static float ur_height(const UiRow &r) {
  return r.kind == UR_CUSTOM ? r.ch : r.kind == UR_HEAD ? 52 : r.kind == UR_SLIDER ? 132 : r.kind == UR_ACTION ? 88 : GL_ITEM_H;
}

// ---------- A page on screen ----------
#define UI_IT 9  // items a row: surface, chip, glyph, title, line, track, fill, and two at the end (chevron, −/+, thumb)
struct UiRun {
  const UiPage *pg;
  HaloList list;
  int armed;  // the row the finger came down on (−1 — none)
  HaloItem title, clock;  // (the time along the top edge: it goes up and shrinks as the page scrolls, as the gallery's)
  HaloItem eb, eb_label;  // the edge button
  char eb_buf[32];
  bool eb_armed;
  HaloItem it[UI_ROWS][UI_IT];
  char buf[UI_ROWS][32];
};
static UiRun *ui_run;  // [UI_LEVELS], in PSRAM (≈ 40 KB of items: internal memory is the Wi-Fi's)
static int ui_cur;     // the level on top (its update runs)
static HaloMask ui_clock_mask;  // the time along the top edge (Wear OS's time text), one for every page: made when it changes
static char ui_clock_of[8];
static float ui_clock_x, ui_clock_y, ui_mid_clock;
static float ui_mid_title, ui_mid_t24, ui_mid_t20, ui_mid_ic;

static bool ui_begin() {
  if (ui_run) return true;
#ifdef __EMSCRIPTEN__
  void *p = calloc(UI_LEVELS, sizeof(UiRun));
#else
  void *p = heap_caps_calloc(UI_LEVELS, sizeof(UiRun), MALLOC_CAP_SPIRAM);
  if (!p) p = calloc(UI_LEVELS, sizeof(UiRun));
#endif
  if (!p) return false;
  ui_run = (UiRun *)p;
  for (int k = 0; k < UI_LEVELS; k++) new (&ui_run[k]) UiRun();
  ui_mid_title = halo_text_mid(gl_title, "Settings"), ui_mid_t24 = halo_text_mid(gl_t24, "Hxg");
  ui_mid_t20 = halo_text_mid(gl_t20, "Hxg"), ui_mid_ic = halo_text_mid(gl_ic24, "\xEF\x80\x93");
  return true;
}
static uint32_t ui_mix(uint32_t a, uint32_t b, float t) {
  uint32_t out = 0;
  for (int s = 0; s <= 16; s += 8) {
    const int ca = a >> s & 255, cb = b >> s & 255;
    out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << s;
  }
  return out;
}

// The page into level k: its rows' heights into the list (the title is the list's header), from its top
static void ui_set_page(int k, const UiPage *pg) {
  if (!ui_begin()) return;
  UiRun &R = ui_run[k];
  R.pg = pg, R.armed = -1;
  float h[UI_ROWS + 1];
  h[0] = GL_HEAD_H;
  for (int i = 0; i < pg->n; i++) h[i + 1] = ur_height(pg->rows[i]);
  halo_list_init(R.list, h, pg->n + 1, 64, pg->edge.act ? CY + UI_EB_R - UI_EB_FULL - 10 : 466 - 56, true);
}

static void ui_text(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, float sc, int align,
                    uint32_t color, float a, const HaloItem *on, float fit = 0) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.spacing = 0, it.color = color, it.scale = sc;
  it.alpha = a, it.on_black = false, it.on = on, it.fit = fit;
  it.x = x, it.y = y + (f.line / 2.0f - mid) * sc;
}
static void ui_rrect(HaloItem &it, float x, float y, float w, float h, float r, float sc, uint32_t c, float a) {
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = sc, it.color = c, it.alpha = a;
  it.on_black = true, it.on = nullptr;
}
static void ui_circle(HaloItem &it, float x, float y, float r, float sc, uint32_t c, float a) {
  it.kind = HI_CIRCLE, it.x = x, it.y = y, it.r = r, it.scale = sc, it.color = c, it.alpha = a, it.on_black = true;
  it.on = nullptr;
}

// The accent's roles (Material 3's tonal palette from one seed, simply): the primary itself, its deep container (an ON row,
// a secondary action), a light tone on that container, a dark one on the primary
static inline uint32_t ui_container(uint32_t p) { return ui_mix(p, 0x000000, 0.66f); }
static inline uint32_t ui_on_container(uint32_t p) { return ui_mix(p, 0xFFFFFF, 0.6f); }
static inline uint32_t ui_on_primary(uint32_t p) { return ui_mix(p, 0x000000, 0.8f); }
// How much of the edge button shows: all of it at the list's end, nothing 140 px before (it grows in as the end comes,
// as the gallery's)
static float ui_eb_height(const UiRun &R) {
  if (!R.pg || !R.pg->edge.act) return 0;
  const float to_end = R.list.scroll - halo_list_min(R.list);
  return UI_EB_FULL * halo_clamp01(1 - to_end / 140);
}

static void ui_layout(int k, const HaloView &v) {
  if (!ui_run || !ui_run[k].pg) return;
  UiRun &R = ui_run[k];
  const UiPage &pg = *R.pg;
  const uint32_t pri = pg.accent;
  const HaloListFx &hf = R.list.fx[0];
  ui_text(R.title, gl_t24, pg.title, ui_mid_t24, CX, hf.top + GL_HEAD_H * hf.s / 2, hf.s, 1, GL_TEXT, hf.a_bg, &halo_black, 300);
  R.title.on_black = true;
  halo_view_commit(R.title, v);
  // The time along the top edge: over the first 36 dp of scrolling it goes up 24 dp and shrinks to 0.5, then is gone
  if (strcmp(ui_clock_of, fc_time)) {
    if (!ui_mid_clock) ui_mid_clock = halo_text_mid(gl_t20, "0123456789");
    halo_text_arc(ui_clock_mask, gl_t20, fc_time, ui_mid_clock, 213, 0, false, 1, &ui_clock_x, &ui_clock_y);
    strlcpy(ui_clock_of, fc_time, sizeof(ui_clock_of));
  }
  {
    const float tt = 1 - halo_clamp01((R.list.top - R.list.scroll) / (36 * DP)), kk = 0.5f + 0.5f * tt;
    HaloItem &c = R.clock;
    c.kind = HI_MASK, c.mask = &ui_clock_mask, c.color = GL_TEXT, c.on_black = true, c.on = &halo_black, c.scale = kk;
    c.x = CX - (CX - ui_clock_x) * kk + ui_clock_mask.w * kk / 2, c.y = ui_clock_y - 24 * DP * (1 - tt) + ui_clock_mask.h * kk / 2;
    c.alpha = tt > 0.02f && ui_clock_mask.bits ? kk : 0;
    halo_view_commit(c, v);
  }
  for (int i = 0; i < pg.n; i++) {
    const UiRow &r = pg.rows[i];
    HaloItem *it = R.it[i];
    const HaloListFx &f = R.list.fx[i + 1];
    const bool vis = halo_list_item_visible(R.list, i + 1);
    const float s = f.s, hh = ur_height(r), h = hh * s, w = GL_ITEM_W * s, x0 = CX - w / 2, y0 = f.top, cy = y0 + h / 2;
    const float ab = vis ? f.a_bg : 0, ac = vis ? f.a_c : 0;
    const uint32_t acc = r.accent ? r.accent : pri;
    for (int j = 0; j < UI_IT; j++) it[j].alpha = 0;
    char *buf = R.buf[i];
    buf[0] = 0;
    if (r.kind == UR_CUSTOM) {  // (the app's own: it is told where, it draws)
      if (r.clay) r.clay(x0, y0, w, h, s, ac, v);
    } else if (r.kind == UR_HEAD) {  // a section's label: small, left, in sentence case — not in a pill
      ui_text(it[3], gl_t20, r.title, ui_mid_t20, x0 + 30 * s, y0 + h * 0.62f, s, 0, UI_DIM, ac, &halo_black);
      it[3].on_black = true;
    } else if (r.kind == UR_ACTION) {  // a secondary action in the list: the accent's container, a light label
      ui_rrect(it[0], CX, cy, GL_ITEM_W, hh, hh / 2, s, ui_container(acc), ab);
      if (r.text) r.text(buf, 32);
      ui_text(it[3], gl_t24, buf[0] ? buf : r.title, ui_mid_t24, CX, cy, s, 1, ui_on_container(acc), ac, &it[0], GL_ITEM_W - 80);
    } else if (r.kind == UR_SLIDER) {  // its label over a pill: − and + at the ends, a bar with a thumb between
      const int lo = r.lo, hi = r.hi, val = r.get ? constrain(r.get(), lo, hi) : lo;
      ui_text(it[3], gl_t20, r.title, ui_mid_t20, x0 + 30 * s, y0 + 20 * s, s, 0, UI_DIM, ac, &halo_black);
      it[3].on_black = true;
      const float py = y0 + 36 * s + 48 * s, bw = (GL_ITEM_W - 196) * s, bx = CX - bw / 2;
      ui_rrect(it[0], CX, py, GL_ITEM_W, 96, 48, s, UI_SURF, ab);
      ui_rrect(it[5], CX, py, GL_ITEM_W - 196, 8, 4, s, UI_TRACK, ac);
      const float fw = bw * (hi > lo ? (float)(val - lo) / (hi - lo) : 0);
      ui_rrect(it[6], bx + fw / 2, py, fmaxf(8, fw / s), 8, 4, s, acc, fw > 0.5f ? ac : 0);
      ui_circle(it[4], bx + fw, py, 11, s, acc, ac);  // (the thumb)
      ui_text(it[7], gl_ic24, "\xEF\x81\xA8", ui_mid_ic, x0 + 50 * s, py, 0.85f * s, 1, GL_TEXT, ac, &it[0]);   // −
      ui_text(it[8], gl_ic24, "\xEF\x81\xA7", ui_mid_ic, x0 + w - 50 * s, py, 0.85f * s, 1, GL_TEXT, ac, &it[0]);  // +
    } else {  // NAV, TOGGLE, CHOICE, INFO: a quiet surface (an ON toggle: the container), a glyph, the title, the line
      const bool on = r.kind == UR_TOGGLE && r.get && r.get();
      const uint32_t surf = on ? ui_container(acc) : UI_SURF;
      ui_rrect(it[0], CX, cy, GL_ITEM_W, hh, hh / 2, s, surf, ab);
      const float tx = x0 + (r.icon ? 78 : 32) * s;
      if (r.icon) ui_text(it[2], gl_ic24, r.icon, ui_mid_ic, x0 + 44 * s, cy, s, 1, on ? ui_on_container(acc) : acc, ac, &it[0]);
      if (r.kind == UR_CHOICE && r.choices && r.get) strlcpy(buf, r.choices[constrain(r.get(), 0, r.hi - 1)], 32);
      else if (r.text) r.text(buf, 32);
      const bool two = buf[0] != 0;
      const float fit = GL_ITEM_W - (r.icon ? 78 : 32) - (r.kind == UR_TOGGLE ? 96 : 32);
      ui_text(it[3], gl_t24, r.title, ui_mid_t24, tx, two ? y0 + 38 * s : cy, s, 0, GL_TEXT, ac, &it[0], fit);
      if (two)
        ui_text(it[4], gl_t20, buf, ui_mid_t20, tx, y0 + 70 * s, s, 0, r.kind == UR_CHOICE ? acc : on ? ui_on_container(acc) : UI_DIM,
                ac, &it[0], fit);
      if (r.kind == UR_TOGGLE) {  // the switch: on — the primary's track, a dark thumb at its end; off — grey, small thumb left
        const float sx = x0 + w - 54 * s;
        ui_rrect(it[5], sx, cy, 52, 32, 16, s, on ? acc : UI_TRACK, ac);
        ui_circle(it[8], sx + (on ? 10 : -10) * s, cy, on ? 12 : 8, s, on ? ui_on_primary(acc) : 0x8E9099, ac);
      }
    }
    for (int j = 0; j < UI_IT; j++) halo_view_commit(it[j], v);
  }
  // The edge button: the page's main action, hugging the bottom (grows in as the end comes)
  const float eh = ui_eb_height(R);
  HaloItem &e = R.eb;
  e.kind = HI_EDGE, e.x = CX, e.y = CY, e.r = UI_EB_R, e.h = eh, e.w = UI_EB_W, e.v = 48, e.scale = 1;
  e.color = pri, e.alpha = eh > 4 ? halo_clamp01((eh - 8) / 52) : 0, e.on_black = true, e.on = nullptr;
  if (pg.edge.text) pg.edge.text(R.eb_buf, 32);
  else strlcpy(R.eb_buf, pg.edge.title ? pg.edge.title : "", 32);
  ui_text(R.eb_label, gl_t24, R.eb_buf, ui_mid_t24, CX, CY + UI_EB_R - eh + fminf(eh, UI_EB_FULL) * 0.42f, 1, 1, ui_on_primary(pri),
          eh > 40 ? halo_clamp01((eh - 40) / 30) : 0, &R.eb, UI_EB_W - 70);
  halo_view_commit(e, v), halo_view_commit(R.eb_label, v);
}

static void ui_draw(int k) {
  if (!ui_run || !ui_run[k].pg) return;
  const UiRun &R = ui_run[k];
  halo_item_draw(R.clock), halo_item_draw(R.title);
  for (int i = 0; i < R.pg->n; i++) {
    for (int j = 0; j < UI_IT; j++) halo_item_draw(R.it[i][j]);
    if (R.pg->rows[i].kind == UR_CUSTOM && R.pg->rows[i].cdraw) R.pg->rows[i].cdraw();
  }
  halo_item_draw(R.eb), halo_item_draw(R.eb_label);  // (over the list)
}

static void ui_open(const UiPage *pg);
static void ui_update(int k, float dt) {
  if (!ui_run || !ui_run[k].pg) return;
  ui_cur = k;
  UiRun &R = ui_run[k];
  HaloFinger &f = halo_finger;
  halo_list_update(R.list, dt, true, false);
  if (f.pressed) {
    const float eh = ui_eb_height(R);
    R.eb_armed = eh > 40 && f.y0 > CY + UI_EB_R - eh && fabsf(f.x0 - CX) < UI_EB_W / 2;  // (the edge button is over the list)
    const int i = R.eb_armed ? -1 : halo_list_hit(R.list, f.y0) - 1;
    R.armed = i >= 0 && i < R.pg->n && R.pg->rows[i].kind != UR_HEAD && R.pg->rows[i].kind != UR_INFO ? i : -1;
  }
  if (f.dragging) R.armed = -1, R.eb_armed = false;
  if (f.tap && R.eb_armed && !f.owner && R.pg->edge.act) R.pg->edge.act();
  if (f.tap && R.armed >= 0 && !f.owner) {
    const UiRow &r = R.pg->rows[R.armed];
    switch (r.kind) {
      case UR_NAV:
        if (r.page) ui_open(r.page);
        else if (r.act) r.act();
        break;
      case UR_TOGGLE:
        if (r.get && r.set) r.set(!r.get());
        break;
      case UR_CHOICE:  // (a tap: the next one — a picker for long lists comes later)
        if (r.get && r.set && r.hi > 0) r.set((r.get() + 1) % r.hi);
        break;
      case UR_SLIDER:
        if (r.get && r.set) {
          const int d = f.x < CX - 60 ? -1 : f.x > CX + 60 ? 1 : 0;
          if (d) r.set(constrain(r.get() + d, r.lo, r.hi));
        }
        break;
      case UR_ACTION:
        if (r.act) r.act();
        break;
    }
  }
  if (!f.down) R.armed = -1, R.eb_armed = false;
}

// The screens of the levels over the root (the root is a screen of the project: sysset.h draws its level 0)
template <int K> static void ui_lv_update(float dt) { ui_update(K, dt); }
template <int K> static void ui_lv_layout(const HaloView &v) { ui_layout(K, v); }
template <int K> static void ui_lv_draw() { ui_draw(K); }
static void ui_open(const UiPage *pg) {
  const int k = min(ui_cur + 1, UI_LEVELS - 1);
  ui_set_page(k, pg);
  const int d = min(halo_nav_depth() + 1, NAV_LEVELS - 1);
  nav_scr[d] = -1, nav_stub[d] = pg->title;
  static const HaloScreen S[UI_LEVELS] = {{nullptr, ui_lv_update<0>, ui_lv_layout<0>, ui_lv_draw<0>},
                                          {nullptr, ui_lv_update<1>, ui_lv_layout<1>, ui_lv_draw<1>},
                                          {nullptr, ui_lv_update<2>, ui_lv_layout<2>, ui_lv_draw<2>}};
  halo_nav_open(S[k]);
}
#endif
