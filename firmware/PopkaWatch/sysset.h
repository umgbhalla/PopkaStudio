// sysset.h — Settings (FL_SETTINGS): the watch's settings as seven non-scrolling pages swiped left and right (upager.h;
// Codex's measured study, design/SPEC_settings.md — 23 Nov 2026):
//   1 Display (brightness, Always On, Wake options ›)   2 Faces (the default face, swipe to switch)
//   3 Buttons (BOOT ›, PWR ›)   4 Sound (the volume, mute)   5 Wi-Fi (the network, how it went, sync now)
//   6 Battery (the level, the charge limit)   7 System (24-hour time, App settings ›, restart, power off)
// and three panels opened over them on the navigation stack (back — the arrow or the swipe from the left edge): Wake
// options (tap, raise, the screen timeout), BOOT / PWR action (one of five), Settings app (open to Display or the last
// page; wrap the pages round). A long press on a page opens its full list on the UI kit (ui.h) — what the page has no
// room for: Always On brightness, burn-in shift, the battery's voltage and history, the firmware, memory, the time zone.
// Every change is the watch's at once and kept in flash (Preferences "sys": what was not kept before — Always On and its
// timing, how it wakes, 12/24 hours, the volume, how Settings opens); sysset_begin brings them back at start. Brightness
// keeps its own key (settings.h, Display), as the charge limit does.
// Included from PopkaWatch.ino after power.h (its settings are power.h's); round board only.
#ifdef HALO_ROUND
#include "ui.h"
#include "upager.h"  // the paged app screens (Settings is one; Weather and the clocks too)
#include <Preferences.h>

static Preferences sy_prefs;
static bool sy_ready;
static void sy_put(const char *key, int v) {
  if (sy_ready) sy_prefs.putInt(key, v);
}

// ---------- The settings: get, set (applied and kept) ----------
// Settings' own (its App settings panel): open on Display or on the page seen last; wrap round past the ends
static int sg_open_last, sg_wrap, sg_last;  // (sg_last: a page's category, SG_* below)
static int sy_open_last() { return sg_open_last; }
static void sy_set_open_last(int v) { sg_open_last = v ? 1 : 0, sy_put("sgopen", sg_open_last); }
static int sy_wrap() { return sg_wrap; }
static void sy_set_wrap(int v) { sg_wrap = v ? 1 : 0, sy_put("sgwrap", sg_wrap); }
static int sy_aod() { return pw_aod_on; }
static void sy_set_aod(int v) { pw_aod_on = v ? 1 : 0, sy_put("aod", pw_aod_on); }
static const char *const SS_IDLE_NAMES[] = {"10 seconds", "15 seconds", "30 seconds", "1 minute"};
static const int SS_IDLE_S[] = {10, 15, 30, 60};
static int sy_idle() {
  for (int i = 0; i < 4; i++) if (pw_idle_s <= SS_IDLE_S[i]) return i;
  return 3;
}
static void sy_set_idle(int i) { pw_idle_s = SS_IDLE_S[constrain(i, 0, 3)], sy_put("idle", pw_idle_s); }
static const char *const SS_DIM_NAMES[] = {"Low", "Medium", "High"};
static const int SS_DIM[] = {12, 24, 48};
static int sy_dim() { return pw_aod_level <= 16 ? 0 : pw_aod_level <= 32 ? 1 : 2; }
static void sy_set_dim(int i) { pw_aod_level = SS_DIM[constrain(i, 0, 2)], sy_put("aodb", pw_aod_level); }
static int sy_tap() { return pw_wake_tap; }
static void sy_set_tap(int v) { pw_wake_tap = v ? 1 : 0, sy_put("wtap", pw_wake_tap); }
static int sy_raise() { return pw_wake_raise; }
static void sy_set_raise(int v) { pw_wake_raise = v ? 1 : 0, sy_put("wraise", pw_wake_raise); }
static int sy_shift() { return pw_aod_shift; }
static void sy_set_shift(int v) { pw_aod_shift = v ? 1 : 0, sy_put("shift", pw_aod_shift); }
static int sy_h24() { return fc_h24; }
static void sy_set_h24(int v) {
  fc_h24 = v != 0, sy_put("h24", fc_h24);
  fc_min = fc_hour = -1, fc_check = true;  // (the clock again, in its new form, on the next tick)
}
// Watch faces: which one is the default, switching by a swipe at all
static const char *sy_face_names[FACE_MAX];
static int sy_face() { return fc_face; }
static void sy_set_face(int i) {
  if (i < 0 || i >= face_n || i == fc_face) return;
  fc_face = i, halo_spring_set(fs_off, 0), face_switch_save(), halo_damage_all();
}
static int sy_fswipe() { return fs_on; }
static void sy_set_fswipe(int v) { fs_on = v != 0, sy_put("fswipe", fs_on); }
// The buttons: what a press does (power.h's FN_*: nothing, screen off, lock, Always On, home)
static const char *const SY_BTN_NAMES[] = {"Nothing", "Screen off", "Lock", "Always On", "Home", "Back"};
static int sy_boot() { return constrain(pw_boot, 0, 5); }
static void sy_set_boot(int v) { pw_boot = constrain(v, 0, 5), sy_put("boot", pw_boot); }
static int sy_pwr() { return constrain(pw_pwr, 0, 5); }
static void sy_set_pwr(int v) { pw_pwr = constrain(v, 0, 5), sy_put("pwr", pw_pwr); }
// Weather: the units (the next fetch asks in them — fetched now, with Wi-Fi)
static const char *const SY_UNIT_NAMES[] = {"Celsius", "Fahrenheit"};
static int sy_units() { return wx_fahr ? 1 : 0; }
static void sy_set_units(int v);
// The charge limit (settings.h keeps it: 80 … 95 %)
static const char *const SY_LIMIT_NAMES[] = {"80%", "85%", "90%", "95%"};
static int sy_limit() { return constrain((charge_limit_selected() - 80) / 5, 0, 3); }
static void sy_set_limit(int i) { __atomic_store_n(&bc_selected, 80 + 5 * constrain(i, 0, 3), __ATOMIC_RELAXED); }
// A screen of the project with a layer of this kind, opened over this page (the Battery's own screen: its history)
static int sy_screen_of(uint8_t kind) {
  for (int s = 1; s < face_scr_n; s++)
    for (int i = 0; i < face_scr[s].n; i++)
      if (face_scr[s].layers[i].kind == kind) return s;
  return -1;
}
static void sy_open_screen(int s) {
  if (s < 0) return;
  const int d = min(halo_nav_depth() + 1, NAV_LEVELS - 1);
  nav_scr[d] = s, nav_stub[d] = face_scr[s].name;
  halo_nav_open(level(d, 0));
}
static void sy_battery_details() { sy_open_screen(sy_screen_of(FL_BATTERY)); }
#if HAS_DISPLAY
static int sy_bright() { return dp_value; }
static void sy_set_bright(int v) { dp_set(v); }  // (settings.h keeps it)
#endif
#define SS_WIFI (FN_WIFI && !defined(__EMSCRIPTEN__))  // (the page's engine: the computer's clock, no Wi-Fi of its own)
#define SS_SOUND (HAS_MUSIC || HAS_RECORDER || HAS_VIDEO || HAS_RADIO || (HAS_POMODORO && !defined(HALO_NO_SPEAKER)))
#if SS_SOUND
static int sy_vol() { return vol_level; }
static void sy_set_vol(int v) { vol_set(v, false), sy_put("vol", vol_level); }
#endif

