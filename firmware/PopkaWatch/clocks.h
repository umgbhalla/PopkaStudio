// clocks.h — Stopwatch, Timer, World clock (FL_STOPWATCH, FL_TIMER, FL_WORLD): Wear OS's clock apps on the UI kit (ui.h).
//   Stopwatch: the time big (minutes:seconds, the hundredths in the accent), the progress round the edge sweeping each
//              minute; Start / Pause — the edge button; Lap / Reset; the laps under.
//   Timer:     presets; running — the time left big, the edge's progress running down, Pause / Resume, +1 minute,
//              Cancel; at the end — "Time's up" over the screen (confirm.h) and three beeps; it wakes the board for it.
//   World clock: here and the cities picked, each with its time and how far from here; Add city — the edge button: a
//              page of cities to switch on and off (kept in flash).
// Time is esp_timer's (it runs through light sleep). Round board only. Included from PopkaWatch.ino after wxapp.h.
#ifdef HALO_ROUND
#include <Preferences.h>
extern const HaloFont gl_ck_n72;  // the big time (studio.py GALLERY_FONTS: digits ":.")
#define CK_IC_FLAG "\xEF\x80\xA4"   // flag (a lap)
#define CK_IC_RESET "\xEF\x8B\xB1"  // rotate (reset)
#define CK_IC_PLUS "+"
#define CK_IC_X "\xEF\x80\x8D"
#define CK_IC_HOURGLASS "\xEF\x89\x94"
#define CK_IC_GLOBE "\xEF\x82\xAC"
#define CK_IC_HOUSE "\xEF\x80\x95"
#define CK_IC_CLOCK "\xEF\x80\x97"
static inline int64_t ck_now() {
#ifdef __EMSCRIPTEN__
  return (int64_t)halo_us();
#else
  return esp_timer_get_time();
#endif
}
static float ck_mid72, ck_mid24;
static void ck_mids() {
  if (!ck_mid72) ck_mid72 = halo_text_mid(gl_ck_n72, "0123456789"), ck_mid24 = halo_text_mid(gl_t24, "0123456789");
}
// The big time as one line: the main part big, a small tail (".34", or nothing) in the accent, centered as one
static HaloItem ck_big[2];
static char ck_main[16], ck_tail[8];
static void ck_big_lay(float y, float s, float a, uint32_t accent, const HaloView &v) {
  const float w1 = halo_text_width(gl_ck_n72, ck_main) * s, w2 = *ck_tail ? halo_text_width(gl_t24, ck_tail) * s + 4 * s : 0;
  const float x0 = CX - (w1 + w2) / 2;
  ui_text(ck_big[0], gl_ck_n72, ck_main, ck_mid72, x0, y, s, 0, GL_TEXT, a, &halo_black);
  ui_text(ck_big[1], gl_t24, ck_tail, ck_mid24, x0 + w1 + 4 * s, y + 14 * s, s, 0, accent, *ck_tail ? a : 0, &halo_black);
  ck_big[0].on_black = ck_big[1].on_black = true;
  halo_view_commit(ck_big[0], v), halo_view_commit(ck_big[1], v);
}
static void ck_big_draw() { halo_item_draw(ck_big[0]), halo_item_draw(ck_big[1]); }
static void ck_hms(char *o, int n, int64_t us, bool cs, char *tail = nullptr, int tn = 0) {
  if (us < 0) us = 0;
  const int64_t t = us / 10000;  // hundredths
  const int h = (int)(t / 360000), m = (int)(t / 6000 % 60), sec = (int)(t / 100 % 60), c = (int)(t % 100);
  if (h) snprintf(o, n, "%d:%02d:%02d", h, m, sec);
  else snprintf(o, n, "%02d:%02d", m, sec);
  if (tail) {
    if (cs && !h) snprintf(tail, tn, ".%02d", c);
    else *tail = 0;
  }
}

