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
#define UP_IT 192
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
  bool edge;  // an edge button holds the bottom: no page dots over it
  UpBlob b[UP_BLOBS];
};
struct UpApp {
  int (*count)();                                              // pages now (1 … UP_PAGES)
  void (*lay)(int p, UpPage &P, float a, const HaloView &v);  // place page p's items (as at rest: v is shifted)
  void (*tap)(int p, float x, float y);                       // a tap on page p (screen coordinates)
  void (*update)(int p, float dt);                            // every frame, the page in front (its own drags)
  float (*ring)();                                             // the rim's progress 0 … 1 (< 0 — the track alone)
  uint32_t accent;
  float (*height)(int p);  // a page taller than the screen scrolls up and down (nullptr, ≤ 466 — it does not)
  bool wrap;               // the pages go round: past the last — the first, before the first — the last
  // A page as a Wear OS list (TransformingLazyColumn: lib/Halo's HaloList — the gallery's scroll): its items one under
  // another, each shrinking to 0.6 and fading at the circle's edges, inertia and rubber, the scroll indicator at the
  // right rim. items(p) > 0 — page p is a list; item k is item_h(p, k) px tall and laid out by lay_item in its own
  // coordinates (x — the screen's, y — from its top), abg — its card's alpha, ac — its content's; item 0 — a header (does
  // not shrink). The app's tap gets screen coordinates: up_list_hit says which item
  int (*items)(int p) = nullptr;
  float (*item_h)(int p, int k) = nullptr;
  void (*lay_item)(int p, int k, UpPage &P, float abg, float ac, const HaloView &v) = nullptr;
};
static const UpApp *up_app;
static UpPage *up_pg;  // [UP_PAGES], PSRAM
static HaloSpring up_pos;
static HaloSpringK K_UP;
static int up_page, up_owner;
static bool up_drag, up_vis[UP_PAGES], up_vdrag;
static HaloSpring up_sy[UP_PAGES];  // each page's scroll (px; 0 — its top)
static float up_vy0, up_vg0;
static int up_vowner;
static float up_x0, up_g0;
static HaloItem up_track, up_fill, up_track2, up_fill2, up_clock, up_dot[UP_PAGES];
// The list pages (UpApp.items): one HaloList a page, rebuilt when its items' heights change; the scroll indicator
#define UP_LIST_TOP 64.0f
#define UP_LIST_BOTTOM 420.0f
// (each pager its own: the quick settings' pager lives beside the app's — upager.h's swap carries the pointer)
struct UpLists {
  HaloList l[UP_PAGES];
  uint32_t key[UP_PAGES];
};
static UpLists *up_lsx;
#define up_ls (up_lsx->l)
#define up_ls_key (up_lsx->key)
static HaloItem up_sc[3];
static HaloSpring up_sc_pos, up_sc_vis;
static uint32_t up_sc_idle;
static inline bool up_is_list(int p) { return up_app && up_lsx && up_app->items && up_app->items(p) > 0; }
static void up_list_sync(int p) {  // (heights from the app; a change — the list again, from the top)
  const int n = min(up_app->items(p), HALO_LIST_MAX);
  float h[HALO_LIST_MAX];
  uint32_t key = 2166136261u ^ n;
  for (int k = 0; k < n; k++) h[k] = up_app->item_h(p, k), key = (key ^ (uint32_t)(h[k] * 4)) * 16777619u;
  if (key == up_ls_key[p]) return;
  const bool fresh = !up_ls_key[p];
  const float was = up_ls[p].scroll;
  up_ls_key[p] = key;
  halo_list_init(up_ls[p], h, n, UP_LIST_TOP, UP_LIST_BOTTOM, true);
  up_ls[p].width = 410;  // (the pills' width: near the rim they shrink to the circle's chord — round ends, not sliced)
  if (!fresh) up_ls[p].scroll = constrain(was, halo_list_min(up_ls[p]), UP_LIST_TOP);  // (same place, if it still fits)
}
static int up_list_hit(int p, float y) { return up_is_list(p) ? halo_list_hit(up_ls[p], y) : -1; }
static HaloMask up_clock_mask;
static char up_clock_of[8];
static float up_clock_x, up_clock_y;
static float up_mid_hero, up_mid_disp, up_mid_pick, up_mid_title, up_mid_num, up_mid_pre, up_mid_lab, up_mid_ic;

