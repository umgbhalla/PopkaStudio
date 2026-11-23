// clocks.h — Stopwatch, Timer, World clock (FL_STOPWATCH, FL_TIMER, FL_WORLD): Wear OS's clock apps as paged,
// non-scrolling screens (upager.h — the Wear OS study, 23 Nov 2026: pages swiped left and right with dots, big round icon
// buttons, the rim's progress), not lists.
//   Stopwatch: the time, Lap / Reset and Start / Pause; page 2 — the laps (fastest green, slowest red).
//   Timer:     set with two picker columns; running — the time left, Cancel, Pause / Resume, +1 min; page 2 — presets.
//              At the end — "Time's up" over the screen (confirm.h) and three beeps; it wakes the board for it.
//   World clock: a page a place (here, then the cities picked); the last page opens the list of cities (kept in flash).
// Time is esp_timer's (it runs through light sleep). Round board only. Included from PopkaWatch.ino after wxapp.h.
#ifdef HALO_ROUND
#include <Preferences.h>
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
static int sw_nlaps;
static int64_t sw_elapsed() { return sw_acc + (sw_run ? ck_now() - sw_t0 : 0); }
static float sw_ring() {
  const int64_t e = sw_elapsed();
  return e > 0 ? (float)(e % 60000000) / 60000000.0f : -1;
}
static void sw_toggle() {
  if (sw_run) sw_acc += ck_now() - sw_t0, sw_run = false;
  else sw_t0 = ck_now(), sw_run = true;
}
static void sw_second_act() {
  if (sw_run) {
    if (sw_nlaps < SW_LAPS) sw_laps[sw_nlaps++] = sw_elapsed();
  } else {
    sw_acc = 0, sw_nlaps = 0;
  }
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
static float tm_ring() { return tm_total > 0 ? (float)tm_left() / tm_total : -1; }
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

template <int I> static int ck_on() { return ck_mask >> I & 1; }
template <int I> static void ck_set(int v) {
  ck_mask = v ? ck_mask | 1u << I : ck_mask & ~(1u << I);
  ck_prefs.putUInt("cities", ck_mask);
}
static int (*const CK_ON[])() = {ck_on<0>, ck_on<1>, ck_on<2>, ck_on<3>, ck_on<4>, ck_on<5>,
                                 ck_on<6>, ck_on<7>, ck_on<8>, ck_on<9>, ck_on<10>, ck_on<11>};
static void (*const CK_SET[])(int) = {ck_set<0>, ck_set<1>, ck_set<2>, ck_set<3>, ck_set<4>, ck_set<5>,
                                      ck_set<6>, ck_set<7>, ck_set<8>, ck_set<9>, ck_set<10>, ck_set<11>};

// ---------- The apps as pages (upager.h): non-scrolling, swiped left and right ----------
// Stopwatch: page 1 — the time big, the hundredths in the accent, two buttons: the tonal Lap / Reset left, the primary
//            Start / Pause right (a circle at rest, a rounded square while it runs); the rim sweeps each minute.
//            Page 2 — the laps, newest first, the fastest green and the slowest red.
#define SW_ACC 0xA8C7FA
static char sw_main[16], sw_tail[8], sw_lap[SW_LAPS][2][16];
static HaloSpring sw_morph;
static int sw_count() { return 2; }
static void sw_primary() {
  if (sw_run) sw_acc += ck_now() - sw_t0, sw_run = false;
  else sw_t0 = ck_now(), sw_run = true;
}
static void sw_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p == 0) {
    ck_hms(sw_main, sizeof(sw_main), sw_elapsed(), true, sw_tail, sizeof(sw_tail));
    up_time(P, sw_main, sw_tail, 196, SW_ACC, a, v);
    const bool any = sw_run || sw_elapsed() > 0;
    up_button(P, 161, 322, 42, sw_run ? UP_IC_FLAG : UP_IC_RESET, false, 0, SW_ACC, any ? a : 0, v);
    up_button(P, any ? 293 : 233, 322, 58, sw_run ? UP_IC_PAUSE : UP_IC_PLAY, true, sw_morph.x, SW_ACC, a, v);
  } else {
    up_text(P, gl_t24, "Laps", ui_mid_t24, CX, 92, 1, 1, GL_TEXT, a, v);
    if (!sw_nlaps) {
      up_text(P, gl_t20, "No laps yet", ui_mid_t20, CX, 233, 1, 1, UI_DIM, a, v);
      return;
    }
    int fast = 0, slow = 0;
    int64_t sp[SW_LAPS];
    for (int i = 0; i < sw_nlaps; i++) {
      sp[i] = sw_laps[i] - (i ? sw_laps[i - 1] : 0);
      if (sp[i] < sp[fast]) fast = i;
      if (sp[i] > sp[slow]) slow = i;
    }
    for (int r = 0; r < 5 && r < sw_nlaps; r++) {  // (the newest five: no scrolling)
      const int i = sw_nlaps - 1 - r;
      const float y = 148 + r * 54;
      snprintf(sw_lap[i][0], 16, "Lap %d", i + 1);
      char t[8];
      ck_hms(sw_lap[i][1], 16, sp[i], true, t, sizeof(t)), strlcat(sw_lap[i][1], t, 16);
      const uint32_t c = sw_nlaps > 1 && i == fast ? 0x8FD9A8 : sw_nlaps > 1 && i == slow ? 0xFFB4AB : GL_TEXT;
      up_text(P, gl_t20, sw_lap[i][0], ui_mid_t20, 108, y, 1, 0, UI_DIM, a, v);
      up_text(P, gl_t24, sw_lap[i][1], ui_mid_t24, 358, y, 1, 2, c, a, v);
    }
  }
}
static void sw_tap(int p, float x, float y) {
  if (p) return;
  const bool any = sw_run || sw_elapsed() > 0;
  if (up_hit(x, y, any ? 293 : 233, 322, 58)) sw_primary();
  else if (any && up_hit(x, y, 161, 322, 42)) sw_second_act();
}
static void sw_upd(int, float dt) {
  static HaloSpringK k;
  static bool kk;
  if (!kk) k = halo_spring(HALO_M3X_DEFAULT), kk = true;
  sw_morph.target = sw_run ? 1 : 0;
  halo_spring_run(sw_morph, k, dt);
  if (sw_run) halo_damage(60, 140, 406, 250);  // (the time runs: its line every frame)
}
static const UpApp SW_APP = {sw_count, sw_lay, sw_tap, sw_upd, sw_ring, SW_ACC};

