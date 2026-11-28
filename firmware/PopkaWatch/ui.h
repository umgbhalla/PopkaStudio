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
enum : uint8_t { UR_HEAD, UR_NAV, UR_TOGGLE, UR_SLIDER, UR_CHOICE, UR_INFO, UR_ACTION, UR_CUSTOM, UR_RADIO };
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
// The progress ring's lane at the rim (centre 226, 6 wide: 223 … 229, 4 px from the screen's edge); with a ring the
// edge button's bottom and the time keep inside it — measured: at 227 the ring ran along the button's bottom and 1 px
// from the time's letters
#define UI_RING_R 226.0f
#define UI_RING_W 6.0f
struct UiPage;
static inline float ui_eb_r(const UiPage &p);

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
  float (*ring)();  // the progress indicator round the edge (Wear OS): 0 … 1 from 12 o'clock; < 0 or nullptr — none
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
// A compact action (Wear OS: a small tonal pill under a hero — Lap, Reset, +1 min): an icon and a short label, centred
#define UI_CPT_W 156.0f
#define UI_CPT_H 56.0f
static inline UiRow ur_compact(const char *icon, const char *title, void (*act)(), uint32_t accent = 0) {
  UiRow r = ur_action(title, act, accent);
  r.icon = icon, r.lo = 1;
  return r;
}

static inline UiRow ur_custom(float h, void (*clay)(float, float, float, float, float, float, const HaloView &),
                              void (*cdraw)()) {
  UiRow r = {};
  r.kind = UR_CUSTOM, r.ch = h, r.clay = clay, r.cdraw = cdraw;
  return r;
}

static float ur_height(const UiRow &r) {
  return r.kind == UR_CUSTOM ? r.ch : r.kind == UR_HEAD ? 52 : r.kind == UR_SLIDER ? 132 : r.kind == UR_ACTION ? (r.lo ? 72 : 88) : GL_ITEM_H;
}

// ---------- A page on screen ----------
#define UI_IT 9  // items a row: surface, chip, glyph, title, line, track, fill, and two at the end (chevron, −/+, thumb)
extern const HaloFont gl_up_clock, gl_row_lab, gl_row_sub;  // (the time text, the rows' type: studio.py GALLERY_FONTS)
struct UiRun {
  const UiPage *pg;
  HaloList list;
  int armed;  // the row the finger came down on (−1 — none)
  HaloItem title, clock;  // (the time along the top edge: it goes up and shrinks as the page scrolls, as the gallery's)
  HaloItem eb, eb_label;  // the edge button
  HaloItem ring;           // the progress round the edge
  float glow, glow_was;    // the overscroll glow now (0 … 1; − at the bottom) and in the last frame
  uint32_t glow_c;
  char eb_buf[32];
  bool eb_armed;
  HaloItem it[UI_ROWS][UI_IT];
  char buf[UI_ROWS][32];
};
static UiRun *ui_run;  // [UI_LEVELS], in PSRAM (≈ 40 KB of items: internal memory is the Wi-Fi's)
static int ui_cur;     // the level on top (its update runs)
static const UiRow *ui_choice_src;  // the choice whose options page is open (its radio rows read and set it)
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
// (ui_mix — a colour toward another: confirm.h, included before this)

// The page into level k: its rows' heights into the list (the title is the list's header), from its top
static void ui_glow_make();
static void ui_set_page(int k, const UiPage *pg) {
  ui_glow_make();  // (once: the overscroll glow's table, before any pull)
  if (!ui_begin()) return;
  UiRun &R = ui_run[k];
  R.pg = pg, R.armed = -1;
  float h[UI_ROWS + 1];
  h[0] = GL_HEAD_H;
  for (int i = 0; i < pg->n; i++) h[i + 1] = ur_height(pg->rows[i]);
  halo_list_init(R.list, h, pg->n + 1, 64, pg->edge.act ? CY + ui_eb_r(*pg) - UI_EB_FULL - 10 : 466 - 56, true);
}

