// sysset.h — Settings (FL_SETTINGS): one place for the watch's settings, many levels — the UI kit's first app (ui.h).
//   Settings ── Display (brightness, Always On: on, after, how bright; wake on tap / raise; burn-in shift)
//            ├─ Time (24-hour; the zone; the last sync)
//            ├─ Wi-Fi (the network, how it went, sync now)              — with Wi-Fi in the firmware
//            ├─ Sound (the volume)                                      — with the speaker in it
//            ├─ Battery (the level, the voltage, USB)
//            └─ System (the firmware, memory; restart, power off)
// Every change is the watch's at once and kept in flash (Preferences "sys": what was not kept before — Always On and its
// timing, how it wakes, 12/24 hours, the volume); sysset_begin brings them back at start. Brightness keeps its own key
// (settings.h, Display), as the charge limit does.
// Included from PopkaWatch.ino after power.h (its settings are power.h's); round board only.
#ifdef HALO_ROUND
#include "ui.h"
#include "upager.h"  // (the paged apps — Weather, the clocks, the media apps — come in with it)
#include <Preferences.h>

static Preferences sy_prefs;
static bool sy_ready;
static void sy_put(const char *key, int v) {
  if (sy_ready) sy_prefs.putInt(key, v);
}

// ---------- The settings: get, set (applied and kept) ----------
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
static const char *const SY_BTN_NAMES[] = {"Nothing", "Screen off", "Lock", "Always On", "Home", "Back"};  // (Back — a level back, then the clock)
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
static void sy_display_line(char *o, int n) {
#if HAS_DISPLAY
  snprintf(o, n, pw_aod_on ? "Brightness %d · Always On" : "Brightness %d", dp_value);  // (not "Always On on")
#else
  snprintf(o, n, "Always On %s", pw_aod_on ? "on" : "off");
#endif
}
static void sy_time_line(char *o, int n) { strlcpy(o, fc_h24 ? "24-hour clock" : "12-hour clock", n); }
static void sy_zone(char *o, int n) { strlcpy(o, TZ, n); }
#if SS_WIFI
static const char *sy_wifi_state() {
  static const char *const S[4] = {"Waiting", "Connected · time synced", "Network not found", "No internet"};
  return S[hclk_state & 3];
}
static void sy_wifi_line(char *o, int n) { strlcpy(o, *hclk_ssid ? hclk_ssid : "Not set", n); }
static void sy_wifi_net(char *o, int n) { strlcpy(o, *hclk_ssid ? hclk_ssid : "Not set", n); }
static void sy_wifi_status(char *o, int n) { strlcpy(o, sy_wifi_state(), n); }
static void sy_wifi_saved(char *o, int n) { snprintf(o, n, "%d network%s", hclk_nets, hclk_nets == 1 ? "" : "s"); }
static void sy_wifi_sync() { xTaskNotifyGive(hclk_handle); }
#endif
#if SS_SOUND
static void sy_sound_line(char *o, int n) { snprintf(o, n, "Volume %d of 10", vol_level); }
#endif
static void sy_bat_line(char *o, int n) {
  const HaloBattery b = halo_battery();
  if (b.ok && b.percent >= 0) snprintf(o, n, "%d%%%s", b.percent, b.usb ? (b.charging ? " · charging" : " · on USB") : "");
  else strlcpy(o, "—", n);
}
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
static void sy_faces_line(char *o, int n) {
  snprintf(o, n, "%s%s", sy_face_names[fc_face] ? sy_face_names[fc_face] : "Watch face", fs_on && face_n > 1 ? " · swipe to switch" : "");
}
static void sy_buttons_line(char *o, int n) { snprintf(o, n, "BOOT %s · PWR %s", SY_BTN_NAMES[sy_boot()], SY_BTN_NAMES[sy_pwr()]); }
static void sy_weather_line(char *o, int n) {
  char t[8];
  wx_deg(t, sizeof(t), wx_city[0].now);
  snprintf(o, n, "%s · %s", wx_city[0].name, t);
}
static void sy_weather_city(char *o, int n) { strlcpy(o, wx_city[0].name, n); }
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
#define SS_IC_CHIP "\xEF\x8B\x9B"    // (Firmware)
#define SS_IC_MEMORY "\xEF\x94\xB8"  // (Memory: each row its own icon, not one wrench for all)
static UiPage sy_root, sy_display, sy_time, sy_wifi, sy_sound, sy_battery, sy_system, sy_faces, sy_buttons, sy_weather;
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
  sy_add(sy_display, ur_choice(SS_IC_CONTRAST, "Brightness", SS_DIM_NAMES, 3, sy_dim, sy_set_dim));  // (under the Always On header: it says whose)
  sy_add(sy_display, ur_toggle(SS_IC_CONTRAST, "Burn-in shift", sy_shift, sy_set_shift));
  sy_add(sy_display, ur_head("Wake up"));
  sy_add(sy_display, ur_toggle(SS_IC_SUN, "Tap to wake", sy_tap, sy_set_tap));
  sy_add(sy_display, ur_toggle(SS_IC_BOLT, "Raise to wake", sy_raise, sy_set_raise));

  sy_time = {"Time", {}, 0, 0xA4F9B5};
  sy_add(sy_time, ur_toggle(SS_IC_CLOCK, "24-hour clock", sy_h24, sy_set_h24));
  sy_add(sy_time, ur_info(SS_IC_LOCATION, "Time zone", sy_zone));
