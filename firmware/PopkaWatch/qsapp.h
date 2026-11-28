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

// A round toggle of Controls (Pixel Watch's quick settings: icon buttons, no tiles): on — filled mint, the dark ink;
// off — the tonal fill, the icon in mint
#define QS_BTN_R 44.0f
static const float QS_BX[3] = {125, 233, 341}, QS_BY[2] = {214, 318};
static void qs_btn(UpPage &P, int k, const HaloFont &f, const char *icon, float on, float a, const HaloView &v) {
  const float x = QS_BX[k % 3], y = QS_BY[k / 3];
  const bool press = up_pressed(v, x - QS_BTN_R, y - QS_BTN_R, x + QS_BTN_R, y + QS_BTN_R);  // (pressed: lighter, a touch smaller)
  HaloItem &d = up_disc(P, x, y, QS_BTN_R * (press ? 0.92f : 1), ui_mix(ui_mix(QS_CARD, QS_MINT, on), 0xFFFFFF, press ? 0.12f : 0), a, v);
  ln_text(P, f, icon, x, y, 1, ui_mix(QS_MINT, QS_INK, on), a, v, &d);
}
// A level as Wear OS's slider: a full pill, − and + at its ends, the icon and value between them over its marks
static void qs_level(UpPage &P, float y, const char *icon, const char *value, int n, int cap, float a, const HaloView &v) {
  up_rrect(P, UP_ROW_X0, y - 48, UP_ROW_X1, y + 48, 48, QS_CARD, a, v);
  HaloItem &c = P.it[P.n - 1];
  // the icon and the value as one group, centred: the icon (its 24 px glyph), 10 px, the value
  const float vw = halo_text_width(gl_qs_lvl, value), gx = CX - (26 + 10 + vw) / 2;
  ln_text(P, gl_qs_ic24, icon, gx + 13, y - 8, 1, QS_MINT, a, v, &c);
  ln_text(P, gl_qs_lvl, value, gx + 36, y - 8, 0, UP_WHITE, a, v, &c);
  up_secondary(P, UP_ROW_X0 + 46, y, 30, QS_IC_MINUS, QS_TONAL, QS_MINT, a, v, gl_qs_ic24);
  up_secondary(P, UP_ROW_X1 - 46, y, 30, QS_IC_PLUS, QS_MINT, QS_INK, a, v, gl_qs_ic24);
  for (int i = 0; i < cap; i++) {
    const float x = CX - (cap - 1) * 7 + 14 * i;
    up_rrect(P, x - 5, y + 24, x + 5, y + 31, 3.5f, i < n ? QS_MINT : QS_MARK, a, v);
  }
}

static void qs_lay(int p, UpPage &P, float a, const HaloView &v) {
  const int b = sh_bright(), vol = vol_level;
  if (p == 0) {  // Controls: the battery, then two rows of three round toggles
    const bool bat = qs_battery && sh_bat >= 0;  // (only with a reading, and when the setting keeps it)
    const int pct = constrain((int)sh_bat, 0, 100);
    // the battery as the face's pill (widgets.h FL_BATPILL): one battery look on the watch
    snprintf(qs_bat_s, sizeof(qs_bat_s), "%d", pct);
    const float bx = CX - 4, by = 122, bw = 92, bh = 36, in = 4, fw = (bw - 2 * in) * pct / 100.0f, ba = bat ? a : 0;
    const uint32_t fill = sh_chg ? 0x6DD58C : pct <= 15 ? 0xF28B82 : QS_MINT;
    up_rrect(P, bx - bw / 2, by - bh / 2, bx + bw / 2, by + bh / 2, bh / 2, QS_TONAL, ba, v);
    up_rrect(P, bx - bw / 2 + in, by - bh / 2 + in, bx - bw / 2 + in + fmaxf(fw, bh - 2 * in), by + bh / 2 - in, bh / 2 - in,
             ui_mix(fill, QS_TONAL, 0.68f), pct > 0 ? ba : 0, v);
    up_rrect(P, bx + bw / 2 + 2, by - 7, bx + bw / 2 + 8, by + 7, 3, QS_TONAL, ba, v);  // (the nub)
    const float tw = halo_text_width(gl_up_lab, qs_bat_s), gx = bx + (sh_chg ? 9 : 0);
    ln_text(P, gl_qs_ic19, "\xEF\x83\xA7", gx - tw / 2 - 12, by, 1, UP_WHITE, sh_chg ? ba : 0, v, nullptr);
    ln_text(P, gl_up_lab, qs_bat_s, gx - tw / 2, by, 0, pct <= 15 && !sh_chg ? 0xF28B82 : UP_WHITE, ba, v, nullptr);
    qs_btn(P, 0, gl_qs_ic24, QS_IC_TILE[0], sh_wifi ? 1 : 0, a, v);                           // Wi-Fi: sync now
    qs_btn(P, 1, gl_qs_ic24, QS_IC_TILE[2], halo_clamp01(sh_on[SI_AOD].x), a, v);             // Always On
    qs_btn(P, 2, gl_qs_ic24, vol ? QS_IC_TILE[3] : QS_IC_MUTED, vol ? 1 : 0, a, v);           // sound: mute, restore
    qs_btn(P, 3, gl_qs_ic24, QS_IC_TILE[1], b >= 5 ? 1 : 0, a, v);                            // brightness: 2 → 5 → 8
    qs_btn(P, 4, gl_qs_ic23, QS_IC_LOCK, 0, a, v);                                            // lock
    qs_btn(P, 5, gl_qs_ic24, QS_IC_GEAR, 0, a, v);                                            // the panel's settings
  } else if (p == 1) {  // Levels: two sliders, screen off under them
    snprintf(qs_lbright_s, sizeof(qs_lbright_s), "%d/8", b);
    snprintf(qs_lvol_s, sizeof(qs_lvol_s), "%d%%", vol * 10);
    qs_level(P, 160, QS_IC_TILE[1], qs_lbright_s, b, 8, a, v);
    qs_level(P, 272, vol ? QS_IC_TILE[3] : QS_IC_MUTED, qs_lvol_s, vol, 10, a, v);
    up_secondary(P, CX, 368, 28, QS_IC_POWER, QS_TONAL, QS_MINT, a, v, gl_qs_ic23);
  }  // (2, Shade settings: a list — qs_lay_item)
}
// Shade settings as Wear OS's settings list (upager.h list mode: the panel's pager has lists of its own)
static int qs_items(int p) { return p == 2 ? 4 : 0; }
static float qs_item_h(int, int k) { return up_set_h(k); }
static void qs_lay_item(int, int k, UpPage &P, float, float ac, const HaloView &v) {
  if (k == 0) return up_set_title(P, "Shade", ac, v);
  if (k == 1) up_set_row(P, "Controls", "Open on this page", UP_ROW_RADIO, qs_on_levels ? 0 : 1, QS_MINT, QS_INK, ac, v);
  else if (k == 2) up_set_row(P, "Levels", "Open on this page", UP_ROW_RADIO, qs_on_levels ? 1 : 0, QS_MINT, QS_INK, ac, v);
  else up_set_row(P, "Battery", "Show on Controls", UP_ROW_SWITCH, qs_sw.x, QS_MINT, QS_INK, ac, v);
}