// ---------- What the rows say ----------
static void sy_zone(char *o, int n) { strlcpy(o, TZ, n); }
#if SS_WIFI
static const char *sy_wifi_state() {
  static const char *const S[4] = {"Waiting", "Connected · time synced", "Network not found", "No internet"};
  return S[hclk_state & 3];
}
static void sy_wifi_net(char *o, int n) { strlcpy(o, *hclk_ssid ? hclk_ssid : "Not set", n); }
static void sy_wifi_status(char *o, int n) { strlcpy(o, sy_wifi_state(), n); }
static void sy_wifi_saved(char *o, int n) { snprintf(o, n, "%d network%s", hclk_nets, hclk_nets == 1 ? "" : "s"); }
static void sy_wifi_sync() { xTaskNotifyGive(hclk_handle); }
#endif
static void sy_bat_level(char *o, int n) {
  const HaloBattery b = halo_battery();
  if (b.ok && b.percent >= 0) snprintf(o, n, "%d%%", b.percent);
  else strlcpy(o, "—", n);
}
static void sy_bat_volt(char *o, int n) {
  const HaloBattery b = halo_battery();
  snprintf(o, n, "%.2f V", b.mv / 1000.0f);
}
static void sy_bat_power(char *o, int n) {
  const HaloBattery b = halo_battery();
  strlcpy(o, !b.usb ? "On battery" : b.charging ? "USB · charging" : "USB · not charging", n);
}
static void sy_fw(char *o, int n) {
#ifdef POPKA_PROJECT
  snprintf(o, n, "PopkaWatch · %s", POPKA_PROJECT);
#else
  strlcpy(o, "PopkaWatch", n);
#endif
}
static void sy_mem(char *o, int n) {
#ifdef __EMSCRIPTEN__
  strlcpy(o, "—", n);
#else
  snprintf(o, n, "%u KB free · PSRAM %u KB", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
#endif
}
static void sy_restart() {
  ui_confirm_open({UI_IC_RESET, "Restart?", "The watch starts again", 0xA8C7FA, [] {
#ifndef __EMSCRIPTEN__
                     esp_restart();
#endif
                   }});
}

// ---------- The pages ----------
#define SS_IC_CONTRAST "\xEF\x81\x82"
#define SS_IC_CLOCK "\xEF\x80\x97"
#define SS_IC_WIFI "\xEF\x87\xAB"
#define SS_IC_VOLUME "\xEF\x80\xA8"
#define SS_IC_BATTERY "\xEF\x89\x80"
#define SS_IC_WRENCH "\xEF\x82\xAD"
#define SS_IC_SUN "\xEF\x86\x85"
#define SS_IC_BOLT "\xEF\x83\xA7"
#define SS_IC_LOCATION "\xEF\x8F\x85"
static UiPage sy_display, sy_wifi, sy_sound, sy_battery, sy_system, sy_faces, sy_buttons;  // (the long-press lists)
#define SY_IC_IMAGE "\xEF\x80\xBE"
#define SY_IC_GAMEPAD "\xEF\x84\x9B"
#define SY_IC_CLOUD "\xEF\x83\x82"
#define SY_IC_LOCK "\xEF\x80\xA3"
#define SY_IC_HOUSE "\xEF\x80\x95"
static void sy_set_units(int v) {
  wx_fahr = v != 0, sy_put("fahr", wx_fahr);
#if SS_WIFI
  xTaskNotifyGive(hclk_handle);  // (fresh weather in the new units)
#endif
}
static void sy_add(UiPage &p, const UiRow &r) {
  if (p.n < UI_ROWS) p.rows[p.n++] = r;
}
static void sy_build() {
  sy_display = {"Display", {}, 0, 0xFFD27A};
#if HAS_DISPLAY
  sy_add(sy_display, ur_slider("Brightness", 0, DP_STEPS, sy_bright, sy_set_bright));
#endif
  sy_add(sy_display, ur_head("Always On"));
  sy_add(sy_display, ur_toggle(SS_IC_SUN, "Always On", sy_aod, sy_set_aod));
  static const char *const *idle = SS_IDLE_NAMES;
  sy_add(sy_display, ur_choice(SS_IC_CLOCK, "Screen timeout", idle, 4, sy_idle, sy_set_idle));
  sy_add(sy_display, ur_choice(SS_IC_CONTRAST, "Always On brightness", SS_DIM_NAMES, 3, sy_dim, sy_set_dim));
  sy_add(sy_display, ur_toggle(SS_IC_CONTRAST, "Burn-in shift", sy_shift, sy_set_shift));
  sy_add(sy_display, ur_head("Wake up"));
  sy_add(sy_display, ur_toggle(SS_IC_SUN, "Tap to wake", sy_tap, sy_set_tap));
  sy_add(sy_display, ur_toggle(SS_IC_BOLT, "Raise to wake", sy_raise, sy_set_raise));

#if SS_WIFI
  sy_wifi = {"Wi-Fi", {}, 0, 0xA8C7FA};
  sy_add(sy_wifi, ur_info(SS_IC_WIFI, "Network", sy_wifi_net));
  sy_add(sy_wifi, ur_info(SS_IC_CLOCK, "Status", sy_wifi_status));
  sy_add(sy_wifi, ur_info(SS_IC_WRENCH, "Remembered", sy_wifi_saved));
  sy_wifi.edge = ur_action("Sync now", sy_wifi_sync);
#endif

#if SS_SOUND
  sy_sound = {"Sound", {}, 0, 0xF6B5EA};
  sy_add(sy_sound, ur_slider("Volume", 0, 10, sy_vol, sy_set_vol));
#endif

  sy_battery = {"Battery", {}, 0, 0xA4F9B5};
  sy_add(sy_battery, ur_info(SS_IC_BATTERY, "Level", sy_bat_level));
  sy_add(sy_battery, ur_info(SS_IC_BOLT, "Power", sy_bat_power));
  sy_add(sy_battery, ur_info(SS_IC_BATTERY, "Voltage", sy_bat_volt));
  sy_add(sy_battery, ur_choice(SS_IC_BOLT, "Charge limit", SY_LIMIT_NAMES, 4, sy_limit, sy_set_limit));
  if (sy_screen_of(FL_BATTERY) >= 0) {
    UiRow r = ur_nav(SS_IC_BATTERY, "History", nullptr, nullptr);
    r.act = sy_battery_details;
    sy_add(sy_battery, r);
  }

  sy_faces = {"Watch faces", {}, 0, 0xFFB68A};
  for (int i = 0; i < face_n; i++) sy_face_names[i] = face_scr[face_ids[i]].name ? face_scr[face_ids[i]].name : "Watch face";
  sy_add(sy_faces, ur_choice(SY_IC_IMAGE, "Watch face", sy_face_names, face_n, sy_face, sy_set_face));
  if (face_n > 1) sy_add(sy_faces, ur_toggle(SY_IC_IMAGE, "Swipe to switch", sy_fswipe, sy_set_fswipe));

  sy_buttons = {"Buttons", {}, 0, 0xC7BFFF};
  sy_add(sy_buttons, ur_choice(SY_IC_LOCK, "BOOT button", SY_BTN_NAMES, 6, sy_boot, sy_set_boot));
  sy_add(sy_buttons, ur_choice(SY_IC_HOUSE, "PWR button", SY_BTN_NAMES, 6, sy_pwr, sy_set_pwr));

  sy_system = {"System", {}, 0, 0xC7BFFF};
  sy_add(sy_system, ur_toggle(SS_IC_CLOCK, "24-hour clock", sy_h24, sy_set_h24));
  sy_add(sy_system, ur_info(SS_IC_LOCATION, "Time zone", sy_zone));
  sy_add(sy_system, ur_choice(SY_IC_CLOUD, "Weather units", SY_UNIT_NAMES, 2, sy_units, sy_set_units));
  sy_add(sy_system, ur_info(SS_IC_WRENCH, "Firmware", sy_fw));
  sy_add(sy_system, ur_info(SS_IC_WRENCH, "Memory", sy_mem));
  sy_add(sy_system, ur_action("Restart", sy_restart, 0xA8C7FA));
  sy_add(sy_system, ur_action("Power off", power_confirm_open, 0xFFB4AB));
}

// At start (setup, after power_build): what was kept
static void sysset_begin() {
  sy_prefs.begin("sys", false);
  sy_ready = true;
  if (sy_prefs.isKey("aod")) pw_aod_on = sy_prefs.getInt("aod", pw_aod_on);
  if (sy_prefs.isKey("idle")) pw_idle_s = sy_prefs.getInt("idle", pw_idle_s);
  if (sy_prefs.isKey("aodb")) pw_aod_level = sy_prefs.getInt("aodb", pw_aod_level);
  if (sy_prefs.isKey("wtap")) pw_wake_tap = sy_prefs.getInt("wtap", pw_wake_tap);
  if (sy_prefs.isKey("wraise")) pw_wake_raise = sy_prefs.getInt("wraise", pw_wake_raise);
  if (sy_prefs.isKey("shift")) pw_aod_shift = sy_prefs.getInt("shift", pw_aod_shift);
  if (sy_prefs.isKey("h24")) fc_h24 = sy_prefs.getInt("h24", fc_h24) != 0;
  if (sy_prefs.isKey("fswipe")) fs_on = sy_prefs.getInt("fswipe", 1) != 0;
  if (sy_prefs.isKey("boot")) pw_boot = constrain(sy_prefs.getInt("boot", pw_boot), 0, 5);
  if (sy_prefs.isKey("pwr")) pw_pwr = constrain(sy_prefs.getInt("pwr", pw_pwr), 0, 5);
  if (sy_prefs.isKey("fahr")) wx_fahr = sy_prefs.getInt("fahr", wx_fahr) != 0;
  sg_open_last = sy_prefs.getInt("sgopen", 0) != 0, sg_wrap = sy_prefs.getInt("sgwrap", 0) != 0;
  sg_last = sy_prefs.getInt("sglast", 0);
  // set — the settings by name from the serial port, through the same setters (applied and kept): set pwr 4, set aod 0;
  // set alone lists them (scripts and tests set the watch up without tapping through it)
  halo_command("set", [](const char *a) {
    char k[12] = {};
    int v = 0;
    const int got = sscanf(a, "%11s %d", k, &v);
    struct S { const char *name; int (*get)(); void (*set)(int); } const S[] = {
        {"aod", sy_aod, sy_set_aod}, {"idle", sy_idle, sy_set_idle}, {"aodb", sy_dim, sy_set_dim},
        {"wtap", sy_tap, sy_set_tap}, {"wraise", sy_raise, sy_set_raise}, {"shift", sy_shift, sy_set_shift},
        {"h24", sy_h24, sy_set_h24}, {"fswipe", sy_fswipe, sy_set_fswipe}, {"face", sy_face, sy_set_face},
        {"boot", sy_boot, sy_set_boot}, {"pwr", sy_pwr, sy_set_pwr}, {"fahr", sy_units, sy_set_units},
        {"limit", sy_limit, sy_set_limit},
        {"sgopen", sy_open_last, sy_set_open_last}, {"sgwrap", sy_wrap, sy_set_wrap},
#if HAS_DISPLAY
        {"bright", sy_bright, sy_set_bright},
#endif
#if SS_SOUND
        {"vol", sy_vol, sy_set_vol},
#endif
    };
    for (const auto &e : S) {
      if (got >= 1 && strcmp(k, e.name)) continue;
      if (got == 2) e.set(v);
      LOG("set %s %d\n", e.name, e.get());
    }
  });
#if SS_SOUND
  if (sy_prefs.isKey("vol")) vol_level = constrain(sy_prefs.getInt("vol", vol_level), 0, 10);
#endif
  sy_build();
}

// ---------- The pages (upager.h; Codex's measured study, design/SPEC_settings.md — 23 Nov 2026) ----------
// Seven categories, each in its own accent and a tonal surface of it; three panels over them. Coordinates are the
// dial's pixels, as the study gives them; text is placed by its ink (its letters' box), centred unless a left edge is given
extern const HaloFont gl_sg_digit, gl_sg_row, gl_sg_ic15, gl_sg_ic20, gl_sg_ic21, gl_sg_ic22, gl_sg_ic23, gl_sg_ic24,
    gl_sg_ic25, gl_sg_ic26, gl_sg_ic28, gl_sg_ic29, gl_sg_ic66;  // (studio.py GALLERY_FONTS)
enum : uint8_t { SG_DISPLAY, SG_FACES, SG_BUTTONS, SG_SOUND, SG_WIFI, SG_BATTERY, SG_SYSTEM, SG_N };
static const char *const SG_NAME[SG_N] = {"Display", "Faces", "Buttons", "Sound", "Wi-Fi", "Battery", "System"};
static const uint32_t SG_ACC[SG_N] = {0xFFD27A, 0xFFB68A, 0xC9D0FF, 0xF6B5EA, 0xA8C7FA, 0xA4F9B5, 0xC9D0FF};
static const uint32_t SG_TONE[SG_N] = {0x352917, 0x37251F, 0x282B49, 0x382438, 0x202F49, 0x1D352C, 0x282B49};
#define SG_INK 0x181724        // dark ink on a bright control
#define SG_RAIL 0x525367       // a level's dots past the value
#define SG_OFF 0x555063        // a switch's rail, off
#define SG_DANGER 0xFFC0C5     // Power off: its glyph and label
#define SG_DANGER_BG 0x432429  // … its disc
#define SG_FACE_BG 0x1D1716    // the face preview's disc
#define SG_BAT_TRACK 0x243C33  // the battery's arc, empty
#define SG_IC_MINUS "\xEF\x81\xA8"
#define SG_IC_PLUS "\xEF\x81\xA7"
#define SG_IC_BACK "\xEF\x81\xA0"
#define SG_IC_LEFT "\xEF\x81\x93"
#define SG_IC_RIGHT "\xEF\x81\x94"
#define SG_IC_CHECK "\xEF\x80\x8C"
#define SG_IC_SYNC "\xEF\x80\xA1"
#define SG_IC_SUN "\xEF\x86\x85"
#define SG_IC_EYE "\xEF\x81\xAE"
#define SG_IC_LOCK "\xEF\x80\xA3"
#define SG_IC_POWER "\xEF\x80\x91"
#define SG_IC_MUTE "\xEF\x9A\xA9"
#define SG_IC_VOLUME "\xEF\x80\xA8"
#define SG_IC_WIFI "\xEF\x87\xAB"
#define SG_IC_HOME "\xEF\x80\x95"
#define SG_IC_MOON "\xEF\x86\x86"
#define SG_IC_BANG "\xEF\x84\xAA"
#define SG_IC_CLOCK "\xEF\x80\x97"
#define SG_IC_RESTART "\xEF\x8B\xAA"
#define SG_IC_SLIDERS "\xEF\x87\x9E"
#define SG_DEG(d) ((d) * HALO_PI / 180)
// The page shows Wi-Fi with Wi-Fi in the firmware; the browser's engine has none of its own: there, the page with no network
#if SS_WIFI || defined(__EMSCRIPTEN__)
#define SG_HAS_WIFI 1
#else
#define SG_HAS_WIFI 0
#endif

static uint8_t sg_cat[SG_N];  // the pages: a category each, in order (Sound and Wi-Fi only with them)
static int sg_n;
static int sg_child;          // the panel open over the pages (SGC_*)
enum : uint8_t { SGC_NONE, SGC_WAKE, SGC_BOOT, SGC_PWR, SGC_PREFS };
static int sg_last_vol = 5;   // the level Mute gives back
static char sg_val[SG_N][8], sg_of[SG_N][8], sg_date[12], sg_net[33], sg_stat[24], sg_pct[8], sg_lim[8], sg_timeout[8];
static UpApp SG_APP;

static void sg_cats() {
  sg_n = 0;
  sg_cat[sg_n++] = SG_DISPLAY, sg_cat[sg_n++] = SG_FACES, sg_cat[sg_n++] = SG_BUTTONS;
#if SS_SOUND
  sg_cat[sg_n++] = SG_SOUND;
#endif
#if SG_HAS_WIFI
  sg_cat[sg_n++] = SG_WIFI;
#endif
  sg_cat[sg_n++] = SG_BATTERY, sg_cat[sg_n++] = SG_SYSTEM;
}
static int sg_page_of(int cat) {  // (a category's page; Display's if this firmware has none)
  for (int p = 0; p < sg_n; p++)
    if (sg_cat[p] == cat) return p;
  return 0;
}

// ---------- The parts: text by its ink, a toggle row, a level, a card with an icon and a label ----------
// Where a string's letters reach left and right of its pen, and how high they are (from the line's top)
static void sg_ink(const HaloFont &f, const char *s, float *l, float *r, float *t = nullptr, float *b = nullptr) {
  int pen = 0, L = 9999, R = -9999, T = 9999, B = -9999;
  for (const char *q = s; *q;) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(q));
    if (!g) continue;
    if (g->w > 1 || g->h > 1) L = min(L, pen + g->ox), R = max(R, pen + g->ox + g->w), T = min(T, (int)g->oy), B = max(B, g->oy + g->h);
    pen += g->adv;
  }
  if (L > R) L = R = 0, T = B = 0;
  *l = L, *r = R;
  if (t) *t = T, *b = B;
}
// A string with its ink's middle at y and its ink's left (align 0), middle (1) or right (2) at x
static HaloItem &sg_text(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                         const HaloView &v, const HaloItem *on = &halo_black) {
  float l, r;
  sg_ink(f, s, &l, &r);
  const float pen = x - l - (align == 1 ? (r - l) / 2 : align == 2 ? r - l : 0);
  up_text(P, f, s, halo_text_mid(f, s), pen, y, 0, c, a, v, on);
  return P.it[min(P.n, UP_IT) - 1];
}
static HaloItem &sg_card(UpPage &P, float x0, float y0, float x1, float y1, float r, uint32_t c, float a, const HaloView &v) {
  up_rrect(P, x0, y0, x1, y1, r, c, a, v);
  return P.it[min(P.n, UP_IT) - 1];
}
// A round button: a disc (fill) and its glyph's ink centred on it
static void sg_round(UpPage &P, float x, float y, float r, const HaloFont &f, const char *icon, uint32_t fill, uint32_t ink,
                     float a, const HaloView &v, float ia = 1) {
  HaloItem &d = up_disc(P, x, y, r, fill, a, v);
  sg_text(P, f, icon, x, y, 1, ink, a * ia, v, &d);
}
static inline bool sg_in_disc(float x, float y, float cx, float cy, float r) { return up_in(x, y, cx - r, cy - r, cx + r, cy + r); }
// A switch row: [82, y − 34] … [384, y + 34], the label left at 103, the switch's rail 305 … 371, its knob r 12
static void sg_toggle(UpPage &P, const char *label, float y, bool on, uint32_t acc, uint32_t tone, float a, const HaloView &v) {
  HaloItem &row = sg_card(P, 82, y - 34, 384, y + 34, 24, tone, a, v);
  sg_text(P, gl_sg_row, label, 103, y, 0, UP_WHITE, a, v, &row);
  sg_card(P, 305, y - 16, 371, y + 16, 16, on ? acc : SG_OFF, a, v);
  up_disc(P, on ? 355 : 321, y, 12, on ? SG_INK : UP_MUTED, a, v);
}
static inline bool sg_toggle_hit(float x, float y, float ry) { return up_in(x, y, 82, ry - 34, 384, ry + 34); }
// A level (brightness, volume): − and + at 108 and 358, the value big at 203, "OF max" under it, a dot a step at 299
static void sg_level(UpPage &P, int cat, int val, int max, bool live, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[cat], tone = SG_TONE[cat];
  sg_round(P, 108, 217, 36, gl_sg_ic29, SG_IC_MINUS, tone, acc, a, v, live && val > 0 ? 1 : 0.38f);
  sg_round(P, 358, 217, 36, gl_sg_ic29, SG_IC_PLUS, tone, acc, a, v, live && val < max ? 1 : 0.38f);
  snprintf(sg_val[cat], sizeof(sg_val[cat]), "%d", val), snprintf(sg_of[cat], sizeof(sg_of[cat]), "OF %d", max);
  sg_text(P, gl_sg_digit, sg_val[cat], CX, 203, 1, UP_WHITE, a, v);
  sg_text(P, gl_up_lab, sg_of[cat], CX, 274, 1, UP_MUTED, a, v);
  for (int i = 0; i <= max; i++) up_disc(P, 157 + 152.0f * i / max, 299, i == val ? 4.5f : 3.5f, i <= val ? acc : SG_RAIL, a, v);
}
static int sg_level_tap(float x, float y) {  // −1, +1 or 0
  return sg_in_disc(x, y, 108, 217, 36) ? -1 : sg_in_disc(x, y, 358, 217, 36) ? 1 : 0;
}
// A card with an icon (a 22 px slot) and a label, the two centred as one: 12 px between the slot and the label's ink
static void sg_pair(UpPage &P, float x0, float y0, float x1, float y1, const HaloFont &icf, const char *icon,
                    const char *label, uint32_t fill, uint32_t ink, float a, const HaloView &v) {
  HaloItem &card = sg_card(P, x0, y0, x1, y1, 23, fill, a, v);
  float l, r;
  sg_ink(gl_sg_row, label, &l, &r);
  const float y = (y0 + y1) / 2, L = (x0 + x1) / 2 - (34 + r - l) / 2;
  sg_text(P, icf, icon, L + 11, y, 1, ink, a, v, &card);
  sg_text(P, gl_sg_row, label, L + 34, y, 0, ink, a, v, &card);
}
// A text that may be long (a network's name, a face's): by its ink if it fits in w, else cut with an ellipsis
static void sg_fit(UpPage &P, const HaloFont &f, const char *s, float y, float w, uint32_t c, float a, const HaloView &v) {
  float l, r;
  sg_ink(f, s, &l, &r);
  if (r - l <= w) return (void)sg_text(P, f, s, CX, y, 1, c, a, v);
  up_text(P, f, s, halo_text_mid(f, s), CX, y, 1, c, a, v);
  P.it[min(P.n, UP_IT) - 1].fit = w;
}

