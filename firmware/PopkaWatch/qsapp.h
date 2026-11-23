// qsapp.h — the quick settings panel's pages (shade_round.h pulls the panel; upager.h pages it): Codex's measured study,
// design/SPEC_quick_settings.md — 23 Nov 2026.
//   1 Controls — four tiles: Wi-Fi (sync now), Brightness (2 → 5 → 8), Always On (on/off now: the tile turns mint), Volume
//     (mute, restore); the battery chip, the gear (to page 3);
//   2 Levels — Brightness 0 … 8 and Volume 0 … 10 with − and +, their marks; Lock, Screen off (the panel closes first);
//   3 Shade settings — which page a pull opens on (Controls or Levels), the battery chip on or off (both kept in flash,
//     "qs"); back to Controls.
// The tiles read the same brightness and speaker state as everything else (shade_round.h: sh_bright, vol_level).
// Round board only, with the panel. Included from PopkaWatch.ino after upager.h and launcher.h (its text helper).
#ifdef HALO_ROUND
extern const HaloFont gl_qs_val, gl_qs_lvl, gl_qs_head, gl_qs_ic24, gl_qs_ic23, gl_qs_ic19, gl_qs_ic25;  // (studio.py)
#define QS_MINT 0xB6EFDA
#define QS_INK 0x102D27     // dark ink on mint
#define QS_CARD 0x172D28
#define QS_TONAL 0x203B34   // tonal buttons, the battery chip
#define QS_MARK 0x365B50    // a level's mark not reached
#define QS_CHOICE 0x29483E  // a preference's other choice, the switch off
static const char *const QS_IC_TILE[] = {"\xEF\x87\xAB", "\xEF\x86\x85", "\xEF\x80\x97", "\xEF\x80\xA8"};  // wifi sun clock volume
#define QS_IC_MUTED "\xEF\x9A\xA9"
#define QS_IC_GEAR "\xEF\x80\x93"
#define QS_IC_MINUS "\xEF\x81\xA8"
#define QS_IC_PLUS "\xEF\x81\xA7"
#define QS_IC_LOCK "\xEF\x80\xA3"
#define QS_IC_POWER "\xEF\x80\x91"
#define QS_IC_BACK "\xEF\x81\xA0"
static const char *const QS_IC_BAT[] = {"\xEF\x89\x84", "\xEF\x89\x83", "\xEF\x89\x82", "\xEF\x89\x81", "\xEF\x89\x80"};

static UpState qs_st;  // the panel's pager (beside the app's under it: upager.h's swap)
static Preferences qs_prefs;
static bool qs_on_levels, qs_battery = true, qs_shown;
static HaloSpring qs_sw;  // the battery switch: 0 — off, 1 — on
static HaloSpringK K_QS_SW;
static char qs_bright_s[8], qs_vol_s[8], qs_bat_s[8], qs_lbright_s[8], qs_lvol_s[8];

static int qs_count() { return 3; }

// A tile of Controls: 152 × 106, its icon 28 px over the middle, its value, its label; on (k) — mint, less rounded
static void qs_tile(UpPage &P, float x, float y, int ic, const char *icon, const char *value, const char *label, float k,
                    bool dim, float a, const HaloView &v) {
  up_rrect(P, x - 76, y - 53, x + 76, y + 53, 38 + (29 - 38) * k, ui_mix(QS_CARD, QS_MINT, k), a, v);
  HaloItem &t = P.it[P.n - 1];
  const uint32_t acc = dim ? UP_MUTED : ui_mix(QS_MINT, QS_INK, k);
  ln_text(P, gl_qs_ic24, icon ? icon : QS_IC_TILE[ic], x, y - 28, 1, acc, a, v, &t);
  ln_text(P, gl_qs_val, value, x, y + 3, 1, ui_mix(UP_WHITE, QS_INK, k), a, v, &t);
  ln_text(P, gl_up_lab, label, x, y + 33, 1, acc, a, v, &t);
}
// A level: its card, − and +, the value, the marks (the first `n` of `cap` lit)
static void qs_level(UpPage &P, float y, const char *label, const char *value, int n, int cap, float a, const HaloView &v) {
  up_rrect(P, 65, y - 51, 401, y + 51, 36, QS_CARD, a, v);
  HaloItem &c = P.it[P.n - 1];
  ln_text(P, gl_up_lab, label, CX, y - 29, 1, QS_MINT, a, v, &c);
  ln_text(P, gl_qs_lvl, value, CX, y + 1, 1, UP_WHITE, a, v, &c);
  up_secondary(P, 108, y, 28, QS_IC_MINUS, QS_TONAL, QS_MINT, a, v, gl_qs_ic24);
  up_secondary(P, 358, y, 28, QS_IC_PLUS, QS_MINT, QS_INK, a, v, gl_qs_ic24);
  for (int i = 0; i < cap; i++) {
    const float x = CX - (cap - 1) * 7 + 14 * i;
    up_rrect(P, x - 5, y + 28, x + 5, y + 35, 3.5f, i < n ? QS_MINT : QS_MARK, a, v);
  }
}