static void up_commands();
static bool up_begin() {
  up_commands();
  if (up_pg && up_lsx) return true;
#ifdef __EMSCRIPTEN__
  if (!up_pg) up_pg = (UpPage *)calloc(UP_PAGES, sizeof(UpPage));
  if (!up_lsx) up_lsx = (UpLists *)calloc(1, sizeof(UpLists));
#else
  if (!up_pg) up_pg = (UpPage *)heap_caps_calloc(UP_PAGES, sizeof(UpPage), MALLOC_CAP_SPIRAM);
  if (!up_lsx) up_lsx = (UpLists *)heap_caps_calloc(1, sizeof(UpLists), MALLOC_CAP_SPIRAM);
#endif
  if (!up_pg || !up_lsx) return false;
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
  up_vdrag = false;
  for (int p = 0; p < UP_PAGES; p++) up_vis[p] = true, halo_spring_set(up_sy[p], 0);  // (each hidden once below, if not seen)
  if (app->items) {  // (a list app: its lists from the top, the indicator hidden; another app's pager leaves them be)
    for (int p = 0; p < UP_PAGES; p++) up_ls_key[p] = 0;
    halo_spring_set(up_sc_vis, 0), up_sc_idle = 0;
  }
  for (HaloItem &s : up_sc) s.alpha = 0, halo_item_commit(s);
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
  ui_rrect(b, (x0 + x1) / 2, (y0 + y1) / 2, x1 - x0, y1 - y0, r, 1, c, a);
  up_commit(b, v);
}
static HaloItem &up_disc(UpPage &P, float x, float y, float r, uint32_t c, float a, const HaloView &v) {
  HaloItem &b = up_item(P);
  ui_circle(b, x, y, r, 1, c, a);
  up_commit(b, v);
  return b;
}
// The page dots live at 6 o'clock on the rim (Wear OS's HorizontalPageIndicator): with more than one page, the rim's
// arcs leave them a gap there (up_gap: its half-angle, radians; 0 — one page, no gap) — an arc never runs through them
#define UP_DOT_R 222.0f   // the dots' radius: 2 dp in from the edge, 6 dp dots (Wear OS: PageIndicator.kt)
#define UP_DOT_PITCH 20.0f  // centre to centre, px along the rim (6 dp dots, 4 dp apart)
static float up_gap;
static void up_arc_seg(UpPage &P, float r, float w, float a0, float a1, uint32_t c, float a, const HaloView &v) {
  HaloItem &it = up_item(P);
  it.kind = HI_ARC, it.x = CX, it.y = CY, it.r = r, it.w = w, it.a0 = a0, it.a1 = a1, it.scale = 1, it.color = c;
  it.alpha = a1 > a0 ? a : 0, it.on_black = true, it.on = nullptr;
  up_commit(it, v);
}
// An arc along the rim (radians from 12 o'clock, clockwise), cut round the dots' gap at 6 o'clock: two items, always
// (the second hidden when it is not needed — the page's items keep their order frame to frame)
static void up_arc(UpPage &P, float r, float w, float a0, float a1, uint32_t c, float a, const HaloView &v) {
  const float g0 = HALO_PI - up_gap, g1 = HALO_PI + up_gap;
  if (up_gap <= 0 || a1 <= g0 || a0 >= g1) return up_arc_seg(P, r, w, a0, a1, c, a, v), up_arc_seg(P, r, w, 0, 0, c, 0, v);
  up_arc_seg(P, r, w, a0, fminf(a1, g0), c, a0 < g0 ? a : 0, v);
  up_arc_seg(P, r, w, fmaxf(a0, g1), a1, c, a1 > g1 ? a : 0, v);
}
// Is the finger down on this box (page coordinates, through the view) and not yet a drag? — a control's press
static inline bool up_pressed(const HaloView &v, float x0, float y0, float x1, float y1) {
  const HaloFinger &f = halo_finger;
  return f.down && !f.dragging && f.x0 >= halo_view_x(v, x0) && f.x0 <= halo_view_x(v, x1) && f.y0 >= halo_view_y(v, y0) &&
         f.y0 <= halo_view_y(v, y1);
}
// The primary: a filled shape (circle → four lobes by t) and its glyph in the dark ink (f — an app's own glyph size;
// nullptr — the study's 43 px)
static void up_primary(UpPage &P, float x, float y, float r, float t, const char *icon, uint32_t fill, uint32_t ink,
                       float a, const HaloView &v, const HaloFont *f = nullptr) {
  const bool press = a > 0.5f && up_pressed(v, x - r, y - r, x + r, y + r);  // (pressed: a touch smaller and lighter)
  if (press) r *= 0.94f, fill = ui_mix(fill, 0xFFFFFF, 0.12f);
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
  const bool press = a > 0.5f && up_pressed(v, x - r, y - r, x + r, y + r);  // (pressed: a touch smaller and lighter)
  HaloItem &d = up_disc(P, x, y, press ? r * 0.92f : r, press ? ui_mix(bg, 0xFFFFFF, 0.12f) : bg, a, v);
  up_text(P, f, icon, halo_text_mid(f, icon), x, y, 1, fg, a, v, &d);
}
// ---------- Wear OS's pill rows (Button / SwitchButton / RadioButton: docs/references/wear-os, the scrolling settings
// and toggle-button layouts) ----------
// One control a row, never a card holding blocks: a full pill (corners h/2) from x0 to x1 around y, its label (and a
// second line) from the left in the house's type, its control at the right end — a switch, a radio, or nothing (a
// plain button). on: 0 … 1 (a spring) — the pill fills with the accent and its text turns to the dark ink, as Wear OS's
// selected toggles. The pill's item comes back (its tap box: x0 … x1, y ± h/2)
enum : uint8_t { UP_ROW_BUTTON, UP_ROW_SWITCH, UP_ROW_RADIO };
#define UP_ROW_X0 47.0f   // the rows' ends on a non-scrolling page (Wear OS: full width less 5.2 % a side)
#define UP_ROW_X1 419.0f
#define UP_ROW_H 72.0f
#define UP_ROW_OFF 0x2E3034  // the house's card (gallery.h's)
extern const HaloFont gl_row_lab, gl_row_sub;  // (Wear OS M3's LabelMedium / LabelSmall: 30 / 26 px, weight 500, width 110;
                                              // on the panel, 28 Nov 2026: clearer than the gallery's t24 / t20 at 620)