// ---------- The seven pages ----------
static void sg_lay_display(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_DISPLAY], tone = SG_TONE[SG_DISPLAY];
#if HAS_DISPLAY
  sg_level(P, SG_DISPLAY, sy_bright(), DP_STEPS, true, a, v);
#else
  sg_level(P, SG_DISPLAY, 0, 8, false, a, v);
#endif
  const bool aod = sy_aod();
  for (int k = 0; k < 2; k++) {  // Always On (filled while on), Wake options (opens its panel)
    const float x = k ? 314 : 152;
    const bool on = !k && aod;
    const uint32_t ink = on ? SG_INK : acc;
    HaloItem &card = sg_card(P, x - 69, 331, x + 69, 390, 22, on ? acc : tone, a, v);
    sg_text(P, gl_sg_ic21, k ? SG_IC_EYE : SG_IC_SUN, x, 347, 1, ink, a, v, &card);
    sg_text(P, gl_up_lab, k ? "Wake options" : "Always On", x, 373, 1, ink, a, v, &card);
  }
}
static void sg_lay_faces(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_FACES], tone = SG_TONE[SG_FACES];
  const bool many = face_n > 1;
  HaloItem &disc = up_disc(P, CX, 208, 77, SG_FACE_BG, a, v);
  const float spans[3][2] = {{0, 360}, {34, 146}, {214, 326}};  // the rim of the preview, then its two accent arcs
  for (int k = 0; k < 3; k++) {
    up_arc(P, k ? 65 : 77, k ? 4 : 3, SG_DEG(spans[k][0]), SG_DEG(spans[k][1]), k ? acc : tone, a, v);
    HaloItem &arc = P.it[min(P.n, UP_IT) - 1];
    arc.y = 208, halo_view_commit(arc, v);  // (up_arc centres on the dial: moved to the preview's centre)
  }
  sg_text(P, gl_up_title, fc_time, CX, 204, 1, UP_WHITE, a, v, &disc);
  const time_t now = time(nullptr);
  struct tm m;
  localtime_r(&now, &m);
  static const char *const WD[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  snprintf(sg_date, sizeof(sg_date), "%s %d", WD[m.tm_wday], m.tm_mday);
  sg_text(P, gl_up_lab, sg_date, CX, 243, 1, UP_MUTED, a, v, &disc);
  sg_round(P, 110, 219, 32, gl_sg_ic25, SG_IC_LEFT, tone, acc, a, v, many ? 1 : 0.38f);
  sg_round(P, 356, 219, 32, gl_sg_ic25, SG_IC_RIGHT, tone, acc, a, v, many ? 1 : 0.38f);
  const char *name = sy_face_names[fc_face] ? sy_face_names[fc_face] : "Watch face";
  sg_fit(P, gl_up_title, name, 306, 300, UP_WHITE, a, v);
  sg_toggle(P, "Face swipe", 360, many && sy_fswipe(), acc, tone, many ? a : a * 0.5f, v);
}
static void sg_lay_buttons(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_BUTTONS], tone = SG_TONE[SG_BUTTONS];
  for (int k = 0; k < 2; k++) {  // BOOT, PWR: what a press does
    const float y = k ? 285 : 184;
    HaloItem &card = sg_card(P, 77, y - 43, 389, y + 43, 29, tone, a, v);
    sg_round(P, 129, y, 27, gl_sg_ic25, k ? SG_IC_POWER : SG_IC_LOCK, acc, SG_INK, a, v);
    sg_text(P, gl_up_lab, k ? "PWR" : "BOOT", 171, y - 16, 0, acc, a, v, &card);
    // the action: in the title cut; one too wide for the 162 px before the chevron (Screen off) — in the condensed row cut
    const char *act = SY_BTN_NAMES[k ? sy_pwr() : sy_boot()];
    float l, r;
    sg_ink(gl_up_title, act, &l, &r);
    sg_text(P, r - l > 162 ? gl_sg_row : gl_up_title, act, 171, y + 12, 0, UP_WHITE, a, v, &card);
    sg_text(P, gl_sg_ic22, SG_IC_RIGHT, 353, y, 1, acc, a, v, &card);
  }
}
static void sg_lay_sound(UpPage &P, float a, const HaloView &v) {
#if SS_SOUND
  const uint32_t acc = SG_ACC[SG_SOUND], tone = SG_TONE[SG_SOUND];
  sg_level(P, SG_SOUND, sy_vol(), 10, true, a, v);
  const bool muted = sy_vol() == 0;  // (muted: the card filled, it gives the level back)
  sg_pair(P, 133, 332, 333, 390, gl_sg_ic22, muted ? SG_IC_VOLUME : SG_IC_MUTE, muted ? "Unmute" : "Mute",
          muted ? acc : tone, muted ? SG_INK : acc, a, v);
#endif
}
static void sg_lay_wifi(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_WIFI], tone = SG_TONE[SG_WIFI];
  sg_round(P, CX, 195, 60, gl_sg_ic66, SG_IC_WIFI, tone, acc, a, v);