static void qs_tap(int p, float x, float y) {
  if (p == 0) {
    static const uint8_t ACT[5] = {SI_WIFI, SI_AOD, SI_VOL, SI_BRIGHT, SI_LOCK};
    for (int k = 0; k < 6; k++)
      if (up_hit(x, y, QS_BX[k % 3], QS_BY[k / 3], QS_BTN_R)) return k < 5 ? sh_act(ACT[k]) : up_go(2);
  } else if (p == 1) {
    for (int k = 0; k < 2; k++) {
      const float cy = k ? 272 : 160;
      const int d = up_hit(x, y, UP_ROW_X0 + 46, cy, 30) ? -1 : up_hit(x, y, UP_ROW_X1 - 46, cy, 30) ? 1 : 0;
      if (!d) continue;
      if (k) sh_volume(vol_level + d);
      else sh_bright_set(constrain(sh_bright() + d, 0, SH_SEGS));
    }
    if (up_hit(x, y, CX, 368, 28)) sh_act(SI_OFF);
  } else {
    const int k = up_list_hit(2, y);
    if (k == 1 && qs_on_levels) qs_on_levels = false, qs_prefs.putUChar("open", 0);
    else if (k == 2 && !qs_on_levels) qs_on_levels = true, qs_prefs.putUChar("open", 1);
    else if (k == 3) qs_battery = !qs_battery, qs_prefs.putUChar("bat", qs_battery);
  }
}
static void qs_upd(int, float dt) {
  qs_sw.target = qs_battery ? 1 : 0;
  halo_spring_run(qs_sw, K_QS_SW, dt);
}
static const UpApp QS_APP = {qs_count, qs_lay, nullptr, qs_upd, nullptr, QS_MINT, nullptr, false, qs_items, qs_item_h, qs_lay_item};  // (taps: shade_round.h hands them to qs_tap)

// ---------- shade_round.h calls these ----------
static void qs_begin() {  // (face_build: the two settings back from flash)
  qs_prefs.begin("qs", false);
  qs_on_levels = qs_prefs.getUChar("open", 0), qs_battery = qs_prefs.getUChar("bat", 1);
  K_QS_SW = halo_spring(0.25f, 0.85f);
  halo_spring_set(qs_sw, qs_battery ? 1 : 0);
  halo_command("qscheck", [](const char *) { up_with(qs_st, up_audit); });  // (upager.h's audit)
}
static void qs_open_page() {  // a new pull: the page the setting picks
  up_with(qs_st, [] { up_open(&QS_APP, qs_on_levels ? 1 : 0); });
}
// The panel's finger while it is out: sideways — the pages; a tap (open, the page at rest) — the page's
static void qs_tick(float dt, bool tap, float x, float y) {
  if (!qs_st.app) qs_open_page();
  up_with(qs_st, [&] {
    up_update(dt);
    if (tap && fabsf(up_pos.x - up_page) < 0.05f) qs_tap(up_page, x, y);
  });
}
// The pages on the disc (ty: where the disc is), or hidden once when it is gone
static void qs_layout(const HaloView &v, float ty, bool shown) {
  if (!shown && !qs_shown) return;
  if (!qs_st.app) qs_open_page();
  HaloView q = v;
  q.y += ty * v.s;
  if (!shown) q.a = 0;
  qs_shown = shown;
  up_with(qs_st, [&] { up_layout(q); });
}
static void qs_draw() {
  if (qs_shown) up_draw_of(qs_st);
}
#endif