static void qs_lay(int p, UpPage &P, float a, const HaloView &v) {
  const int b = sh_bright(), vol = vol_level;
  if (p == 0) {  // Controls
    up_title(P, "Quick Settings", a, v);
    snprintf(qs_bright_s, sizeof(qs_bright_s), "%d/8", b);
    snprintf(qs_vol_s, sizeof(qs_vol_s), "%d%%", vol * 10);
    qs_tile(P, 151, 185, 0, nullptr, sh_wifi ? "Sync" : "Off", "WI-FI", 0, !sh_wifi, a, v);
    qs_tile(P, 315, 185, 1, nullptr, qs_bright_s, "BRIGHTNESS", 0, false, a, v);
    qs_tile(P, 151, 299, 2, nullptr, shade_aod_is_on() ? "On" : "Off", "ALWAYS ON", halo_clamp01(sh_on[SI_AOD].x), false, a, v);
    qs_tile(P, 315, 299, 3, vol ? nullptr : QS_IC_MUTED, vol ? qs_vol_s : "Muted", "VOLUME", 0, false, a, v);
    const bool bat = qs_battery && sh_bat >= 0;  // (the chip: only with a reading, and when the setting keeps it)
    const int pct = constrain((int)sh_bat, 0, 100);
    snprintf(qs_bat_s, sizeof(qs_bat_s), "%d%%", pct);
    up_rrect(P, 129, 371, 267, 407, 18, QS_TONAL, bat ? a : 0, v);
    HaloItem &chip = P.it[P.n - 1];
    const int lv = pct >= 88 ? 4 : pct >= 63 ? 3 : pct >= 38 ? 2 : pct >= 13 ? 1 : 0;
    ln_text(P, gl_qs_ic19, QS_IC_BAT[lv], 181, 389, 1, QS_MINT, bat ? a : 0, v, &chip);
    // the percent from where the study's "78%" (centred at 216) starts: a wider one grows to the right, off the icon
    static const float left = 216 - halo_text_width(gl_up_lab, "78%") / 2.0f;
    ln_text(P, gl_up_lab, qs_bat_s, left, 389, 0, UP_WHITE, bat ? a : 0, v, &chip);
    up_secondary(P, 310, 389, 25, QS_IC_GEAR, QS_CARD, QS_MINT, a, v, gl_qs_ic24);
  } else if (p == 1) {  // Levels
    up_title(P, "Levels", a, v);
    snprintf(qs_lbright_s, sizeof(qs_lbright_s), "%d/8", b);
    snprintf(qs_lvol_s, sizeof(qs_lvol_s), "%d%%", vol * 10);
    qs_level(P, 181, "BRIGHTNESS", qs_lbright_s, b, 8, a, v);
    qs_level(P, 292, "VOLUME", qs_lvol_s, vol, 10, a, v);
    up_secondary(P, 170, 377, 26, QS_IC_LOCK, QS_TONAL, QS_MINT, a, v, gl_qs_ic23);
    up_secondary(P, 296, 377, 26, QS_IC_POWER, QS_TONAL, QS_MINT, a, v, gl_qs_ic23);
    ln_text(P, gl_up_lab, "LOCK", 170, 412, 1, UP_MUTED, a, v);
    ln_text(P, gl_up_lab, "SCREEN OFF", 296, 412, 1, UP_MUTED, a, v);
  } else {  // Shade settings
    up_title(P, "Shade settings", a, v);
    up_rrect(P, 72, 132, 394, 246, 34, QS_CARD, a, v);
    HaloItem &open = P.it[P.n - 1];
    ln_text(P, gl_qs_head, "Open on", CX, 156, 1, UP_WHITE, a, v, &open);
    for (int k = 0; k < 2; k++) {
      const bool on = (k == 1) == qs_on_levels;
      const float x = k ? 310 : 156;
      up_rrect(P, x - 71, 181, x + 71, 232, 25.5f, on ? QS_MINT : QS_CHOICE, a, v);
      ln_text(P, gl_up_lab, k ? "Levels" : "Controls", x, 206.5f, 1, on ? QS_INK : UP_WHITE, a, v, &P.it[P.n - 1]);
    }
    up_rrect(P, 72, 261, 394, 345, 34, QS_CARD, a, v);
    HaloItem &card = P.it[P.n - 1];
    ln_text(P, gl_qs_head, "Battery", 190, 293, 1, UP_WHITE, a, v, &card);
    ln_text(P, gl_up_lab, "SHOW ON CONTROLS", 190, 322, 1, QS_MINT, a, v, &card);
    const float s = halo_clamp01(qs_sw.x), kx = 325.5f + 25.5f * qs_sw.x;  // the thumb: 325.5 off, 351 on
    up_rrect(P, 306, 284, 371, 323, 19.5f, ui_mix(QS_CHOICE, QS_MINT, s), a, v);
    HaloItem &thumb = up_disc(P, kx, 303.5f, 14.5f, ui_mix(UP_MUTED, QS_INK, s), a, v);
    ln_text(P, gl_ln_ic15, LN_IC_CHECK, kx, 303.5f, 1, QS_MINT, a * s, v, &thumb);
    up_secondary(P, CX, 386, 29, QS_IC_BACK, QS_TONAL, QS_MINT, a, v, gl_qs_ic25);
  }
}