#if SS_WIFI
  static const char *const S[4] = {"WAITING", "CONNECTED", "NETWORK NOT FOUND", "NO INTERNET"};
  strlcpy(sg_net, *hclk_ssid ? hclk_ssid : "Not set", sizeof(sg_net)), strlcpy(sg_stat, S[hclk_state & 3], sizeof(sg_stat));
  const float sa = a;
#else
  strlcpy(sg_net, "No network", sizeof(sg_net)), strlcpy(sg_stat, "NO WI-FI IN THE PAGE", sizeof(sg_stat));
  const float sa = a * 0.5f;  // (no sync to ask for)
#endif
  sg_fit(P, gl_up_title, sg_net, 278, 300, UP_WHITE, a, v);
  sg_text(P, gl_up_lab, sg_stat, CX, 313, 1, acc, a, v);
  sg_pair(P, 138, 338, 328, 396, gl_sg_ic22, SG_IC_SYNC, "Sync now", acc, SG_INK, sa, v);
}
static void sg_lay_battery(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_BATTERY], tone = SG_TONE[SG_BATTERY];
  const HaloBattery b = halo_battery();
  const int pct = b.ok && b.percent >= 0 ? b.percent : -1;
  // the arc: two 130° spans round the clock's and the dots' gaps, filled in order for 260° × the level
  const float fill = 260 * halo_clamp01(pct / 100.0f);
  for (int k = 0; k < 2; k++) up_arc(P, 213, 7, SG_DEG(k ? 200 : 30), SG_DEG(k ? 330 : 160), SG_BAT_TRACK, a, v);
  up_arc(P, 213, 7, SG_DEG(30), SG_DEG(30 + fminf(fill, 130)), acc, a, v);
  up_arc(P, 213, 7, SG_DEG(200), SG_DEG(200 + fmaxf(0, fill - 130)), acc, a, v);
  // the level: its digits white, the accent % 7 px after them, bottoms level, the two centred as one at y 195
  if (pct >= 0) snprintf(sg_pct, sizeof(sg_pct), "%d", pct);
  else strlcpy(sg_pct, "--", sizeof(sg_pct));
  float dl, dr, dt, db, pl, pr, pt, pb;
  sg_ink(gl_sg_digit, sg_pct, &dl, &dr, &dt, &db), sg_ink(gl_up_title, "%", &pl, &pr, &pt, &pb);
  const float left = CX - (dr - dl + 7 + pr - pl) / 2, bottom = 195 + (db - dt) / 2;
  sg_text(P, gl_sg_digit, sg_pct, left, 195, 0, UP_WHITE, a, v);
  sg_text(P, gl_up_title, "%", left + dr - dl + 7, bottom - (pb - pt) / 2, 0, acc, a, v);
  sg_text(P, gl_up_lab, !b.ok ? "NO DATA" : !b.usb ? "ON BATTERY" : b.charging ? "CHARGING" : "ON USB", CX, 267, 1, acc, a, v);
  // the charge limit: − and + on a card, 80 … 95 %
  HaloItem &card = sg_card(P, 86, 310, 380, 382, 27, tone, a, v);
  const int lim = sy_limit();
  sg_round(P, 123, 346, 26, gl_sg_ic22, SG_IC_MINUS, acc, SG_INK, a, v, lim > 0 ? 1 : 0.38f);
  sg_round(P, 343, 346, 26, gl_sg_ic22, SG_IC_PLUS, acc, SG_INK, a, v, lim < 3 ? 1 : 0.38f);
  strlcpy(sg_lim, SY_LIMIT_NAMES[lim], sizeof(sg_lim));
  sg_text(P, gl_up_title, sg_lim, CX, 333, 1, UP_WHITE, a, v, &card);
  sg_text(P, gl_up_lab, "CHARGE LIMIT", CX, 365, 1, UP_MUTED, a, v, &card);
}
static void sg_lay_system(UpPage &P, float a, const HaloView &v) {
  const uint32_t acc = SG_ACC[SG_SYSTEM], tone = SG_TONE[SG_SYSTEM];
  sg_toggle(P, "24-hour time", 171, sy_h24(), acc, tone, a, v);
  HaloItem &card = sg_card(P, 82, 219, 384, 287, 24, tone, a, v);
  sg_text(P, gl_sg_ic26, SG_IC_SLIDERS, 117, 253, 1, acc, a, v, &card);
  sg_text(P, gl_sg_row, "App settings", 143, 253, 0, UP_WHITE, a, v, &card);
  sg_text(P, gl_sg_ic20, SG_IC_RIGHT, 350, 253, 1, acc, a, v, &card);
  sg_round(P, 161, 345, 34, gl_sg_ic29, SG_IC_RESTART, tone, acc, a, v);
  sg_round(P, 305, 345, 34, gl_sg_ic28, SG_IC_POWER, SG_DANGER_BG, SG_DANGER, a, v);
  sg_text(P, gl_up_lab, "Restart", 161, 398, 1, acc, a, v);
  sg_text(P, gl_up_lab, "Power off", 305, 398, 1, SG_DANGER, a, v);
}