// Timer: page 1 — set: two picker columns (minutes : seconds) and a Start button; running: the time left big, the rim
//        running down, Cancel (tonal, left), Pause / Resume (primary, morphing), +1 min (tonal, right).
//        Page 2 — six presets as round chips; a tap starts it and turns back to page 1.
#define TM_ACC 0xFFB68A
static UpPicker tm_pm, tm_ps;
static char tm_pbuf[2][4][4], tm_main[16], tm_tail[8], tm_chip[6][4];
static HaloSpring tm_morph;
static int tm_count() { return 2; }
static void tm_go_secs(int secs) {
  if (secs <= 0) return;
  tm_total = (int64_t)secs * 1000000, tm_end = ck_now() + tm_total, tm_state = TM_RUN;
}
static const int TM_PRESETS[6] = {1, 3, 5, 10, 15, 30};
static void tm_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p == 0) {
    if (tm_state == TM_IDLE) {
      up_picker_lay(P, tm_pm, 150, 206, tm_pbuf[0], a, v);
      up_text(P, gl_up_n96, ":", up_mid_n96, CX, 206, 1, 1, UI_DIM, a, v);
      up_picker_lay(P, tm_ps, 316, 206, tm_pbuf[1], a, v);
      up_text(P, gl_t20, "MIN", ui_mid_t20, 150, 78, 1, 1, UI_DIM, a * 0.8f, v);  // (above the number before)
      up_text(P, gl_t20, "SEC", ui_mid_t20, 316, 78, 1, 1, UI_DIM, a * 0.8f, v);
      const bool ok = up_picker_value(tm_pm) * 60 + up_picker_value(tm_ps) > 0;
      up_button(P, CX, 364, 46, UP_IC_PLAY, true, 0, TM_ACC, ok ? a : a * 0.35f, v);
    } else {
      ck_hms(tm_main, sizeof(tm_main), tm_left() + 999999, false, tm_tail, sizeof(tm_tail));
      up_time(P, tm_main, nullptr, 196, TM_ACC, a, v);
      up_button(P, 120, 318, 40, UP_IC_X, false, 0, 0xFFB4AB, a, v);
      up_button(P, CX, 330, 58, tm_state == TM_RUN ? UP_IC_PAUSE : UP_IC_PLAY, true, tm_morph.x, TM_ACC, a, v);
      up_button(P, 346, 318, 40, UP_IC_PLUS, false, 0, TM_ACC, a, v);
    }
  } else {
    up_text(P, gl_t24, "Quick start", ui_mid_t24, CX, 96, 1, 1, GL_TEXT, a, v);
    for (int i = 0; i < 6; i++) {
      const float x = 133 + (i % 3) * 100, y = 196 + (i / 3) * 108;
      snprintf(tm_chip[i], 4, "%d", TM_PRESETS[i]);
      HaloItem &d = up_item(P);
      d.kind = HI_CIRCLE, d.x = x, d.y = y, d.r = 44, d.scale = 1, d.color = ui_container(TM_ACC), d.alpha = a;
      d.on_black = true, d.on = nullptr;
      up_commit(d, v);
      HaloItem &t = up_item(P);
      ui_text(t, gl_up_n36, tm_chip[i], up_mid_n36, x, y - 6, 1, 1, ui_on_container(TM_ACC), a, &d);
      up_commit(t, v);
      HaloItem &l = up_item(P);
      ui_text(l, gl_t20, "min", ui_mid_t20, x, y + 22, 0.8f, 1, ui_on_container(TM_ACC), a * 0.8f, &d);
      up_commit(l, v);
    }
  }
}
static void tm_tap(int p, float x, float y) {
  if (p == 0) {
    if (tm_state == TM_IDLE) {
      if (up_hit(x, y, CX, 364, 46)) tm_go_secs(up_picker_value(tm_pm) * 60 + up_picker_value(tm_ps));
    } else if (up_hit(x, y, CX, 330, 58)) {
      if (tm_state == TM_RUN) tm_left_p = tm_left(), tm_state = TM_PAUSED;
      else tm_end = ck_now() + tm_left_p, tm_state = TM_RUN;
    } else if (up_hit(x, y, 120, 318, 40)) {
      tm_state = TM_IDLE;
    } else if (up_hit(x, y, 346, 318, 40)) {
      tm_total += 60000000;
      if (tm_state == TM_RUN) tm_end += 60000000;
      else tm_left_p += 60000000;
    }
  } else {
    for (int i = 0; i < 6; i++)
      if (up_hit(x, y, 133 + (i % 3) * 100, 196 + (i / 3) * 108, 44)) tm_go_secs(TM_PRESETS[i] * 60), up_go(0);
  }
}
static void tm_upd(int p, float dt) {
  static HaloSpringK k;
  static bool kk;
  if (!kk) k = halo_spring(HALO_M3X_DEFAULT), kk = true, tm_pm = {}, tm_ps = {}, tm_pm.hi = 99, tm_ps.hi = 59,
           halo_spring_set(tm_pm.pos, 5), halo_spring_set(tm_ps.pos, 0);
  tm_morph.target = tm_state == TM_RUN ? 1 : 0;
  halo_spring_run(tm_morph, k, dt);
  if (p == 0 && tm_state == TM_IDLE) up_picker_update(tm_pm, 150, 70, dt), up_picker_update(tm_ps, 316, 70, dt);
}
static const UpApp TM_APP = {tm_count, tm_lay, tm_tap, tm_upd, tm_ring, TM_ACC};