// ---------- Stopwatch ----------
#define SW_LAPS 8
static bool sw_run;
static int64_t sw_t0, sw_acc, sw_laps[SW_LAPS];
static int sw_nlaps, sw_built = -1;
static bool sw_built_run;
static int64_t sw_elapsed() { return sw_acc + (sw_run ? ck_now() - sw_t0 : 0); }
static void sw_hero_lay(float x0, float y0, float w, float h, float s, float a, const HaloView &v) {
  ck_mids();
  ck_hms(ck_main, sizeof(ck_main), sw_elapsed(), true, ck_tail, sizeof(ck_tail));
  ck_big_lay(y0 + h / 2, s, a, 0xA8C7FA, v);
}
static float sw_ring() {
  const int64_t e = sw_elapsed();
  return e > 0 ? (float)(e % 60000000) / 60000000.0f : -1;
}
static void sw_toggle() {
  if (sw_run) sw_acc += ck_now() - sw_t0, sw_run = false;
  else sw_t0 = ck_now(), sw_run = true;
}
static void sw_edge_label(char *o, int n) { strlcpy(o, sw_run ? "Pause" : sw_elapsed() > 0 ? "Resume" : "Start", n); }
static void sw_second(char *o, int n) { strlcpy(o, sw_run ? "Lap" : "Reset", n); }
static void sw_second_act() {
  if (sw_run) {
    if (sw_nlaps < SW_LAPS) sw_laps[sw_nlaps++] = sw_elapsed();
  } else {
    sw_acc = 0, sw_nlaps = 0;
  }
}
template <int L> static void sw_lap_line(char *o, int n) {
  const int64_t prev = L ? sw_laps[L - 1] : 0;
  char tot[16], tt[8], sp[16], st[8];
  ck_hms(tot, sizeof(tot), sw_laps[L], true, tt, sizeof(tt)), ck_hms(sp, sizeof(sp), sw_laps[L] - prev, true, st, sizeof(st));
  snprintf(o, n, "%s%s · total %s%s", sp, st, tot, tt);
}
static UiPage sw_page;
static void sw_build() {
  sw_page = {"Stopwatch", {}, 0, 0xA8C7FA};
  UiPage &p = sw_page;
  p.rows[p.n++] = ur_custom(140, sw_hero_lay, ck_big_draw);
  UiRow r = ur_compact(sw_run ? CK_IC_FLAG : CK_IC_RESET, "Lap", sw_second_act);
  r.text = sw_second;
  p.rows[p.n++] = r;
  static void (*const LAP[SW_LAPS])(char *, int) = {sw_lap_line<0>, sw_lap_line<1>, sw_lap_line<2>, sw_lap_line<3>,
                                                    sw_lap_line<4>, sw_lap_line<5>, sw_lap_line<6>, sw_lap_line<7>};
  static const char *const LAPN[SW_LAPS] = {"Lap 1", "Lap 2", "Lap 3", "Lap 4", "Lap 5", "Lap 6", "Lap 7", "Lap 8"};
  for (int i = sw_nlaps - 1; i >= 0 && p.n < UI_ROWS; i--) p.rows[p.n++] = ur_info(CK_IC_FLAG, LAPN[i], LAP[i]);  // (newest first)
  p.edge = ur_action("Start", sw_toggle);
  p.edge.text = sw_edge_label;
  p.ring = sw_ring;
  sw_built = sw_nlaps, sw_built_run = sw_run;
}