static int sg_count() { return sg_n; }
static void sg_lay(int p, UpPage &P, float a, const HaloView &v) {
  const int cat = sg_cat[constrain(p, 0, sg_n - 1)];
  up_title(P, SG_NAME[cat], a, v);
  switch (cat) {
    case SG_DISPLAY: return sg_lay_display(P, a, v);
    case SG_FACES: return sg_lay_faces(P, a, v);
    case SG_BUTTONS: return sg_lay_buttons(P, a, v);
    case SG_SOUND: return sg_lay_sound(P, a, v);
    case SG_WIFI: return sg_lay_wifi(P, a, v);
    case SG_BATTERY: return sg_lay_battery(P, a, v);
    default: return sg_lay_system(P, a, v);
  }
}
static void sg_child_open(int which);
static void sg_tap(int p, float x, float y) {
  switch (sg_cat[constrain(p, 0, sg_n - 1)]) {
    case SG_DISPLAY:
#if HAS_DISPLAY
      if (const int d = sg_level_tap(x, y)) sy_set_bright(constrain(sy_bright() + d, 0, DP_STEPS));
#endif
      if (up_in(x, y, 83, 331, 221, 390)) sy_set_aod(!sy_aod());
      else if (up_in(x, y, 245, 331, 383, 390)) sg_child_open(SGC_WAKE);
      break;
    case SG_FACES:
      if (face_n > 1 && sg_in_disc(x, y, 110, 219, 32)) sy_set_face((fc_face + face_n - 1) % face_n);
      else if (face_n > 1 && sg_in_disc(x, y, 356, 219, 32)) sy_set_face((fc_face + 1) % face_n);
      else if (face_n > 1 && sg_toggle_hit(x, y, 360)) sy_set_fswipe(!sy_fswipe());
      break;
    case SG_BUTTONS:
      if (up_in(x, y, 77, 141, 389, 227)) sg_child_open(SGC_BOOT);
      else if (up_in(x, y, 77, 242, 389, 328)) sg_child_open(SGC_PWR);
      break;
#if SS_SOUND
    case SG_SOUND:
      if (const int d = sg_level_tap(x, y)) sy_set_vol(constrain(sy_vol() + d, 0, 10));
      else if (up_in(x, y, 133, 332, 333, 390)) {
        if (sy_vol() > 0) sg_last_vol = sy_vol(), sy_set_vol(0);
        else sy_set_vol(max(1, sg_last_vol));
      }
      break;
#endif
#if SS_WIFI
    case SG_WIFI:
      if (up_in(x, y, 138, 338, 328, 396)) sy_wifi_sync();
      break;
#endif
    case SG_BATTERY:
      if (sg_in_disc(x, y, 123, 346, 26)) sy_set_limit(max(0, sy_limit() - 1));
      else if (sg_in_disc(x, y, 343, 346, 26)) sy_set_limit(min(3, sy_limit() + 1));
      break;
    case SG_SYSTEM:
      if (sg_toggle_hit(x, y, 171)) sy_set_h24(!sy_h24());
      else if (up_in(x, y, 82, 219, 384, 287)) sg_child_open(SGC_PREFS);
      else if (sg_in_disc(x, y, 161, 345, 34)) sy_restart();
      else if (sg_in_disc(x, y, 305, 345, 34)) power_confirm_open();
      break;
  }
}
// A long press on a page: its full list on the UI kit (what the page has no room for)
static void sg_more(int cat) {
  const UiPage *pg = cat == SG_DISPLAY ? &sy_display : cat == SG_FACES ? &sy_faces : cat == SG_BUTTONS ? &sy_buttons
                   : cat == SG_BATTERY ? &sy_battery : cat == SG_SYSTEM ? &sy_system : nullptr;
#if SS_SOUND
  if (cat == SG_SOUND) pg = &sy_sound;
#endif
#if SS_WIFI
  if (cat == SG_WIFI) pg = &sy_wifi;
#endif
  if (pg) ui_cur = 0, ui_open(pg);
}
static void sg_update(int p, float dt) {
  const HaloFinger &f = halo_finger;
  // wrap: let go past an end (its rubber) — the page at the other end comes in from that side
  static bool was_drag;
  if (was_drag && !up_drag && sg_wrap && sg_n > 1) {
    if (up_page == 0 && up_pos.x < -0.04f) up_page = sg_n - 1, up_pos.x = sg_n - 0.5f, up_pos.v = 0, up_pos.target = up_page;
    else if (up_page == sg_n - 1 && up_pos.x > sg_n - 1 + 0.04f) up_page = 0, up_pos.x = -0.5f, up_pos.v = 0, up_pos.target = 0;
  }
  was_drag = up_drag;
  // the page seen last (Open to: Last category), kept once the page has settled
  if (!up_drag && fabsf(up_pos.x - up_page) < 0.02f && sg_cat[up_page] != sg_last) sg_last = sg_cat[up_page], sy_put("sglast", sg_last);
  // a long press (held still 0.6 s on nothing that took it): the page's full list
  static bool held;
  if (!f.down) held = false;
  else if (!held && !f.dragging && !f.owner && halo_ms() - f.t0 > 600 && !up_drag) held = true, sg_more(sg_cat[p]);
  (void)dt;
}