static void ui_text(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, float sc, int align,
                    uint32_t color, float a, const HaloItem *on, float fit = 0) {
  halo_item_text(it, f, s, mid, x, y, sc, align, color, a, on);
  it.spacing = 0, it.fit = fit;
}
static inline void ui_rrect(HaloItem &it, float x, float y, float w, float h, float r, float sc, uint32_t c, float a) {
  halo_item_rrect(it, x, y, w, h, r, sc, c, a);
}
static inline void ui_circle(HaloItem &it, float x, float y, float r, float sc, uint32_t c, float a) {
  halo_item_circle(it, x, y, r, sc, c, a);
}

// The accent's roles (Material 3's tonal palette from one seed, simply): the primary itself, its deep container (an ON row,
// a secondary action), a light tone on that container, a dark one on the primary
static inline uint32_t ui_container(uint32_t p) { return ui_mix(p, 0x000000, 0.66f); }
static inline uint32_t ui_on_container(uint32_t p) { return ui_mix(p, 0xFFFFFF, 0.6f); }
static inline uint32_t ui_on_primary(uint32_t p) { return ui_mix(p, 0x000000, 0.8f); }
// How much of the edge button shows: all of it at the list's end, nothing 140 px before (it grows in as the end comes,
// as the gallery's)
static inline float ui_eb_r(const UiPage &p) { return p.ring ? UI_RING_R - UI_RING_W / 2 - 4 : UI_EB_R; }
static float ui_eb_height(const UiRun &R) {
  if (!R.pg || !R.pg->edge.act) return 0;
  if (R.pg->ring) return UI_EB_FULL;  // (a live page — Stopwatch, Timer: its Start / Pause stays, whatever is scrolled)
  const float to_end = R.list.scroll - halo_list_min(R.list);
  return UI_EB_FULL * halo_clamp01(1 - to_end / 140);
}