// World clock: a page a place — here first, then each city picked: its name, its time big, how far from here, the sun
// or the moon; the last page — Cities (a tap: the kit's list of cities to pick).
#define CK_ACC 0xC7BFFF
static int ck_idx[CK_N + 1], ck_n;
static char ck_time[CK_N + 1][8], ck_off[CK_N + 1][16];
static bool ck_day[CK_N + 1];
static UiPage ck_pick;
static void ck_list() {
  ck_n = 0;
  ck_idx[ck_n++] = -1;  // (here)
  for (int i = 0; i < CK_N && ck_n < UP_PAGES - 1; i++)
    if (ck_mask >> i & 1) ck_idx[ck_n++] = i;
}
static int ck_count() { return ck_n + 1; }
static void ck_fill(int p) {
  const time_t now = time(nullptr);
  struct tm here, t;
  ck_tz_local(TZ, now, here);
  const int i = ck_idx[p];
  if (i < 0) t = here;
  else ck_tz_local(CK_CITIES[i].tz, now, t);
  snprintf(ck_time[p], 8, fc_h24 ? "%02d:%02d" : "%d:%02d", fc_h24 ? t.tm_hour : (t.tm_hour + 11) % 12 + 1, t.tm_min);
  int d = (t.tm_yday * 1440 + t.tm_hour * 60 + t.tm_min) - (here.tm_yday * 1440 + here.tm_hour * 60 + here.tm_min);
  if (d > 1440) d -= 365 * 1440;
  if (d < -1440) d += 365 * 1440;
  if (i < 0) snprintf(ck_off[p], 16, "%s %d", WEEKDAY[here.tm_wday % 7], here.tm_mday);  // (here: the date — the title says Here)
  else if (!d) strlcpy(ck_off[p], "Same time", 16);
  else snprintf(ck_off[p], 16, d % 60 ? "%c%dh %02dm" : "%c%dh", d < 0 ? '-' : '+', abs(d) / 60, abs(d) % 60);
  ck_day[p] = t.tm_hour >= 6 && t.tm_hour < 18;
}
static void ck_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p < ck_n) {
    ck_fill(p);
    const int i = ck_idx[p];
    up_text(P, gl_title, i < 0 ? "Here" : CK_CITIES[i].name, ui_mid_title, CX, 132, 1, 1, GL_TEXT, a, v);
    up_text(P, gl_up_n96, ck_time[p], up_mid_n96, CX, 228, 1, 1, GL_TEXT, a, v);
    up_text(P, gl_t24, ck_off[p], ui_mid_t24, CX, 312, 1, 1, CK_ACC, a, v);
    up_text(P, gl_up_ic40, ck_day[p] ? UP_IC_SUN : UP_IC_MOON, up_mid_ic, CX, 374, 0.7f, 1, ck_day[p] ? 0xFFD580 : 0xC7BFFF, a, v);
  } else {
    up_text(P, gl_t24, "Cities", ui_mid_t24, CX, 150, 1, 1, GL_TEXT, a, v);
    up_button(P, CX, 250, 58, UP_IC_GLOBE, true, 0, CK_ACC, a, v);
    up_text(P, gl_t20, "Pick the places", ui_mid_t20, CX, 344, 1, 1, UI_DIM, a, v);
  }
}
static void ck_tap(int p, float x, float y) {
  if (p == ck_n && up_hit(x, y, CX, 250, 58)) ui_cur = 0, ui_open(&ck_pick);
}
static uint32_t ck_listed = 0xFFFFFFFF;
static void ck_upd(int, float) {
  if (ck_listed != ck_mask) ck_listed = ck_mask, ck_list();
  static int min_was = -1;  // (a new minute: the times again)
  const time_t now = time(nullptr);
  if (now / 60 != min_was) min_was = now / 60, halo_damage_all();
}
static const UpApp CK_APP = {ck_count, ck_lay, ck_tap, ck_upd, nullptr, CK_ACC};

// ---------- The screens (settings.h calls these, by the layer's kind) ----------
static void clocks_begin() {
  ck_prefs.begin("world", false);
  ck_mask = ck_prefs.getUInt("cities", ck_mask);
}
static void clocks_enter(uint8_t kind) {
  if (kind == FL_STOPWATCH) up_open(&SW_APP);
  else if (kind == FL_TIMER) up_open(&TM_APP);
  else {
    ck_list(), ck_listed = ck_mask;
    ck_pick = {"Cities", {}, 0, CK_ACC};
    for (int i = 0; i < CK_N && ck_pick.n < UI_ROWS; i++)
      ck_pick.rows[ck_pick.n++] = ur_toggle(UP_IC_GLOBE, CK_CITIES[i].name, CK_ON[i], CK_SET[i]);
    up_open(&CK_APP);
  }
}
static void clocks_update(uint8_t, float dt) { up_update(dt); }
static void clocks_layout(uint8_t, const HaloView &v) { up_layout(v); }
static void clocks_draw(uint8_t) { up_draw(); }
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