// ---------- Timer ----------
enum : uint8_t { TM_IDLE, TM_RUN, TM_PAUSED };
static uint8_t tm_state;
static int64_t tm_total, tm_end, tm_left_p;
static volatile bool tm_done;  // it ran out (the alert: ui_result, the beeps)
static int64_t tm_left() { return tm_state == TM_RUN ? max((int64_t)0, tm_end - ck_now()) : tm_state == TM_PAUSED ? tm_left_p : tm_total; }
static void tm_start(int minutes) { tm_total = (int64_t)minutes * 60000000, tm_end = ck_now() + tm_total, tm_state = TM_RUN; }
template <int M> static void tm_preset() { tm_start(M); }
static void tm_toggle() {
  if (tm_state == TM_RUN) tm_left_p = tm_left(), tm_state = TM_PAUSED;
  else if (tm_state == TM_PAUSED) tm_end = ck_now() + tm_left_p, tm_state = TM_RUN;
}
static void tm_plus() {
  tm_total += 60000000;
  if (tm_state == TM_RUN) tm_end += 60000000;
  else tm_left_p += 60000000;
}
static void tm_cancel() { tm_state = TM_IDLE; }
static void tm_edge_label(char *o, int n) { strlcpy(o, tm_state == TM_RUN ? "Pause" : "Resume", n); }
static float tm_ring() { return tm_total > 0 ? (float)tm_left() / tm_total : -1; }
static void tm_hero_lay(float x0, float y0, float w, float h, float s, float a, const HaloView &v) {
  ck_mids();
  ck_hms(ck_main, sizeof(ck_main), tm_left() + 999999, false, ck_tail, sizeof(ck_tail));  // (up to the second: 0:00 at the end)
  ck_big_lay(y0 + h / 2, s, a, 0xFFB68A, v);
}
static void tm_total_line(char *o, int n) {
  const int m = (int)(tm_total / 60000000);
  snprintf(o, n, "%d minute%s", m, m == 1 ? "" : "s");
}
static UiPage tm_set, tm_run_page;
static void tm_build() {
  tm_set = {"Timer", {}, 0, 0xFFB68A};
  static const char *const NAMES[6] = {"1 minute", "3 minutes", "5 minutes", "10 minutes", "15 minutes", "30 minutes"};
  static void (*const ACT[6])() = {tm_preset<1>, tm_preset<3>, tm_preset<5>, tm_preset<10>, tm_preset<15>, tm_preset<30>};
  for (int i = 0; i < 6; i++) {
    UiRow r = ur_nav(CK_IC_HOURGLASS, NAMES[i], nullptr);
    r.act = ACT[i];
    tm_set.rows[tm_set.n++] = r;
  }
  tm_run_page = {"Timer", {}, 0, 0xFFB68A};
  UiPage &p = tm_run_page;
  p.rows[p.n++] = ur_custom(140, tm_hero_lay, ck_big_draw);
  p.rows[p.n++] = ur_compact(CK_IC_PLUS, "1 min", tm_plus);
  p.rows[p.n++] = ur_compact(CK_IC_X, "Cancel", tm_cancel, 0xFFB4AB);
  p.rows[p.n++] = ur_info(CK_IC_HOURGLASS, "Set for", tm_total_line);
  p.edge = ur_action("Pause", tm_toggle);
  p.edge.text = tm_edge_label;
  p.ring = tm_ring;
}
// The alert: three beeps on the speaker (sound.h's alert slot: over everything)
#if SS_SOUND
static int tm_tone_i, tm_tone_n;
static float tm_ph;
static int tm_tone(int16_t *buf, int n) {
  int k = 0;
  for (; k < n && tm_tone_i < tm_tone_n; k++, tm_tone_i++) {
    const int in = tm_tone_i % 7700;  // (a beep of 0.18 s every 0.35 s at 22 050 Hz)
    const bool on = in < 4000;
    tm_ph += 2 * HALO_PI * 1046.5f / 22050;
    if (tm_ph > 2 * HALO_PI) tm_ph -= 2 * HALO_PI;
    const float env = on ? fminf(1, fminf(in, 4000 - in) / 130.0f) : 0;
    buf[k] = (int16_t)(sinf(tm_ph) * 9000 * env);
  }
  return k;
}
#endif
static void tm_check() {
  if (tm_state != TM_RUN || tm_left() > 0) return;
  tm_state = TM_IDLE, tm_done = true;
}

// ---------- World clock ----------
struct CkCity {
  const char *name, *tz;
};
static const CkCity CK_CITIES[] = {
    {"London", "GMT0BST,M3.5.0/1,M10.5.0"}, {"New York", "EST5EDT,M3.2.0,M11.1.0"},
    {"San Francisco", "PST8PDT,M3.2.0,M11.1.0"}, {"Tokyo", "JST-9"}, {"Dubai", "<+04>-4"}, {"Singapore", "<+08>-8"},
    {"Paris", "CET-1CEST,M3.5.0,M10.5.0/3"}, {"Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"}, {"Hong Kong", "HKT-8"},
    {"Moscow", "MSK-3"}, {"Sao Paulo", "<-03>3"}, {"Kolkata", "IST-5:30"}};
#define CK_N (int)(sizeof(CK_CITIES) / sizeof(CK_CITIES[0]))
static uint32_t ck_mask = 0x1 | 0x2 | 0x8 | 0x10;  // London, New York, Tokyo, Dubai
static char ck_line[CK_N][28], ck_here[28];
static int ck_min_was = -1;
static Preferences ck_prefs;
// A POSIX TZ string ("EST5EDT,M3.2.0,M11.1.0", "<+04>-4", "IST-5:30") evaluated here — the same on the watch and on
// the page (the page's C library ignores TZ: every city came out as "same time"); no setenv / tzset churn either
static const char *ck_tz_name(const char *p) {  // past a zone's name: letters, or <…>
  if (*p == '<') {
    while (*p && *p != '>') p++;
    return *p ? p + 1 : p;
  }
  while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) p++;
  return p;
}
static const char *ck_tz_hms(const char *p, long &sec) {  // [+-]h[:mm[:ss]] → seconds
  int sg = 1;
  if (*p == '+' || *p == '-') sg = *p == '-' ? -1 : 1, p++;
  long v = 0, part = 0;
  int k = 0;
  for (;;) {
    part = 0;
    while (*p >= '0' && *p <= '9') part = part * 10 + (*p++ - '0');
    v += part * (k == 0 ? 3600 : k == 1 ? 60 : 1);
    if (*p != ':' || k == 2) break;
    p++, k++;
  }
  sec = sg * v;
  return p;
}
static long ck_days(int y, int m, int d) {  // days from 1970-01-01 (m 1 … 12)
  y -= m <= 2;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const long yoe = y - era * 400, doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}