// ---------- The panels over the pages (Wake options, BOOT / PWR action, Settings app) ----------
// One page each, laid as the pages are (the same parts), on the navigation stack: back — its arrow or the left edge
static UpPage *sg_cp;  // (PSRAM)
static HaloItem sg_cclock;
static HaloMask sg_cclock_mask;
static char sg_cclock_of[8];
static int sg_child_cat() { return sg_child == SGC_WAKE ? SG_DISPLAY : sg_child == SGC_PREFS ? SG_SYSTEM : SG_BUTTONS; }
static const char *sg_child_title() {
  return sg_child == SGC_WAKE ? "Wake options" : sg_child == SGC_BOOT ? "BOOT action" : sg_child == SGC_PWR ? "PWR action" : "Settings app";
}
static const float SG_ACT_XY[6][2] = {{151, 162}, {315, 162}, {151, 239}, {315, 239}, {151, 316}, {315, 316}};
static const char *const SG_ACT_IC[6] = {SG_IC_BANG, SG_IC_MOON, SG_IC_LOCK, SG_IC_SUN, SG_IC_HOME, SG_IC_BACK};
static void sg_child_lay(UpPage &P, float a, const HaloView &v) {
  const int cat = sg_child_cat();
  const uint32_t acc = SG_ACC[cat], tone = SG_TONE[cat];
  up_title(P, sg_child_title(), a, v);
  if (sg_child == SGC_WAKE) {
    sg_toggle(P, "Tap to wake", 157, sy_tap(), acc, tone, a, v);
    sg_toggle(P, "Raise to wake", 236, sy_raise(), acc, tone, a, v);
    const int i = sy_idle();
    sg_round(P, 109, 325, 30, gl_sg_ic24, SG_IC_MINUS, tone, acc, a, v, i > 0 ? 1 : 0.38f);
    sg_round(P, 357, 325, 30, gl_sg_ic24, SG_IC_PLUS, tone, acc, a, v, i < 3 ? 1 : 0.38f);
    snprintf(sg_timeout, sizeof(sg_timeout), "%d s", SS_IDLE_S[i]);
    sg_text(P, gl_up_title, sg_timeout, CX, 313, 1, UP_WHITE, a, v);
    sg_text(P, gl_up_lab, "SCREEN TIMEOUT", CX, 343, 1, UP_MUTED, a, v);
  } else if (sg_child == SGC_BOOT || sg_child == SGC_PWR) {
    const int cur = sg_child == SGC_BOOT ? sy_boot() : sy_pwr();
    for (int k = 0; k < 6; k++) {
      const float x = SG_ACT_XY[k][0], y = SG_ACT_XY[k][1];
      const bool on = k == cur;
      const uint32_t ink = on ? SG_INK : acc;
      HaloItem &card = sg_card(P, x - 72, y - 33, x + 72, y + 33, 24, on ? acc : tone, a, v);
      sg_text(P, gl_sg_ic24, SG_ACT_IC[k], x, y - 10, 1, ink, a, v, &card);
      sg_text(P, gl_up_lab, SY_BTN_NAMES[k], x, y + 16, 1, ink, a, v, &card);
      if (on) sg_text(P, gl_sg_ic15, SG_IC_CHECK, x + 48, y - 13, 1, SG_INK, a, v, &card);
    }
  } else {
    sg_text(P, gl_up_lab, "OPEN SETTINGS TO", CX, 139, 1, UP_MUTED, a, v);
    for (int k = 0; k < 2; k++) {
      const float x = k ? 315 : 151;
      const bool on = k == sy_open_last();
      const uint32_t ink = on ? SG_INK : acc;
      HaloItem &card = sg_card(P, x - 71, 160, x + 71, 245, 24, on ? acc : tone, a, v);
      sg_text(P, gl_sg_ic28, k ? SG_IC_CLOCK : SG_IC_SUN, x, 189, 1, ink, a, v, &card);
      sg_text(P, gl_up_lab, k ? "Last category" : "Display", x, 222, 1, ink, a, v, &card);
      if (on) sg_text(P, gl_sg_ic15, SG_IC_CHECK, x + 48, 179, 1, SG_INK, a, v, &card);
    }
    sg_toggle(P, "Wrap pages", 308, sy_wrap(), acc, tone, a, v);
  }
  sg_round(P, CX, 405, 26, gl_sg_ic23, SG_IC_BACK, tone, acc, a, v);
}
static void sg_child_tap(float x, float y) {
  if (sg_in_disc(x, y, CX, 405, 26)) return halo_nav_back();
  if (sg_child == SGC_WAKE) {
    if (sg_toggle_hit(x, y, 157)) sy_set_tap(!sy_tap());
    else if (sg_toggle_hit(x, y, 236)) sy_set_raise(!sy_raise());
    else if (sg_in_disc(x, y, 109, 325, 30)) sy_set_idle(max(0, sy_idle() - 1));
    else if (sg_in_disc(x, y, 357, 325, 30)) sy_set_idle(min(3, sy_idle() + 1));
  } else if (sg_child == SGC_BOOT || sg_child == SGC_PWR) {
    for (int k = 0; k < 6; k++)
      if (up_in(x, y, SG_ACT_XY[k][0] - 72, SG_ACT_XY[k][1] - 33, SG_ACT_XY[k][0] + 72, SG_ACT_XY[k][1] + 33)) {
        (sg_child == SGC_BOOT ? sy_set_boot : sy_set_pwr)(k);
        return halo_nav_back();  // (picked: back to Buttons, which shows it)
      }
  } else {
    if (up_in(x, y, 80, 160, 222, 245)) sy_set_open_last(0);
    else if (up_in(x, y, 244, 160, 386, 245)) sy_set_open_last(1);
    else if (sg_toggle_hit(x, y, 308)) sy_set_wrap(!sy_wrap());
  }
}
static void sg_c_update(float) {
  up_over = sg_cp;  // (upcheck audits the panel while it is on top)
  const HaloFinger &f = halo_finger;
  if (f.tap && !f.owner) sg_child_tap(f.x, f.y);
}
static void sg_c_layout(const HaloView &v) {
  if (!sg_cp) return;
  UpPage &P = *sg_cp;
  const int was = P.n;
  P.n = 0, P.nb = 0;
  sg_child_lay(P, 1, v);
  for (int i = P.n; i < was; i++) P.it[i].alpha = 0, halo_view_commit(P.it[i], v);  // (fewer than last frame)
  // the time along the top, as the pages have it (upager.h: 35 px on a 197 px arc, its ink centred at 233, 38)
  if (strcmp(sg_cclock_of, fc_time)) {
    float x, y;
    halo_text_arc(sg_cclock_mask, gl_up_clock, fc_time, halo_text_mid(gl_up_clock, "0123456789"), 197, 0, false, 1, &x, &y);
    strlcpy(sg_cclock_of, fc_time, sizeof(sg_cclock_of));
  }
  HaloItem &c = sg_cclock;
  c.kind = HI_MASK, c.mask = &sg_cclock_mask, c.color = UP_MUTED, c.on_black = true, c.on = &halo_black, c.scale = 1;
  c.x = CX, c.y = 38, c.alpha = sg_cclock_mask.bits ? 1 : 0;
  halo_view_commit(c, v);
}
static void sg_c_draw() {
  if (!sg_cp) return;
  for (int i = 0; i < sg_cp->n; i++) halo_item_draw(sg_cp->it[i]);
  halo_item_draw(sg_cclock);
}
static void sg_child_open(int which) {
  if (!sg_cp) {
#ifdef __EMSCRIPTEN__
    sg_cp = (UpPage *)calloc(1, sizeof(UpPage));
#else
    sg_cp = (UpPage *)heap_caps_calloc(1, sizeof(UpPage), MALLOC_CAP_SPIRAM);
#endif
    if (!sg_cp) return;
  }
  sg_child = which;
  const int d = min(halo_nav_depth() + 1, NAV_LEVELS - 1);
  nav_scr[d] = -1, nav_stub[d] = sg_child_title();
  static const HaloScreen S = {nullptr, sg_c_update, sg_c_layout, sg_c_draw};
  halo_nav_open(S);
}

// The screen (settings.h calls these): the pages, opened on Display or on the page seen last
static void sysset_enter() {
  if (!sg_n) sg_cats();
  SG_APP = {sg_count, sg_lay, sg_tap, sg_update, nullptr, SG_ACC[SG_DISPLAY]};
  up_open(&SG_APP, sg_open_last ? sg_page_of(sg_last) : 0);
}
static void sysset_update(float dt) {
  up_over = nullptr;  // (a panel closed: upcheck audits the pages again)
  up_update(dt);
}
static void sysset_layout(const HaloView &v) {
  if (up_app == &SG_APP) SG_APP.accent = SG_ACC[sg_cat[constrain((int)roundf(up_pos.x), 0, sg_n - 1)]];  // (the dot's)
  up_layout(v);
}
static void sysset_draw() { up_draw(); }
#endif
