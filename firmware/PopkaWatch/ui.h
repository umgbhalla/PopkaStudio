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
enum : uint8_t { UR_HEAD, UR_NAV, UR_TOGGLE, UR_SLIDER, UR_CHOICE, UR_INFO, UR_ACTION };
#define UI_ROWS 14
#define UI_LEVELS 3  // pages open at once: a root and two deep (lib/Halo's stack has 4 over the gallery)
#define UI_SURF 0x2E3034  // the rows' tonal surface (the gallery's cards')
#define UI_TRACK 0x45474D

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
};
struct UiPage {
  const char *title;
  UiRow rows[UI_ROWS];
  int n;
  uint32_t accent;
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

static float ur_height(const UiRow &r) {
  return r.kind == UR_HEAD ? 52 : r.kind == UR_SLIDER ? 132 : r.kind == UR_ACTION ? 88 : GL_ITEM_H;
}

// ---------- A page on screen ----------
#define UI_IT 9  // items a row: surface, chip, glyph, title, line, track, fill, and two at the end (chevron, −/+, thumb)
struct UiRun {
  const UiPage *pg;
  HaloList list;
  int armed;  // the row the finger came down on (−1 — none)
  HaloItem title, clock;  // (the time along the top edge: it goes up and shrinks as the page scrolls, as the gallery's)
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
  halo_list_init(R.list, h, pg->n + 1, 64, 466 - 70, true);
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

static void ui_layout(int k, const HaloView &v) {
  if (!ui_run || !ui_run[k].pg) return;
  UiRun &R = ui_run[k];
  const UiPage &pg = *R.pg;
  const HaloListFx &hf = R.list.fx[0];
  ui_text(R.title, gl_title, pg.title, ui_mid_title, CX, hf.top + GL_HEAD_H * hf.s / 2, hf.s, 1, GL_TEXT, hf.a_bg, &halo_black);
  R.title.on_black = true;
  halo_view_commit(R.title, v);
  // The time along the top edge: over the first 36 dp of scrolling it goes up 24 dp and shrinks to 0.5, then is gone
  if (strcmp(ui_clock_of, fc_time)) {
    if (!ui_mid_clock) ui_mid_clock = halo_text_mid(gl_t20, "0123456789");
    halo_text_arc(ui_clock_mask, gl_t20, fc_time, ui_mid_clock, 213, 0, false, 1, &ui_clock_x, &ui_clock_y);
    strlcpy(ui_clock_of, fc_time, sizeof(ui_clock_of));
  }
  {
    const float tt = 1 - halo_clamp01((R.list.top - R.list.scroll) / (36 * DP)), k = 0.5f + 0.5f * tt;
    HaloItem &c = R.clock;
    c.kind = HI_MASK, c.mask = &ui_clock_mask, c.color = GL_TEXT, c.on_black = true, c.on = &halo_black, c.scale = k;
    c.x = CX - (CX - ui_clock_x) * k + ui_clock_mask.w * k / 2, c.y = ui_clock_y - 24 * DP * (1 - tt) + ui_clock_mask.h * k / 2;
    c.alpha = tt > 0.02f && ui_clock_mask.bits ? k : 0;
    halo_view_commit(c, v);
  }
  for (int i = 0; i < pg.n; i++) {
    const UiRow &r = pg.rows[i];
    HaloItem *it = R.it[i];
    const HaloListFx &f = R.list.fx[i + 1];
    const bool vis = halo_list_item_visible(R.list, i + 1);
    const float s = f.s, hh = ur_height(r), h = hh * s, w = GL_ITEM_W * s, x0 = CX - w / 2, y0 = f.top, cy = y0 + h / 2;
    const float ab = vis ? f.a_bg : 0, ac = vis ? f.a_c : 0;
    const uint32_t acc = r.accent ? r.accent : pg.accent, on_acc = ui_mix(acc, 0x000000, 0.78f);
    for (int j = 0; j < UI_IT; j++) it[j].alpha = 0;
    char *buf = R.buf[i];
    buf[0] = 0;
    if (r.kind == UR_HEAD) {  // a section's label: small, dim, centered — not in a pill
      ui_text(it[3], gl_t20, r.title, ui_mid_t20, CX, cy, s, 1, GL_DIM, ac, &halo_black);
      it[3].on_black = true;
    } else if (r.kind == UR_ACTION) {  // a pill of its own: pastel with a dark label (the primary action)
      ui_rrect(it[0], CX, cy, GL_ITEM_W * 0.72f, hh, hh / 2, s, acc, ab);
      if (r.text) r.text(buf, 32);
      ui_text(it[3], gl_t24, buf[0] ? buf : r.title, ui_mid_t24, CX, cy, s, 1, on_acc, ac, &it[0], GL_ITEM_W * 0.62f);
    } else if (r.kind == UR_SLIDER) {  // the title over a pill: − and + at its ends, the bar between
      const int lo = r.lo, hi = r.hi, val = r.get ? constrain(r.get(), lo, hi) : lo;
      ui_text(it[3], gl_t20, r.title, ui_mid_t20, CX, y0 + 18 * s, s, 1, GL_DIM, ac, &halo_black);
      it[3].on_black = true;
      const float py = y0 + 36 * s + 48 * s;
      ui_rrect(it[0], CX, py, GL_ITEM_W, 96, 48, s, UI_SURF, ab);
      const float bw = (GL_ITEM_W - 200) * s, bx = CX - bw / 2;
      ui_rrect(it[5], CX, py, GL_ITEM_W - 200, 12, 6, s, UI_TRACK, ac);
      const float fw = bw * (hi > lo ? (float)(val - lo) / (hi - lo) : 0);
      ui_rrect(it[6], bx + fw / 2, py, fmaxf(12, fw / s), 12, 6, s, acc, fw > 0.5f ? ac : 0);
      ui_text(it[7], gl_ic24, "\xEF\x81\xA8", ui_mid_ic, x0 + 52 * s, py, s, 1, GL_TEXT, ac, &it[0]);   // −
      ui_text(it[8], gl_ic24, "\xEF\x81\xA7", ui_mid_ic, x0 + w - 52 * s, py, s, 1, GL_TEXT, ac, &it[0]);  // +
    } else {  // NAV, TOGGLE, CHOICE, INFO: the surface, the chip, the title and the line, the end
      ui_rrect(it[0], CX, cy, GL_ITEM_W, hh, hh / 2, s, UI_SURF, ab);
      const float tx = x0 + (r.icon ? 104 : 36) * s;
      if (r.icon) {
        ui_circle(it[1], x0 + 56 * s, cy, 30, s, acc, ac);
        ui_text(it[2], gl_ic24, r.icon, ui_mid_ic, x0 + 56 * s, cy, s, 1, on_acc, ac, &it[1]);
      }
      if (r.kind == UR_CHOICE && r.choices && r.get) strlcpy(buf, r.choices[constrain(r.get(), 0, r.hi - 1)], 32);
      else if (r.text) r.text(buf, 32);
      const bool two = buf[0] != 0;
      const float end = r.kind == UR_TOGGLE ? 116 : r.kind == UR_NAV ? 64 : 36;  // (room for what is at the end)
      const float fit = GL_ITEM_W - (r.icon ? 104 : 36) - end;
      ui_text(it[3], gl_t24, r.title, ui_mid_t24, tx, two ? y0 + 38 * s : cy, s, 0, GL_TEXT, ac, &it[0], fit);
      if (two)
        ui_text(it[4], gl_t20, buf, ui_mid_t20, tx, y0 + 70 * s, s, 0, r.kind == UR_CHOICE ? acc : GL_DIM, ac, &it[0], fit);
      if (r.kind == UR_NAV)
        ui_text(it[7], gl_ic24, "\xEF\x81\x94", ui_mid_ic, x0 + w - 40 * s, cy, 0.8f * s, 1, GL_DIM, ac, &it[0]);  // ›
      if (r.kind == UR_TOGGLE) {  // the switch: on — the accent's track, a dark thumb at its end; off — grey, thumb left
        const bool on = r.get && r.get();
        const float sx = x0 + w - 64 * s;
        ui_rrect(it[5], sx, cy, 68, 40, 20, s, on ? acc : UI_TRACK, ac);
        ui_circle(it[8], sx + (on ? 14 : -14) * s, cy, on ? 14 : 11, s, on ? on_acc : 0x9A9CA3, ac);
      }
    }
    for (int j = 0; j < UI_IT; j++) halo_view_commit(it[j], v);
  }
}

static void ui_draw(int k) {
  if (!ui_run || !ui_run[k].pg) return;
  const UiRun &R = ui_run[k];
  halo_item_draw(R.clock), halo_item_draw(R.title);
  for (int i = 0; i < R.pg->n; i++)
    for (int j = 0; j < UI_IT; j++) halo_item_draw(R.it[i][j]);
}

static void ui_open(const UiPage *pg);
static void ui_update(int k, float dt) {
  if (!ui_run || !ui_run[k].pg) return;
  ui_cur = k;
  UiRun &R = ui_run[k];
  HaloFinger &f = halo_finger;
  halo_list_update(R.list, dt, true, false);
  if (f.pressed) {
    const int i = halo_list_hit(R.list, f.y0) - 1;
    R.armed = i >= 0 && i < R.pg->n && R.pg->rows[i].kind != UR_HEAD && R.pg->rows[i].kind != UR_INFO ? i : -1;
  }
  if (f.dragging) R.armed = -1;
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
  if (!f.down) R.armed = -1;
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
