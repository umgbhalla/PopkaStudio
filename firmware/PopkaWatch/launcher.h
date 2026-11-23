// launcher.h — the Apps launcher (the round board's gallery screen, the one a swipe up on the face opens): pages of
// round icons, three by two, swiped left and right (upager.h; Codex's measured study, design/SPEC_launcher.md —
// 23 Nov 2026), and its Launcher settings (the last tile).
//
// The tiles are the project's: every visible card of the Apps screen (gallery.h's list on other screens), in the
// project's order or A–Z — its icon, its name, its colours (color — the icon, color2 — the disc behind it; the old
// neutral card grey gives a disc of the icon's own colour, deep), its link (a tap opens it the way a card's tap does:
// gl_req, face.h's gallery_tick). The last tile is always Launcher: its settings — the order, the names under the
// icons — apply at once and are kept in flash ("launcher"). Back from an app — the page it was opened from.
// A drag down on the launcher — the face (the top edge stays the quick settings').
// Round board only. Included from PopkaWatch.ino after upager.h.
#ifdef HALO_ROUND
extern const HaloFont gl_ln_ic38, gl_ln_lab, gl_ln_kick, gl_ln_row, gl_ln_opt, gl_ln_ic15;  // (studio.py GALLERY_FONTS)
#define LN_ACCENT 0xC9D0FF
#define LN_INK 0x20284D    // dark ink on the accent
#define LN_PANEL 0x161926
#define LN_CONTROL 0x272C45
#define LN_GREY 0x2E3034   // the cards' old neutral background (a project from before the launcher)
#define LN_PER 6           // tiles on a page
#define LN_MAX (UP_PAGES * LN_PER)
#define LN_IC_GRID "\xEF\x80\x89"   // Launcher's tile: table-cells-large
#define LN_IC_CHECK "\xEF\x80\x8C"
#define LN_IC_BACK "\xEF\x81\xA0"

static const FaceScreen *ln_scr;       // the Apps screen (its layers are the tiles)
static int8_t ln_tile[LN_MAX];    // each tile's layer (-1 — Launcher's own)
static int ln_n, ln_depth;
static bool ln_az, ln_labels = true, ln_want_set, ln_ready;
static UpState ln_st, ln_set_st;  // the grid's pager, the settings' (beside any app's: upager.h's swap)
static HaloSpring ln_sw;          // the labels' switch: 0 — off, 1 — on
static char ln_owner;             // (a drag down: to the face)
static bool ln_pull;
static Preferences ln_prefs;

static void ln_load() {
  if (ln_ready) return;
  ln_ready = true;
  ln_prefs.begin("launcher", false);
  ln_az = ln_prefs.getUChar("az", 0), ln_labels = ln_prefs.getUChar("labels", 1);
  halo_spring_set(ln_sw, ln_labels ? 1 : 0);
  // upager.h's audit of the launcher's page in front, of its settings (serial: lncheck, lnsetcheck)
  halo_command("lncheck", [](const char *) { up_swap(ln_st), up_audit(), up_swap(ln_st); });
  halo_command("lnsetcheck", [](const char *) { up_swap(ln_set_st), up_audit(), up_swap(ln_set_st); });
}
// The tiles from the screen's layers: its visible cards, in its order or by name; Launcher last
static void ln_collect() {
  ln_n = 0;
  if (!ln_scr) return;
  const FaceScreen &S = *ln_scr;
  for (int i = 0; i < S.n && ln_n < LN_MAX - 1; i++)
    if (S.layers[i].kind == FL_CARD && !S.layers[i].hidden) ln_tile[ln_n++] = i;
  if (ln_az)  // (a dozen or two: insertion)
    for (int a = 1; a < ln_n; a++)
      for (int b = a; b > 0 && strcasecmp(gl_str(S.layers[ln_tile[b - 1]].text), gl_str(S.layers[ln_tile[b]].text)) > 0; b--)
        std::swap(ln_tile[b - 1], ln_tile[b]);
  ln_tile[ln_n++] = -1;
}
// (one grid scrolled up and down, not pages: the user's call, 23 Nov 2026 — Wear OS's launcher scrolls)
static int ln_count() { return 1; }
static float ln_height(int) { return 179 + 129 * ((ln_n + 2) / 3 - 1) + 150; }
static inline float ln_tx(int k) { return 122 + 111 * (k % 3); }
static inline float ln_ty(int k) { return 179 + 129 * (k / 3); }