#if SS_WIFI
  sy_add(sy_time, ur_info(SS_IC_WIFI, "Synced over Wi-Fi", sy_wifi_status));
#endif

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
  if (face_n > 1) sy_add(sy_faces, ur_toggle("\xEF\x8B\xB1", "Swipe to switch", sy_fswipe, sy_set_fswipe));  // (rotating arrows: switching — not the picture again)

  sy_buttons = {"Buttons", {}, 0, 0xC7BFFF};
  sy_add(sy_buttons, ur_choice(SY_IC_LOCK, "BOOT button", SY_BTN_NAMES, 6, sy_boot, sy_set_boot));
  sy_add(sy_buttons, ur_choice(SY_IC_HOUSE, "PWR button", SY_BTN_NAMES, 6, sy_pwr, sy_set_pwr));

  sy_weather = {"Weather", {}, 0, 0xFFB030};
  sy_add(sy_weather, ur_info(SS_IC_LOCATION, "Location", sy_weather_city));
  sy_add(sy_weather, ur_choice(SY_IC_CLOUD, "Units", SY_UNIT_NAMES, 2, sy_units, sy_set_units));
#if SS_WIFI
  sy_weather.edge = ur_action("Refresh now", sy_wifi_sync, 0xFFB030);
#endif

  sy_system = {"System", {}, 0, 0xC7BFFF};
  sy_add(sy_system, ur_info(SS_IC_CHIP, "Firmware", sy_fw));
  sy_add(sy_system, ur_info(SS_IC_MEMORY, "Memory", sy_mem));
  sy_add(sy_system, ur_action("Restart", sy_restart, 0xA8C7FA));
  sy_add(sy_system, ur_action("Power off", power_confirm_open, 0xFFB4AB));

  sy_root = {"Settings", {}, 0, 0xA8C7FA};
  sy_add(sy_root, ur_nav(SS_IC_CONTRAST, "Display", &sy_display, sy_display_line, 0xFFD27A));
  sy_add(sy_root, ur_nav(SY_IC_IMAGE, "Watch faces", &sy_faces, sy_faces_line, 0xFFB68A));
  sy_add(sy_root, ur_nav(SY_IC_LOCK, "Buttons", &sy_buttons, sy_buttons_line, 0xC7BFFF));
  sy_add(sy_root, ur_nav(SY_IC_CLOUD, "Weather", &sy_weather, sy_weather_line, 0xFFB030));
  sy_add(sy_root, ur_nav(SS_IC_CLOCK, "Time", &sy_time, sy_time_line, 0xA4F9B5));
#if SS_WIFI
  sy_add(sy_root, ur_nav(SS_IC_WIFI, "Wi-Fi", &sy_wifi, sy_wifi_line, 0xA8C7FA));
#endif
#if SS_SOUND
  sy_add(sy_root, ur_nav(SS_IC_VOLUME, "Sound", &sy_sound, sy_sound_line, 0xF6B5EA));
#endif
  sy_add(sy_root, ur_nav(SS_IC_BATTERY, "Battery", &sy_battery, sy_bat_line, 0xA4F9B5));
  sy_add(sy_root, ur_nav(SS_IC_WRENCH, "System", &sy_system, nullptr, 0xC7BFFF));
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

// The root: the project's screen with the Settings layer (settings.h calls these)
static void sysset_enter() {
  if (!sy_root.n) sy_build();
  ui_cur = 0;
  ui_set_page(0, &sy_root);
}
static void sysset_update(float dt) { ui_update(0, dt); }
static void sysset_layout(const HaloView &v) { ui_layout(0, v); }
static void sysset_draw() { ui_draw(0); }
#endif