static HaloItem &up_row(UpPage &P, float x0, float x1, float y, float h, const char *label, const char *sub,
                        uint8_t kind, float on, uint32_t acc, uint32_t ink, float a, const HaloView &v) {
  // Wear OS M3's tokens (SwitchButtonTokens, FilledTonalButtonTokens): unchecked — the surface container, the label
  // on-surface, the second line on-surface-variant; checked — the PRIMARY CONTAINER (the accent dark and tonal, not the
  // accent itself), the label on-primary-container (the accent light), the second line the same, dimmer. The type is
  // the Apps list's contrast (the user, 28 Nov 2026): a bold name, a smaller lighter grey line under it
  (void)ink;
  on = halo_clamp01(on);
  const bool fills = kind != UP_ROW_BUTTON;
  // pressed (the finger came down on it and has not become a drag): lighter, its corners squeezed — the Apps list's
  // cards' press, so a tap answers at once
  const bool press = a > 0.5f && up_pressed(v, x0, y - h / 2, x1, y + h / 2);
  uint32_t box = ui_mix(UP_ROW_OFF, ui_mix(acc, 0x000000, 0.68f), fills ? on : 0);  // (the primary container)
  if (press) box = ui_mix(box, 0xFFFFFF, 0.10f);
  HaloItem &pill = up_item(P);
  ui_rrect(pill, (x0 + x1) / 2, y, x1 - x0, h, press ? h * 0.36f : h / 2, 1, box, a);
  pill.on_black = true;
  up_commit(pill, v);
  const uint32_t lab_on = ui_mix(acc, 0xFFFFFF, 0.55f);  // (on-primary-container)
  const uint32_t txt = ui_mix(0xFFF8EE, lab_on, fills ? on : 0), txt2 = ui_mix(0x9E9E9E, ui_mix(lab_on, box, 0.25f), fills ? on : 0);
  const float lx = x0 + 32, room = x1 - x0 - 32 - (kind == UP_ROW_BUTTON ? 32 : 98);
  auto text = [&](const HaloFont &f, const char *s, float ty, uint32_t c) {
    HaloItem &t = up_item(P);
    ui_text(t, f, s, halo_text_mid(f, "Hxkdg"), lx, ty, 1, 0, c, a, &pill, room);
    up_commit(t, v);
  };
  if (sub && *sub) text(gl_row_lab, label, y - 15, txt), text(gl_row_sub, sub, y + 17, txt2);
  else text(gl_row_lab, label, y, txt);
  const float cx = x1 - 32 - 26;
  if (kind == UP_ROW_SWITCH) {  // off — a grey track, the thumb left; on — the accent's track, a dark thumb right
    HaloItem &tr = up_item(P);
    ui_rrect(tr, cx, y, 52, 32, 16, 1, ui_mix(0x45474C, acc, on), a);
    tr.on_black = true, up_commit(tr, v);
    HaloItem &th = up_item(P);
    ui_circle(th, cx - 10 + 20 * on, y, 10 + 2 * on, 1, ui_mix(0x9E9E9E, ui_mix(acc, 0x000000, 0.75f), on), a);
    th.on_black = true, up_commit(th, v);
  } else if (kind == UP_ROW_RADIO) {  // a ring, its middle filled when picked — in the accent
    HaloItem &rg = up_item(P);
    rg.kind = HI_RING, rg.x = cx + 14, rg.y = y, rg.r = 11, rg.w = 2.5f, rg.v = 1, rg.scale = 1;
    rg.color = ui_mix(0x9E9E9E, acc, on), rg.color2 = rg.color, rg.alpha = a, rg.on_black = true, rg.on = nullptr;
    up_commit(rg, v);
    HaloItem &dt = up_item(P);
    ui_circle(dt, cx + 14, y, 6, 1, acc, a * on);
    dt.on_black = true, up_commit(dt, v);
  }
  return pill;
}