// Text with its own ink centred on y (the study measures every string's ink)
static void ln_text(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                    const HaloView &v, const HaloItem *on = &halo_black, float fit = 0) {
  HaloItem &t = up_item(P);
  ui_text(t, f, s, halo_text_mid(f, s), x, y, 1, align, c, a, on, fit);
  t.on_black = on == &halo_black;
  up_commit(t, v);
}

// ---------- The grid ----------
static void ln_lay(int p, UpPage &P, float a, const HaloView &v) {
  const FaceScreen &S = *ln_scr;
  const char *title = "Apps";
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_TITLE && !S.layers[i].hidden && S.layers[i].text && *S.layers[i].text) {
      title = S.layers[i].text;
      break;
    }
  up_title(P, title, a, v);
  for (int k = 0; k < ln_n; k++) {
    const int li = ln_tile[k];
    const FaceLayer *L = li >= 0 ? &S.layers[li] : nullptr;
    const float x = ln_tx(k), y = ln_ty(k);
    const uint32_t ink = L ? L->color : LN_ACCENT;
    const uint32_t disc = !L ? 0x242B48 : L->color2 == LN_GREY ? ui_mix(L->color, 0x000000, 0.79f) : L->color2;
    HaloItem &d = up_disc(P, x, y, 44, disc, a, v);
    const char *icon = L ? gl_str(L->icon) : LN_IC_GRID;
    ln_text(P, gl_ln_ic38, icon, x, y, 1, ink, *icon ? a : 0, v, &d);
    // the name: 100 px at most (its tap target's width), cut with an ellipsis past it
    ln_text(P, gl_ln_lab, L ? gl_str(L->text) : "Launcher", x, y + 60, 1, UP_WHITE, ln_labels ? a : 0, v, &halo_black, 100);
  }
}
static void ln_tap(int p, float x, float y) {
  for (int k = 0; k < ln_n; k++) {
    const float tx = ln_tx(k), ty = ln_ty(k);
    if (!up_in(x, y, tx - 50, ty - 44, tx + 50, ty + 69)) continue;
    const int li = ln_tile[k];
    if (li < 0) ln_want_set = true;  // (opened after the pager's state is put back)
    else gl_req = &ln_scr->layers[li], gl_req_depth = ln_depth;
    return;
  }
}
static const UpApp LN_APP = {ln_count, ln_lay, ln_tap, nullptr, nullptr, LN_ACCENT, ln_height, false};