// Its shape is fixed, so it is made once (ui_glow_make, when an app first opens): the lit crescent of the top — rows
// 0 … UI_GLOW_H, at most UI_GLOW_D px in from the rim, the left half (the right is its mirror) — as each row's span and
// its alphas (0 … 255), packed in internal RAM (~10 KB). A frame then only blends: integers, two short runs a row,
// a 4×4 ordered dither (no steps); the bottom is the top mirrored
#define UI_GLOW_H 130
#define UI_GLOW_D 36
static uint8_t *ui_glow_a;                          // the rows' alphas, one after another
static uint16_t ui_glow_at[UI_GLOW_H];              // where each row's run starts in ui_glow_a
static int16_t ui_glow_x0[UI_GLOW_H], ui_glow_x1[UI_GLOW_H];  // its run, left half (x1 < W / 2); x0 > x1 — none
static float ui_glow_alpha(int x, int y) {
  const float dx = x + 0.5f - CX, dy = y + 0.5f - CY, r = sqrtf(dx * dx + dy * dy), th = atan2f(dx, -dy), d = CX - r;
  return d < 0 || d > UI_GLOW_D ? 0 : expf(-(th * th) / (0.5f * 0.5f)) * expf(-d / 11);
}
static void ui_glow_make() {
  if (ui_glow_a) return;
  int total = 0;
  for (int y = 0; y < UI_GLOW_H; y++) {  // (the spans first: how much to keep)
    int lo = W, hi = -1;
    for (int x = 0; x < W / 2; x++)
      if ((int)(ui_glow_alpha(x, y) * 255 + 0.5f)) lo = min(lo, x), hi = max(hi, x);
    ui_glow_x0[y] = lo, ui_glow_x1[y] = hi, ui_glow_at[y] = total;
    if (hi >= lo) total += hi - lo + 1;
  }
#ifdef ESP_PLATFORM
  ui_glow_a = (uint8_t *)heap_caps_malloc(total, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  ui_glow_a = (uint8_t *)malloc(total);
#endif
  if (!ui_glow_a) return;
  for (int y = 0; y < UI_GLOW_H; y++)
    for (int x = ui_glow_x0[y]; x <= ui_glow_x1[y]; x++)
      ui_glow_a[ui_glow_at[y] + x - ui_glow_x0[y]] = (uint8_t)(ui_glow_alpha(x, y) * 255 + 0.5f);
}
static inline void ui_glow_px(uint16_t *p, int A, int cr, int cg, int cb, int d) {
  const uint32_t v = HALO_PX(*p);
  const int R = (v >> 11) << 4, G = (v >> 5 & 63) << 4, B = (v & 31) << 4;
  const int r5 = min(31, (R + ((cr - R) * A >> 8) + d) >> 4), g6 = min(63, (G + ((cg - G) * A >> 8) + d) >> 4),
            b5 = min(31, (B + ((cb - B) * A >> 8) + d) >> 4);
  *p = HALO_PX((r5 << 11) | (g6 << 5) | b5);
}
static void ui_glow(float g, uint32_t c) {
  if (fabsf(g) < 0.004f || !ui_glow_a) return;
  const bool top = g > 0;
  const int K = (int)(fabsf(g) * 0.9f * 256);  // strength, 0 … 230
  const int y0 = top ? 0 : HALO_H - UI_GLOW_H, y1 = top ? UI_GLOW_H : HALO_H;
  // the accent in 565 units × 16 (the dither's fraction)
  const int cr = (c >> 16 & 255) * 31 * 16 / 255, cg = (c >> 8 & 255) * 63 * 16 / 255, cb = (c & 255) * 31 * 16 / 255;
  static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
  for (int y = max(hb.y, y0); y < min(hb.y + hb.rows, y1); y++) {
    const int gy = top ? y : HALO_H - 1 - y;  // (the bottom: the top's table, mirrored)
    const int lo = ui_glow_x0[gy], hi = ui_glow_x1[gy];
    if (lo > hi) continue;
    uint16_t *row = halo_row(y);
    const uint8_t *ar = ui_glow_a + ui_glow_at[gy] - lo, *by = BY + (y & 3) * 4;
    for (int x = max(lo, hb.x0); x <= min(hi, hb.x1); x++)  // the left run
      if (const int A = ar[x] * K >> 8) ui_glow_px(row + x, A, cr, cg, cb, by[x & 3]);
    for (int x = max(W - 1 - hi, hb.x0); x <= min(W - 1 - lo, hb.x1); x++)  // the right: the left mirrored
      if (const int A = ar[W - 1 - x] * K >> 8) ui_glow_px(row + x, A, cr, cg, cb, by[x & 3]);
  }
}

static void ui_layout(int k, const HaloView &v) {
  if (!ui_run || !ui_run[k].pg) return;
  UiRun &R = ui_run[k];
  const UiPage &pg = *R.pg;
  const uint32_t pri = pg.accent;
  const HaloListFx &hf = R.list.fx[0];
  ui_text(R.title, gl_title, pg.title, ui_mid_title, CX, hf.top + GL_HEAD_H * hf.s / 2, hf.s, 1, GL_TEXT, hf.a_bg, &halo_black, 300);  // (every screen's title: the Apps list's 34 px)
  R.title.on_black = true;
  halo_view_commit(R.title, v);
  // The time along the top edge: over the first 36 dp of scrolling it goes up 24 dp and shrinks to 0.5, then is gone
  if (strcmp(ui_clock_of, fc_time)) {
    if (!ui_mid_clock) ui_mid_clock = halo_text_mid(gl_t20, "0123456789");
    // (one time text on every screen: the pager's cut and place — upager.h, the Apps list's Top clock layer)
    halo_text_arc(ui_clock_mask, gl_up_clock, fc_time, halo_text_mid(gl_up_clock, "0123456789"), 197, 0, false, 1, &ui_clock_x, &ui_clock_y);
    strlcpy(ui_clock_of, fc_time, sizeof(ui_clock_of));
  }
  {
    const float tt = 1 - halo_clamp01((R.list.top - R.list.scroll) / (36 * DP)), kk = 0.5f + 0.5f * tt;
    HaloItem &c = R.clock;
    c.kind = HI_MASK, c.mask = &ui_clock_mask, c.color = 0xB7B5C1, c.on_black = true, c.on = &halo_black, c.scale = kk;
    c.x = CX - (CX - ui_clock_x) * kk + ui_clock_mask.w * kk / 2, c.y = ui_clock_y - 24 * DP * (1 - tt) + ui_clock_mask.h * kk / 2;
    c.alpha = tt > 0.02f && ui_clock_mask.bits && halo_clock_valid() ? kk : 0;  // (no time yet: nothing)
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
      ui_text(it[3], gl_row_sub, r.title, halo_text_mid(gl_row_sub, "Hxkdg"), x0 + 30 * s, y0 + h * 0.62f, s, 0, UI_DIM, ac, &halo_black);  // (a section's name: the grey line's type)
      it[3].on_black = true;
    } else if (r.kind == UR_ACTION && r.lo) {  // compact: a small pill, its icon and label centred as one
      ui_rrect(it[0], CX, cy, UI_CPT_W, UI_CPT_H, UI_CPT_H / 2, s, ui_container(acc), ab);
      if (r.text) r.text(buf, 32);
      const char *lb = buf[0] ? buf : r.title;
      // (the card language's name type, as every row's — was the 24 px cut at 0.9)
      static const float mid_l = halo_text_mid(gl_row_lab, "Hxkdg");
      const float tw = halo_text_width(gl_row_lab, lb) * s, iw = r.icon ? 32 * s : 0, x = CX - (tw + iw) / 2;
      if (r.icon) ui_text(it[2], gl_ic24, r.icon, ui_mid_ic, x + 11 * s, cy, 0.9f * s, 1, ui_on_container(acc), ac, &it[0]);
      ui_text(it[3], gl_row_lab, lb, mid_l, x + iw, cy, s, 0, ui_on_container(acc), ac, &it[0]);
    } else if (r.kind == UR_ACTION) {  // a secondary action in the list: the accent's container, a light label
      ui_rrect(it[0], CX, cy, GL_ITEM_W, hh, hh / 2, s, ui_container(acc), ab);
      if (r.text) r.text(buf, 32);
      ui_text(it[3], gl_row_lab, buf[0] ? buf : r.title, halo_text_mid(gl_row_lab, "Hxkdg"), CX, cy, s, 1, ui_on_container(acc), ac, &it[0],
              GL_ITEM_W - 80);  // (the rows' name type)
    } else if (r.kind == UR_SLIDER) {  // its label over a pill: − and + at the ends, a bar with a thumb between
      const int lo = r.lo, hi = r.hi, val = r.get ? constrain(r.get(), lo, hi) : lo;
      ui_text(it[3], gl_row_sub, r.title, halo_text_mid(gl_row_sub, "Hxkdg"), x0 + 30 * s, y0 + 20 * s, s, 0, UI_DIM, ac, &halo_black);
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
      const bool radio = r.kind == UR_RADIO;
      const bool on = (r.kind == UR_TOGGLE && r.get && r.get()) ||
                      (radio && ui_choice_src && ui_choice_src->get && ui_choice_src->get() == r.lo);
      const uint32_t surf = on ? ui_container(acc) : UI_SURF;
      ui_rrect(it[0], CX, cy, GL_ITEM_W, hh, hh / 2, s, surf, ab);
      const float tx = x0 + (r.icon ? 78 : 32) * s;
      if (r.icon) ui_text(it[2], gl_ic24, r.icon, ui_mid_ic, x0 + 44 * s, cy, s, 1, on ? ui_on_container(acc) : acc, ac, &it[0]);
      if (r.kind == UR_CHOICE && r.choices && r.get) strlcpy(buf, r.choices[constrain(r.get(), 0, r.hi - 1)], 32);
      else if (r.text) r.text(buf, 32);
      const bool two = buf[0] != 0;
      const float fit = GL_ITEM_W - (r.icon ? 78 : 32) - (r.kind == UR_TOGGLE ? 96 : 32);
      // the card language (upager.h up_row, gallery.h): a bold name, a lighter grey line under it, the pair centred
      static const float mid_l = halo_text_mid(gl_row_lab, "Hxkdg"), mid_s = halo_text_mid(gl_row_sub, "Hxkdg");
      ui_text(it[3], gl_row_lab, r.title, mid_l, tx, two ? cy - 15 * s : cy, s, 0, on ? ui_on_container(acc) : GL_TEXT, ac, &it[0], fit);
      if (two)
        ui_text(it[4], gl_row_sub, buf, mid_s, tx, cy + 17 * s, s, 0, r.kind == UR_CHOICE ? acc : on ? ui_on_container(acc) : UI_DIM,
                ac, &it[0], fit);
      if (radio) {  // the radio mark: a ring; picked — the primary's disc in it
        const float sx = x0 + w - 44 * s;
        ui_circle(it[5], sx, cy, 13, s, on ? acc : 0x8E9099, ac);
        ui_circle(it[6], sx, cy, 10.5f, s, on ? ui_container(acc) : UI_SURF, ac);
        ui_circle(it[8], sx, cy, 6.5f, s, acc, on ? ac : 0);
      }
      if (r.kind == UR_TOGGLE) {  // the switch: on — the primary's track, a dark thumb at its end; off — grey, small thumb left
        const float sx = x0 + w - 54 * s;
        ui_rrect(it[5], sx, cy, 52, 32, 16, s, on ? acc : UI_TRACK, ac);
        ui_circle(it[8], sx + (on ? 10 : -10) * s, cy, on ? 12 : 8, s, on ? ui_on_primary(acc) : 0x8E9099, ac);
      }
    }
    for (int j = 0; j < UI_IT; j++) halo_view_commit(it[j], v);
  }
  // The progress indicator round the edge: a dark track, the primary from 12 o'clock clockwise (Wear OS's: steps, a timer)
  {
    const float fr = pg.ring ? pg.ring() : -1;
    HaloItem &rg = R.ring;
    rg.kind = HI_RING, rg.x = CX, rg.y = CY, rg.r = UI_RING_R, rg.w = UI_RING_W, rg.v = halo_clamp01(fr), rg.scale = 1, rg.color = pri;
    rg.color2 = 0x262629, rg.alpha = fr >= 0 ? 1 : 0, rg.on_black = true, rg.on = nullptr;
    halo_view_commit(rg, v);
  }
  // The overscroll glow: pulled past the top or the end, one soft light of the accent at that edge — brightest at its
  // middle, fading smoothly sideways and inward (ui_glow draws it per pixel); its band is drawn again when it changes
  {
    const float rest_end = fminf(R.list.top, halo_list_min(R.list));
    const float over = R.list.scroll - R.list.top, under = rest_end - R.list.scroll;
    const float g = over > 0.5f ? halo_clamp01(over / 80) : under > 0.5f ? -halo_clamp01(under / 80) : 0;
    R.glow = g * v.a, R.glow_c = pri;
    if (fabsf(R.glow - R.glow_was) > 0.004f) {
      if (R.glow_was > 0 || R.glow > 0) halo_damage(0, 0, W - 1, UI_GLOW_H);
      if (R.glow_was < 0 || R.glow < 0) halo_damage(0, HALO_H - UI_GLOW_H - 1, W - 1, HALO_H - 1);
      R.glow_was = R.glow;
    }
  }
  // The edge button: the page's main action, hugging the bottom (grows in as the end comes)
  const float eh = ui_eb_height(R);
  HaloItem &e = R.eb;
  const float ebr = ui_eb_r(pg);
  e.kind = HI_EDGE, e.x = CX, e.y = CY, e.r = ebr, e.h = eh, e.w = UI_EB_W, e.v = 48, e.scale = 1;
  e.color = pri, e.alpha = eh > 4 ? halo_clamp01((eh - 8) / 52) : 0, e.on_black = true, e.on = nullptr;
  if (pg.edge.text) pg.edge.text(R.eb_buf, 32);
  else strlcpy(R.eb_buf, pg.edge.title ? pg.edge.title : "", 32);
  ui_text(R.eb_label, gl_row_lab, R.eb_buf, halo_text_mid(gl_row_lab, "Hxkdg"), CX, CY + ebr - eh + fminf(eh, UI_EB_FULL) * 0.42f, 1, 1, ui_on_primary(pri),
          eh > 40 ? halo_clamp01((eh - 40) / 30) : 0, &R.eb, UI_EB_W - 70);
  halo_view_commit(e, v), halo_view_commit(R.eb_label, v);
}