static void qs_tap(int p, float x, float y) {
  if (p == 0) {
    if (up_in(x, y, 75, 132, 227, 238)) sh_act(SI_WIFI);
    else if (up_in(x, y, 239, 132, 391, 238)) sh_act(SI_BRIGHT);
    else if (up_in(x, y, 75, 246, 227, 352)) sh_act(SI_AOD);
    else if (up_in(x, y, 239, 246, 391, 352)) sh_act(SI_VOL);
    else if (up_in(x, y, 285, 364, 335, 414)) up_go(2);
  } else if (p == 1) {
    for (int k = 0; k < 2; k++) {
      const float y0 = k ? 264 : 153;
      const int d = up_in(x, y, 80, y0, 136, y0 + 56) ? -1 : up_in(x, y, 330, y0, 386, y0 + 56) ? 1 : 0;
      if (!d) continue;
      if (k) sh_volume(vol_level + d);
      else sh_bright_set(constrain(sh_bright() + d, 0, SH_SEGS));
    }
    if (up_in(x, y, 144, 351, 196, 403)) sh_act(SI_LOCK);
    else if (up_in(x, y, 270, 351, 322, 403)) sh_act(SI_OFF);
  } else {
    if (up_in(x, y, 85, 181, 227, 232) && qs_on_levels) qs_on_levels = false, qs_prefs.putUChar("open", 0);
    else if (up_in(x, y, 239, 181, 381, 232) && !qs_on_levels) qs_on_levels = true, qs_prefs.putUChar("open", 1);
    else if (up_in(x, y, 72, 261, 394, 345)) qs_battery = !qs_battery, qs_prefs.putUChar("bat", qs_battery);
    else if (up_in(x, y, 204, 357, 262, 415)) up_go(0);
  }
}
static void qs_upd(int, float dt) {
  qs_sw.target = qs_battery ? 1 : 0;
  halo_spring_run(qs_sw, K_QS_SW, dt);
}
static const UpApp QS_APP = {qs_count, qs_lay, nullptr, qs_upd, nullptr, QS_MINT};

// ---------- shade_round.h calls these ----------
static void qs_begin() {  // (face_build: the two settings back from flash)
  qs_prefs.begin("qs", false);
  qs_on_levels = qs_prefs.getUChar("open", 0), qs_battery = qs_prefs.getUChar("bat", 1);
  K_QS_SW = halo_spring(0.25f, 0.85f);
  halo_spring_set(qs_sw, qs_battery ? 1 : 0);
  halo_command("qscheck", [](const char *) { up_swap(qs_st), up_audit(), up_swap(qs_st); });  // (upager.h's audit)
}
static void qs_open_page() {  // a new pull: the page the setting picks
  up_swap(qs_st), up_open(&QS_APP, qs_on_levels ? 1 : 0), up_swap(qs_st);
}
// The panel's finger while it is out: sideways — the pages; a tap (open, the page at rest) — the page's
static void qs_tick(float dt, bool tap, float x, float y) {
  if (!qs_st.app) qs_open_page();
  up_swap(qs_st);
  up_update(dt);
  if (tap && fabsf(up_pos.x - up_page) < 0.05f) qs_tap(up_page, x, y);
  up_swap(qs_st);
}
// The pages on the disc (ty: where the disc is), or hidden once when it is gone
static void qs_layout(const HaloView &v, float ty, bool shown) {
  if (!shown && !qs_shown) return;
  if (!qs_st.app) qs_open_page();
  HaloView q = v;
  q.y += ty * v.s;
  if (!shown) q.a = 0;
  qs_shown = shown;
  up_swap(qs_st), up_layout(q), up_swap(qs_st);
}
static void qs_draw() {
  if (qs_shown) up_draw_of(qs_st);
}
#endif