// ---------- Launcher settings ----------
static int ln_set_count() { return 1; }
static void ln_set_lay(int, UpPage &P, float a, const HaloView &v) {
  up_title(P, "Launcher", a, v);
  up_secondary(P, 108, 102, 25, LN_IC_BACK, LN_CONTROL, LN_ACCENT, a, v, gl_back_ic);
  up_rrect(P, 73, 152, 393, 271, 28, LN_PANEL, a, v);
  HaloItem &order = P.it[P.n - 1];
  ln_text(P, gl_ln_kick, "APP ORDER", CX, 175, 1, UP_MUTED, a, v, &order);
  for (int k = 0; k < 2; k++) {
    const bool on = (k == 1) == ln_az;
    const float x = k ? 311 : 155;
    up_rrect(P, x - 70, 198, x + 70, 252, 27, on ? LN_ACCENT : LN_CONTROL, a, v);
    ln_text(P, gl_ln_opt, k ? "A\xE2\x80\x93Z" : "Project", x, 225, 1, on ? LN_INK : UP_WHITE, a, v, &P.it[P.n - 1]);
  }
  up_rrect(P, 73, 289, 393, 359, 28, LN_PANEL, a, v);
  HaloItem &row = P.it[P.n - 1];
  ln_text(P, gl_ln_row, "App labels", 96, 324, 0, UP_WHITE, a, v, &row);
  const float s = halo_clamp01(ln_sw.x);  // the switch: the track, the knob sliding 316 → 346, the check fading in
  up_rrect(P, 297, 305, 365, 343, 19, ui_mix(LN_CONTROL, LN_ACCENT, s), a, v);
  HaloItem &knob = up_disc(P, 316 + 30 * ln_sw.x, 324, 15, ui_mix(UP_MUTED, LN_INK, s), a, v);
  ln_text(P, gl_ln_ic15, LN_IC_CHECK, 316 + 30 * ln_sw.x, 324, 1, LN_ACCENT, a * s, v, &knob);
}
static void ln_set_tap(int, float x, float y) {
  if (up_in(x, y, 83, 77, 133, 127)) return halo_nav_back();
  if (up_in(x, y, 85, 198, 225, 252) && ln_az) ln_az = false, ln_prefs.putUChar("az", 0);
  else if (up_in(x, y, 241, 198, 381, 252) && !ln_az) ln_az = true, ln_prefs.putUChar("az", 1);
  else if (up_in(x, y, 73, 289, 393, 359)) ln_labels = !ln_labels, ln_prefs.putUChar("labels", ln_labels);
}
static void ln_set_upd(int, float dt) {
  ln_sw.target = ln_labels ? 1 : 0;
  static HaloSpringK K = halo_spring(0.25f, 0.85f);
  halo_spring_run(ln_sw, K, dt);
}
static const UpApp LN_SET_APP = {ln_set_count, ln_set_lay, ln_set_tap, ln_set_upd, nullptr, LN_ACCENT};
static void ln_set_update(float dt) { up_swap(ln_set_st), up_update(dt), up_swap(ln_set_st); }
static void ln_set_layout(const HaloView &v) { up_swap(ln_set_st), up_layout(v), up_swap(ln_set_st); }
static void ln_set_draw() { up_draw_of(ln_set_st); }
static void ln_set_open() {
  const int d = halo_nav_depth() + 1;
  if (d >= NAV_LEVELS) return;
  up_swap(ln_set_st), up_open(&LN_SET_APP), up_swap(ln_set_st);
  halo_spring_set(ln_sw, ln_labels ? 1 : 0);
  nav_scr[d] = -1, nav_stub[d] = "Launcher";
  halo_nav_open({nullptr, ln_set_update, ln_set_layout, ln_set_draw});
}

// ---------- The screen (gallery.h calls these for the Apps screen) ----------
static void ln_enter(const FaceScreen &S) {  // (from the face: the first page)
  ln_load();
  ln_scr = &S, ln_collect();
  up_swap(ln_st), up_open(&LN_APP), up_swap(ln_st);
}
static void ln_update(const FaceScreen &S, float dt, int depth) {
  HaloFinger &f = halo_finger;
  ln_load();
  ln_scr = &S, ln_depth = depth, ln_collect();
  if (!ln_st.app) up_swap(ln_st), up_open(&LN_APP), up_swap(ln_st);
  up_swap(ln_st), up_update(dt), up_swap(ln_st);
  if (ln_want_set) ln_want_set = false, ln_set_open();
  // A drag down (not from the top edge: the quick settings') — back to the face, as the old list's pull
  if (depth == 0 && f.drag_start && !f.horiz && f.y > f.y0 && f.edge != HALO_EDGE_TOP && halo_finger_claim(&ln_owner))
    ln_pull = true;
  if (ln_pull && !(f.down && f.owner == &ln_owner)) {
    ln_pull = false;
    if (f.y - f.y0 > 60 || f.vy > 500) halo_nav_to_face(true);
  }
}
static void ln_layout(const FaceScreen &S, float a, const HaloView &v) {
  ln_load();
  ln_scr = &S, ln_collect();
  if (!ln_st.app) up_swap(ln_st), up_open(&LN_APP), up_swap(ln_st);
  HaloView lv = v;
  lv.a *= a;
  up_swap(ln_st), up_layout(lv), up_swap(ln_st);
}
static void ln_draw() { up_draw_of(ln_st); }
#endif