static const char *ck_tz_rule(const char *p, int year, long &at) {  // Mm.w.d[/time] → seconds from 1970 (local wall)
  int m = 3, w = 1, d = 0;
  long t = 7200;
  if (*p == 'M') sscanf(p + 1, "%d.%d.%d", &m, &w, &d);
  while (*p && *p != '/' && *p != ',') p++;
  if (*p == '/') p = ck_tz_hms(p + 1, t);
  const long first = ck_days(year, m, 1), wd1 = ((first % 7) + 11) % 7;  // weekday of the 1st (0 — Sunday)
  long day = first + (d - wd1 + 7) % 7 + (w - 1) * 7;
  const long next = m == 12 ? ck_days(year + 1, 1, 1) : ck_days(year, m + 1, 1);
  while (day >= next) day -= 7;  // (week 5: the last)
  at = day * 86400 + t;
  return p;
}
static void ck_tz_local(const char *tz, time_t now, struct tm &out) {
  long std = 0, dst;
  const char *p = ck_tz_hms(ck_tz_name(tz), std);  // (POSIX: west of Greenwich is positive)
  dst = std - 3600;
  long off = -std;
  if (*p && *p != ',') {  // a summer time
    p = ck_tz_name(p);
    if (*p && *p != ',') p = ck_tz_hms(p, dst);
    if (*p == ',') {
      struct tm u;
      gmtime_r(&now, &u);
      long a, b;
      p = ck_tz_rule(p + 1, u.tm_year + 1900, a);
      if (*p == ',') ck_tz_rule(p + 1, u.tm_year + 1900, b);
      else b = a;
      const long ws = (long)now - std, wd = (long)now - dst;  // the wall clock in standard and in summer time
      const bool summer = a < b ? ws >= a && wd < b : !(wd >= b && ws < a);  // (south: the summer spans the new year)
      if (summer) off = -dst;
    }
  }
  const time_t t = now + off;
  gmtime_r(&t, &out);
}

// Once a minute: each city's time and how far from here
static void ck_world_times() {
  const time_t now = time(nullptr);
  struct tm here;
  ck_tz_local(TZ, now, here);
  if (here.tm_min == ck_min_was) return;
  ck_min_was = here.tm_min;
  const int hm = here.tm_yday * 1440 + here.tm_hour * 60 + here.tm_min;
  snprintf(ck_here, sizeof(ck_here), fc_h24 ? "%02d:%02d" : "%d:%02d", fc_h24 ? here.tm_hour : (here.tm_hour + 11) % 12 + 1, here.tm_min);
  for (int i = 0; i < CK_N; i++) {
    if (!(ck_mask >> i & 1)) continue;
    struct tm t;
    ck_tz_local(CK_CITIES[i].tz, now, t);
    int d = t.tm_yday * 1440 + t.tm_hour * 60 + t.tm_min - hm;
    if (d > 720 * 2) d -= 365 * 1440;
    if (d < -720 * 2) d += 365 * 1440;
    char off[12];
    if (!d) strlcpy(off, "same time", sizeof(off));
    else snprintf(off, sizeof(off), d % 60 ? "%c%dh %02dm" : "%c%dh", d < 0 ? '-' : '+', abs(d) / 60, abs(d) % 60);
    const int hh = fc_h24 ? t.tm_hour : (t.tm_hour + 11) % 12 + 1;
    snprintf(ck_line[i], sizeof(ck_line[i]), fc_h24 ? "%02d:%02d · %s" : "%d:%02d · %s", hh, t.tm_min, off);
  }
}
template <int I> static void ck_city_line(char *o, int n) { strlcpy(o, ck_line[I], n); }
static void ck_here_line(char *o, int n) { strlcpy(o, ck_here, n); }
template <int I> static int ck_on() { return ck_mask >> I & 1; }
template <int I> static void ck_set(int v) {
  ck_mask = v ? ck_mask | 1u << I : ck_mask & ~(1u << I), ck_min_was = -1;
  ck_prefs.putUInt("cities", ck_mask);
}
static UiPage ck_page, ck_pick;
static void ck_build();
static void ck_open_pick() { ui_open(&ck_pick); }
static void (*const CK_LINE[])(char *, int) = {ck_city_line<0>, ck_city_line<1>, ck_city_line<2>, ck_city_line<3>,
                                               ck_city_line<4>, ck_city_line<5>, ck_city_line<6>, ck_city_line<7>,
                                               ck_city_line<8>, ck_city_line<9>, ck_city_line<10>, ck_city_line<11>};