// A settings page as Wear OS's: a scrolling list (UpApp.items) — its title, then a pill row an item, 52 dp each
#define UP_SET_ROW 104.0f
#define UP_SET_X0 (CX - 205)
#define UP_SET_X1 (CX + 205)
static inline float up_set_h(int k) { return k ? UP_SET_ROW : 70; }
static inline void up_set_title(UpPage &P, const char *t, float ac, const HaloView &v) {
  up_text(P, gl_up_title, t, up_mid_title, CX, 34, 1, UP_WHITE, ac, v);  // (at y 98 at rest: every app's title)
}
static inline HaloItem &up_set_row(UpPage &P, const char *label, const char *sub, uint8_t kind, float on, uint32_t acc,
                                   uint32_t ink, float ac, const HaloView &v) {
  return up_row(P, UP_SET_X0, UP_SET_X1, UP_SET_ROW / 2, UP_SET_ROW, label, sub, kind, on, acc, ink, ac, v);
}

static inline bool up_hit(float x, float y, float cx, float cy, float r) {
  return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= (r + 6) * (r + 6);
}
static inline bool up_in(float x, float y, float x0, float y0, float x1, float y1) {
  return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}
// ---------- Wear OS's volume screen (docs/references/wear-os/surfaces/apps/layouts/non-scrolling/
// app-layout-non-scrolling-volume.png, m2-5 steppers): one for every app with a speaker level ----------
// Louder at the top, quieter at the bottom — tonal stadium buttons, the one at its end dimmed —, the output named in a
// row between them ("Speaker" / "Volume 60 %"), the level an arc on the left rim filling upwards. No title: the buttons
// are the page. (Radio and Music had a big number with − and + side by side and rim marks round both sides; 28 Nov 2026)
#define UP_VB_W 148.0f
#define UP_VB_H 88.0f
#define UP_VB_UP 116.0f
#define UP_VB_DN 350.0f
static char up_vol_line[24];
static void up_vol_button(UpPage &P, float y, const char *icon, const HaloFont &f, uint32_t acc, float a, const HaloView &v) {
  const float x0 = CX - UP_VB_W / 2, x1 = CX + UP_VB_W / 2;
  const bool press = a > 0.5f && up_pressed(v, x0, y - UP_VB_H / 2, x1, y + UP_VB_H / 2);
  uint32_t box = ui_mix(acc, 0x000000, 0.68f);  // (the accent's tonal container, as a checked row)
  if (press) box = ui_mix(box, 0xFFFFFF, 0.10f);
  HaloItem &b = up_item(P);
  ui_rrect(b, CX, y, UP_VB_W, UP_VB_H, press ? UP_VB_H * 0.36f : UP_VB_H / 2, 1, box, a);
  b.on_black = true;
  up_commit(b, v);
  up_text(P, f, icon, halo_text_mid(f, icon), CX, y, 1, acc, a, v, &b);
}
// level 0 … 10; track — the arc's empty part; f, plus, minus — the app's glyphs
static void up_volume(UpPage &P, int level, uint32_t acc, uint32_t track, const HaloFont &f, const char *plus,
                      const char *minus, float a, const HaloView &v) {
  const float A0 = 212 * HALO_PI / 180, SW = 116 * HALO_PI / 180;  // (8 to 10 o'clock and a bit)
  up_arc(P, UP_RING_R, UP_RING_W, A0, A0 + SW, track, a, v);
  up_arc(P, UP_RING_R, UP_RING_W, A0, A0 + SW * level / 10.0f, acc, level > 0 ? a : 0, v);
  up_vol_button(P, UP_VB_UP, plus, f, acc, level < 10 ? a : a * 0.38f, v);
  snprintf(up_vol_line, sizeof(up_vol_line), "Volume %d %%", level * 10);
  up_row(P, CX - 150, CX + 150, CY, 96, "Speaker", up_vol_line, UP_ROW_BUTTON, 0, acc, 0, a, v);
  up_vol_button(P, UP_VB_DN, minus, f, acc, level > 0 ? a : a * 0.38f, v);
}
// A tap on it: +1, −1 or 0
static int up_volume_tap(float x, float y) {
  const float x0 = CX - UP_VB_W / 2 - 8, x1 = CX + UP_VB_W / 2 + 8, h = UP_VB_H / 2 + 8;
  if (up_in(x, y, x0, UP_VB_UP - h, x1, UP_VB_UP + h)) return 1;
  if (up_in(x, y, x0, UP_VB_DN - h, x1, UP_VB_DN + h)) return -1;
  return 0;
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
      if (!up_app->wrap) {
        if (o < 0) o = -halo_rubber(-o * W) / W;
        else if (o > n - 1) o = n - 1 + halo_rubber((o - (n - 1)) * W) / W;
      }
      up_pos.x = o, up_pos.v = -f.vx / W, up_pos.target = o;
    } else {
      up_drag = false;
      const float d = up_pos.x - up_page, v = up_pos.v;
      // a third of the way or a fling faster than 0.9 page/s, either way (a fling wins over the way)
      int to = up_page + (halo_land(d, v, 0.33f, 0.9f, true) ? 1 : halo_land(-d, -v, 0.33f, 0.9f, true) ? -1 : 0);
      if (up_app->wrap) {  // (round: the same place, counted from the other end — nothing jumps on screen)
        if (to < 0) to += n, up_pos.x += n, up_g0 += n;
        else if (to >= n) to -= n, up_pos.x -= n, up_g0 -= n;
      }
      up_page = constrain(to, 0, n - 1), up_pos.target = up_page;
    }
  } else {
    halo_spring_run(up_pos, K_UP, dt);
  }
  // Scrolling the page in front up and down (a page taller than the screen): under the finger, rubber past the ends,
  // a fling carries on and settles on the spring
  const bool list = up_is_list(up_page);
  if (list) {
    up_list_sync(up_page);
    HaloList &l = up_ls[up_page];
    halo_list_update(l, dt, !up_drag);
    if (l.flinging || f.owner == &l) up_sc_idle = halo_ms();
    up_sc_pos.target = halo_list_progress(l), up_sc_vis.target = up_sc_idle && halo_ms() - up_sc_idle < 2000 ? 1 : 0;
    halo_spring_run(up_sc_pos, K_UP, dt), halo_spring_run(up_sc_vis, K_UP, dt);
  }
  const float hmax = !list && up_app->height ? fmaxf(0, up_app->height(up_page) - HALO_H) : 0;
  HaloSpring &sy = up_sy[up_page];
  if (hmax > 0 && !up_drag && f.drag_start && !f.horiz && halo_finger_claim(&up_vowner)) up_vdrag = true, up_vy0 = f.y, up_vg0 = sy.x;
  if (up_vdrag) {
    if (f.down && f.owner == &up_vowner) {
      float y = up_vg0 + (up_vy0 - f.y);
      if (y < 0) y = -halo_rubber(-y);
      else if (y > hmax) y = hmax + halo_rubber(y - hmax);
      sy.x = y, sy.v = -f.vy, sy.target = y;
    } else {
      up_vdrag = false, sy.target = constrain(sy.x - f.vy * 0.3f, 0.0f, hmax);
    }
  } else {
    halo_spring_run(sy, K_UP, dt);
  }
  if (!up_drag && !up_vdrag && f.tap && !f.owner && fabsf(up_pos.x - up_page) < 0.05f && up_app->tap)
    up_app->tap(up_page, f.x, f.y + (list ? 0 : sy.x));  // (in the page's own coordinates: scrolled; a list's — the screen's)
  if (up_app->update) up_app->update(up_page, dt);
}