// The overscroll glow, per pixel: the accent over what is there, alpha = strength × e^(−(θ/0.5)²) sideways (θ from the
// edge's middle) × e^(−depth/11) inward from the rim — a smooth light, no rings, no ends
static void ui_draw(int k) {
  if (!ui_run || !ui_run[k].pg) return;
  const UiRun &R = ui_run[k];
  halo_item_draw(R.title);
  for (int i = 0; i < R.pg->n; i++) {
    for (int j = 0; j < UI_IT; j++) halo_item_draw(R.it[i][j]);
    if (R.pg->rows[i].kind == UR_CUSTOM && R.pg->rows[i].cdraw) R.pg->rows[i].cdraw();
  }
  halo_item_draw(R.clock), halo_item_draw(R.ring);
  ui_glow(R.glow, R.glow_c);
  halo_item_draw(R.eb), halo_item_draw(R.eb_label);  // (over the list)
}

static void ui_open(const UiPage *pg);
// A choice opens its options as a page of radio rows (Wear OS: a selection list — a tap picks one and goes back); a tap
// on the row itself no longer changes anything, so a stray tap can't change a setting
static UiPage ui_choice_page;
static void ui_open_choice(const UiRow &r) {
  ui_choice_src = &r;
  const uint32_t acc = r.accent ? r.accent : ui_run && ui_run[ui_cur].pg ? ui_run[ui_cur].pg->accent : 0xA8C7FA;
  ui_choice_page = {r.title, {}, 0, acc};
  for (int i = 0; i < r.hi && ui_choice_page.n < UI_ROWS; i++) {
    UiRow o = {};
    o.kind = UR_RADIO, o.title = r.choices[i], o.lo = i, o.accent = r.accent;
    ui_choice_page.rows[ui_choice_page.n++] = o;
  }
  ui_open(&ui_choice_page);
}
static void ui_update(int k, float dt) {
  if (!ui_run || !ui_run[k].pg) return;
  ui_cur = k;
  UiRun &R = ui_run[k];
  HaloFinger &f = halo_finger;
  halo_list_update(R.list, dt, true, false);
  if (f.pressed) {
    const float eh = ui_eb_height(R);
    R.eb_armed = eh > 40 && f.y0 > CY + ui_eb_r(*R.pg) - eh && fabsf(f.x0 - CX) < UI_EB_W / 2;  // (the edge button is over the list)
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
      case UR_CHOICE:
        if (r.get && r.set && r.hi > 0 && r.choices) ui_open_choice(r);
        break;
      case UR_RADIO:  // (an option of a choice: picked — the setting changes, back to where it was asked)
        if (ui_choice_src && ui_choice_src->set) ui_choice_src->set(r.lo), halo_nav_back();
        break;
      case UR_SLIDER:
        if (r.get && r.set) {
          const int d = f.x < CX - 60 ? -1 : f.x > CX + 60 ? 1 : 0;
          if (d) r.set(constrain(r.get() + d, r.lo, r.hi));
        }
        break;
      case UR_ACTION:  // (compact: only on the pill)
        if (r.act && (!r.lo || fabsf(f.x - CX) < UI_CPT_W / 2 + 12)) r.act();
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