static int (*const CK_ON[])() = {ck_on<0>, ck_on<1>, ck_on<2>, ck_on<3>, ck_on<4>, ck_on<5>,
                                 ck_on<6>, ck_on<7>, ck_on<8>, ck_on<9>, ck_on<10>, ck_on<11>};
static void (*const CK_SET[])(int) = {ck_set<0>, ck_set<1>, ck_set<2>, ck_set<3>, ck_set<4>, ck_set<5>,
                                      ck_set<6>, ck_set<7>, ck_set<8>, ck_set<9>, ck_set<10>, ck_set<11>};
static uint32_t ck_built_mask = 0xFFFFFFFF;
static void ck_build() {
  ck_page = {"World clock", {}, 0, 0xC7BFFF};
  UiPage &p = ck_page;
  p.rows[p.n++] = ur_info(CK_IC_HOUSE, "Here", ck_here_line);
  for (int i = 0; i < CK_N && p.n < UI_ROWS; i++)
    if (ck_mask >> i & 1) p.rows[p.n++] = ur_info(CK_IC_GLOBE, CK_CITIES[i].name, CK_LINE[i]);
  p.edge = ur_action("Add city", ck_open_pick);
  ck_pick = {"Cities", {}, 0, 0xC7BFFF};
  for (int i = 0; i < CK_N && ck_pick.n < UI_ROWS; i++)
    ck_pick.rows[ck_pick.n++] = ur_toggle(CK_IC_GLOBE, CK_CITIES[i].name, CK_ON[i], CK_SET[i]);
  ck_built_mask = ck_mask;
}

// ---------- The screens (settings.h calls these, by the layer's kind) ----------
static void clocks_begin() {
  ck_prefs.begin("world", false);
  ck_mask = ck_prefs.getUInt("cities", ck_mask);
}
static void clocks_enter(uint8_t kind) {
  ui_cur = 0;
  if (kind == FL_STOPWATCH) sw_build(), ui_set_page(0, &sw_page);
  else if (kind == FL_TIMER) tm_build(), ui_set_page(0, tm_state == TM_IDLE ? &tm_set : &tm_run_page);
  else ck_min_was = -1, ck_world_times(), ck_build(), ui_set_page(0, &ck_page);
}
static void clocks_update(uint8_t kind, float dt) {
  if (kind == FL_STOPWATCH && (sw_built != sw_nlaps || sw_built_run != sw_run) && ui_run && ui_run[0].pg == &sw_page) {  // (a lap: its row)
    const float sc = ui_run[0].list.scroll;
    sw_build(), ui_set_page(0, &sw_page), ui_run[0].list.scroll = sc;
  }
  if (kind == FL_TIMER && ui_run) {  // (set → running and back: the page for it)
    const UiPage *want = tm_state == TM_IDLE ? &tm_set : &tm_run_page;
    if (ui_run[0].pg != want && ui_cur == 0) ui_set_page(0, want);
  }
  if (kind == FL_WORLD) {
    ck_world_times();
    if (ck_built_mask != ck_mask && ui_cur == 0 && ui_run && (ui_run[0].pg == &ck_page)) {
      ck_build(), ui_set_page(0, &ck_page);
    }
  }
  ui_update(0, dt);
}
static void clocks_layout(uint8_t, const HaloView &v) { ui_layout(0, v); }
static void clocks_draw(uint8_t) { ui_draw(0); }
// Every frame (PopkaWatch.ino): the timer runs out wherever the watch is — the result over the screen, the beeps
static void clocks_tick() {
  tm_check();
  if (!tm_done) return;
  tm_done = false;
  ui_result(true, "Time's up", 0xFFB68A);
#if SS_SOUND
  tm_tone_i = 0, tm_tone_n = 7700 * 3, tm_ph = 0;
  snd_alert = tm_tone;
  snd_begin(22050);
#endif
}
// For power.h (settings.h): wake the board for the timer's end, and bring the screen back then
static uint32_t clocks_wake_ms() {
  if (tm_state != TM_RUN) return 0;
  const int64_t ms = tm_left() / 1000 + 20;
  return ms < 1 ? 1 : ms > 0x7FFFFFFF ? 0x7FFFFFFF : (uint32_t)ms;
}
static bool clocks_due() { return tm_state == TM_RUN && tm_left() <= 0; }
#endif