static void up_layout(const HaloView &v) {
  if (!up_app || !up_pg) return;
  const int n = up_count();
  const uint32_t acc = up_app->accent;
  // The rim: the track, the progress over it (fixed: it does not move with the pages)
  const float fr = up_app->ring ? up_app->ring() : -2;
  // How far the page in front is scrolled: the time (and on a list, the rim's ring) goes away over its first 60 px
  const bool lst = up_is_list(up_page);
  const float sc = up_page >= UP_PAGES ? 0 : lst ? up_ls[up_page].top - up_ls[up_page].scroll : up_sy[up_page].x;
  // The rim's ring is the first page's, and only while it carries progress: no empty track framing an idle screen
  // (the user, 28 Nov 2026: the arcs fought the dots and the titles); it fades as the pages swipe away from the first
  const float gone = halo_clamp01(sc / 60), ring_a = (lst ? 1 - gone : 1) * halo_clamp01(1 - fabsf(up_pos.x));
  // (the dots' gap at 6 o'clock with more than one page: the half-angle the dots and a margin take — up_arc cuts round it)
  up_gap = n > 1 ? ((n - 1) * UP_DOT_PITCH / 2 + 22) / UP_DOT_R : 0;
  const float g0 = HALO_PI - up_gap, g1 = HALO_PI + up_gap, end = UP_A0 + UP_SWEEP * halo_clamp01(fr);
  for (HaloItem *it : {&up_track, &up_fill, &up_track2, &up_fill2}) {
    it->kind = HI_ARC, it->x = CX, it->y = CY, it->r = UP_RING_R, it->w = UP_RING_W, it->scale = 1;
    it->on_black = true, it->on = nullptr;
  }
  const bool cut = up_gap > 0;
  up_track.a0 = UP_A0, up_track.a1 = cut ? g0 : UP_A0 + UP_SWEEP, up_track2.a0 = g1, up_track2.a1 = UP_A0 + UP_SWEEP;
  up_track.color = up_track2.color = UP_TRACK;
  up_track.alpha = fr > 0.001f ? ring_a : 0, up_track2.alpha = cut ? up_track.alpha : 0;
  up_fill.a0 = UP_A0, up_fill.a1 = cut ? fminf(end, g0) : end, up_fill2.a0 = g1, up_fill2.a1 = end;
  up_fill.color = up_fill2.color = acc;
  up_fill.alpha = fr > 0.001f ? ring_a : 0, up_fill2.alpha = cut && end > g1 ? up_fill.alpha : 0;
  for (HaloItem *it : {&up_track, &up_fill, &up_track2, &up_fill2}) halo_view_commit(*it, v);
  for (int p = 0; p < UP_PAGES; p++) {
    UpPage &P = up_pg[p];
    float d = p - up_pos.x;
    if (up_app->wrap && n > 1) d -= n * roundf(d / n);  // (round: the nearest way)
    const float dx = d * W;
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
    const bool list = up_is_list(p);
    pv.x += dx * v.s, pv.y -= (list ? 0 : up_sy[p].x) * v.s;  // (moved by the swipe, and up by its scroll)
    const int was = P.n, wasb = P.nb;
    P.n = 0, P.nb = 0, P.edge = false;
    if (list) {
      up_list_sync(p);
      const HaloList &l = up_ls[p];
      for (int k = 0; k < l.n; k++) {
        if (!halo_list_item_visible(l, k)) continue;
        const HaloListFx &fx = l.fx[k];
        // the item's own view: its top at fx.top, scaled by fx.s about the screen's middle line, through the page's
        HaloView iv = pv;
        iv.s = pv.s * fx.s, iv.y = pv.y + (fx.top - HALO_H / 2.0f) * pv.s + HALO_H / 2.0f * iv.s;
        up_app->lay_item(p, k, P, fx.a_bg, fx.a_c, iv);
      }
    } else {
      up_app->lay(p, P, 1, pv);
    }
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
  // (a page scrolled down: the time goes up and away with its first 60 px, as Wear OS's time text — the content never
  // runs under it)
  if (lst) {  // (a list: the scroll indicator — ScrollIndicator, as the gallery's)
    const HaloList &l = up_ls[up_page];
    const float r = 226, len = 100 / r, gp = 18 / r, a0 = HALO_PI / 2 - len / 2;
    const float size = len * constrain(halo_list_visible(l), 0.3f, 0.7f), t0 = a0 + (len - size) * halo_clamp01(up_sc_pos.x);
    const float t1 = t0 + size, al = up_sc_vis.x > 0.01f ? fminf(1, up_sc_vis.x) : 0;
    const float from[3] = {a0, t1 + gp, t0}, to[3] = {t0 - gp, a0 + len, t1};
    for (int j = 0; j < 3; j++) {
      HaloItem &s = up_sc[j];
      s.kind = HI_ARC, s.x = CX, s.y = CY, s.r = r, s.w = 10, s.scale = 1, s.a0 = from[j], s.a1 = to[j];
      s.color = j == 2 ? 0xC6C6C6 : 0x333333, s.on_black = true, s.on = nullptr, s.alpha = from[j] < to[j] ? al : 0;
      halo_view_commit(s, v);
    }
  }
  // (no time yet — "--:--" before the first sync: the clock's font has no "-", only the colon showed, a "⋮": nothing)
  const bool known = halo_clock_valid();
  c.x = CX, c.y = 38 - 50 * gone, c.alpha = up_clock_mask.bits && known ? 1 - gone : 0;
  halo_view_commit(c, v);
  // The page dots along the rim at 6 o'clock (Wear OS's HorizontalPageIndicator): 6 dp dots 4 dp apart, the one in front
  // in the accent, the others white at 30 %, the first page on the left (none with one page)
  for (int p = 0; p < UP_PAGES; p++) {
    HaloItem &d = up_dot[p];
    const float on = halo_clamp01(1 - fabsf(up_pos.x - p)), th = HALO_PI + ((n - 1) / 2.0f - p) * (UP_DOT_PITCH / UP_DOT_R);
    d.kind = HI_CIRCLE, d.x = CX + UP_DOT_R * sinf(th), d.y = CY - UP_DOT_R * cosf(th), d.r = 5 + on, d.scale = 1;
    const bool edge = up_pg[up_page].edge;
    d.color = ui_mix(0x4D4D4D, acc, on), d.alpha = n > 1 && p < n && !edge ? 1 - gone : 0, d.on_black = true, d.on = nullptr;  // (scrolled, an edge button: away)
    halo_view_commit(d, v);
  }
}

static void up_draw() {
  if (!up_app || !up_pg) return;
  halo_item_draw(up_track), halo_item_draw(up_fill), halo_item_draw(up_track2), halo_item_draw(up_fill2);
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
  for (HaloItem &s : up_sc) halo_item_draw(s);
}

// ---------- More than one pager at once (the launcher under an app, the quick settings over one) ----------
// The pager is one set of globals; a screen that keeps its own pager beside the app's swaps its state in around
// up_open / up_update / up_layout (one task, one after another) — and draws from its own copy with up_draw_of, never by
// swapping: the bands are drawn on both cores at once
struct UpState {
  const UpApp *app = nullptr;
  UpPage *pg = nullptr;
  HaloSpring pos = {0, 0, 0};
  int page = 0;
  bool drag = false, vis[UP_PAGES] = {};
  float x0 = 0, g0 = 0;
  HaloItem track, fill, track2, fill2, clock, dot[UP_PAGES];
  UpLists *ls = nullptr;  // (its lists, its scroll indicator)
  HaloItem sc[3];
  HaloSpring sc_pos = {0, 0, 0}, sc_vis = {0, 0, 0};
  uint32_t sc_idle = 0;
};
static void up_swap(UpState &s) {
  std::swap(s.app, up_app), std::swap(s.pg, up_pg), std::swap(s.pos, up_pos), std::swap(s.page, up_page);
  std::swap(s.drag, up_drag), std::swap(s.x0, up_x0), std::swap(s.g0, up_g0);
  for (int p = 0; p < UP_PAGES; p++) std::swap(s.vis[p], up_vis[p]), std::swap(s.dot[p], up_dot[p]);
  std::swap(s.track, up_track), std::swap(s.fill, up_fill), std::swap(s.clock, up_clock);
  std::swap(s.track2, up_track2), std::swap(s.fill2, up_fill2);
  std::swap(s.ls, up_lsx), std::swap(s.sc_pos, up_sc_pos), std::swap(s.sc_vis, up_sc_vis), std::swap(s.sc_idle, up_sc_idle);
  for (int j = 0; j < 3; j++) std::swap(s.sc[j], up_sc[j]);
}
// fn with s as the pager's state (up_open, up_update, up_layout, up_audit of a pager kept aside)
template <class F> static inline void up_with(UpState &s, F fn) {
  up_swap(s), fn(), up_swap(s);
}
static void up_draw_of(const UpState &s) {  // (up_draw of a state kept aside)
  if (!s.app || !s.pg) return;
  halo_item_draw(s.track), halo_item_draw(s.fill), halo_item_draw(s.track2), halo_item_draw(s.fill2);
  for (int p = 0; p < UP_PAGES; p++) {
    if (!s.vis[p]) continue;
    const UpPage &P = s.pg[p];
    int k = 0;
    for (int i = 0; i < P.n; i++) {
      while (k < P.nb && P.b[k].at == i) up_blob_draw(P.b[k++]);
      halo_item_draw(P.it[i]);
    }
    while (k < P.nb) up_blob_draw(P.b[k++]);
  }
  halo_item_draw(s.clock);
  for (const HaloItem &d : s.dot) halo_item_draw(d);
  for (const HaloItem &c : s.sc) halo_item_draw(c);
}

// ---------- The audit (serial: upcheck) — what overlaps on the page in front, what comes near the rim ----------
// Every two shown items whose boxes cross by more than 2 px, unless one is the other's ground (on) or holds it whole
// (a text on its card); every text or shape whose box reaches past r 221 (12 px from the rim). Measured, not eyeballed
static const UpPage *up_over;  // a page an app lays itself over its pages (a panel on top): upcheck audits it instead
static void up_audit() {
  if (!up_over && (!up_app || !up_pg)) return (void)LOG("upcheck: no paged app open\n");
  const UpPage &P = up_over ? *up_over : up_pg[up_page];
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
      if (a.kind == HI_ARC || b.kind == HI_ARC) continue;  // (a rim arc's box covers the dial; its ink is at the rim — the
                                                           // near-the-rim check is the one that matters for it)
      if (b.on == &a || a.on == &b || holds(a, b) || holds(b, a)) continue;
      float A[4], B[4];
      ink(a, A), ink(b, B);
      const float ox = fminf(A[2], B[2]) - fmaxf(A[0], B[0]), oy = fminf(A[3], B[3]) - fmaxf(A[1], B[1]);
      if (ox > 2 && oy > 2) LOG("upcheck: \"%s\" overlaps \"%s\" (%.0f x %.0f px)\n", name(a), name(b), ox, oy), bad++;
    }
  }
  if (up_over) LOG("upcheck: the panel on top — %d problem%s\n", bad, bad == 1 ? "" : "s");
  else LOG("upcheck: page %d — %d problem%s\n", up_page + 1, bad, bad == 1 ? "" : "s");
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
  bool wrap;  // round (Wear OS's time pickers: 59 sits over 00): past the last — the first
};
static inline int up_picker_wrap(const UpPicker &k, int v) {
  const int n = k.hi - k.lo + 1;
  return k.lo + ((v - k.lo) % n + n) % n;
}
static void up_picker_update(UpPicker &k, float cx, float half_w, float dt) {
  const HaloFinger &f = halo_finger;
  if (f.drag_start && !f.horiz && fabsf(f.x0 - cx) < half_w && halo_finger_claim(&k.owner)) k.drag = true, k.y0 = f.y, k.g0 = k.pos.x;
  if (k.drag) {
    if (f.down && f.owner == &k.owner) {
      const float x = k.g0 + (k.y0 - f.y) / 62;
      k.pos.x = k.wrap ? x : constrain(x, (float)k.lo, (float)k.hi), k.pos.target = k.pos.x;
    } else {
      k.drag = false, k.pos.target = k.wrap ? roundf(k.pos.x) : constrain(roundf(k.pos.x), (float)k.lo, (float)k.hi);
    }
  } else {
    halo_spring_run(k.pos, K_UP, dt);
    if (k.wrap && fabsf(k.pos.x - k.pos.target) < 0.002f && fabsf(k.pos.v) < 0.01f) {  // (settled: back into lo … hi)
      const float t = (float)up_picker_wrap(k, (int)roundf(k.pos.target));
      k.pos.x += t - k.pos.target, k.pos.target = t;
    }
  }
}
static inline int up_picker_value(const UpPicker &k) {
  const int v = (int)roundf(k.pos.target);
  return k.wrap ? up_picker_wrap(k, v) : constrain(v, k.lo, k.hi);
}
static inline void up_picker_step(UpPicker &k, int d) {
  k.pos.target = k.wrap ? roundf(k.pos.target) + d : constrain(up_picker_value(k) + d, k.lo, k.hi);
}
#endif
