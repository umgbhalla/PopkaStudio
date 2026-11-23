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
  float lat, lon;  // (the sun: sunrise and sunset for the day/night ring)
};
static const CkCity CK_CITIES[] = {
    {"London", "GMT0BST,M3.5.0/1,M10.5.0", 51.51f, -0.13f}, {"New York", "EST5EDT,M3.2.0,M11.1.0", 40.71f, -74.01f},
    {"San Francisco", "PST8PDT,M3.2.0,M11.1.0", 37.77f, -122.42f}, {"Tokyo", "JST-9", 35.68f, 139.69f},
    {"Dubai", "<+04>-4", 25.20f, 55.27f}, {"Singapore", "<+08>-8", 1.35f, 103.82f},
    {"Paris", "CET-1CEST,M3.5.0,M10.5.0/3", 48.86f, 2.35f}, {"Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3", -33.87f, 151.21f},
    {"Hong Kong", "HKT-8", 22.32f, 114.17f}, {"Moscow", "MSK-3", 55.76f, 37.62f}, {"Sao Paulo", "<-03>3", -23.55f, -46.63f},
    {"Kolkata", "IST-5:30", 22.57f, 88.36f}};
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

// ---------- The apps as pages (upager.h) — Codex's measured study (apps/SPEC.md, 23 Nov 2026) ----------
// Shared: the title at y 98, the secondary (r 38) at (161, 352), the primary (r 55, circle → four lobes while running)
// at (287, 352), the rim from 26° to 334°.
#define UP_SEC_X 161.0f
#define UP_PRI_X 287.0f
#define UP_ROW_Y 352.0f
#define UP_SEC_R 38.0f
#define UP_PRI_R 55.0f
static HaloSpringK K_MORPH;
static void ck_morph(HaloSpring &m, bool on, float dt) {
  static bool k;
  if (!k) K_MORPH = halo_spring(HALO_M3X_FAST), k = true;
  m.target = on ? 1 : 0;
  halo_spring_run(m, K_MORPH, dt);
}

// Stopwatch: page 1 — the time (white, the hundredths in the accent 8 px after it, bottoms aligned), Reset / Lap and
// Start / Pause; page 2 — the newest three laps as cards, the fastest mint, the slowest amber, each said in words
#define SW_ACC 0xD9CBFF
#define SW_BG 0x29233A
#define SW_INK 0x22163C
static char sw_main[16], sw_tail[8], sw_lap[3][2][16];
static HaloSpring sw_morph;
static int sw_count() { return 2; }
static void sw_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p == 0) {
    up_title(P, "Stopwatch", a, v);
    ck_hms(sw_main, sizeof(sw_main), sw_elapsed(), true, sw_tail, sizeof(sw_tail));
    const float w1 = halo_text_width(gl_up_disp, sw_main), w2 = halo_text_width(gl_up_num, sw_tail);
    const float x0 = CX - (w1 + 8 + w2) / 2;
    up_text(P, gl_up_disp, sw_main, up_mid_disp, x0, 209, 0, UP_WHITE, a, v);
    // (bottoms aligned: the small digits' baseline on the big ones')
    const float base = 209 - up_mid_disp + gl_up_disp.ascent, y2 = base - gl_up_num.ascent + up_mid_num;
    up_text(P, gl_up_num, sw_tail, up_mid_num, x0 + w1 + 8, y2, 0, SW_ACC, a, v);
    const bool any = sw_run || sw_elapsed() > 0;
    up_secondary(P, UP_SEC_X, UP_ROW_Y, UP_SEC_R, sw_run ? UP_IC_FLAG : UP_IC_RESET, SW_BG, any ? SW_ACC : UP_DIM, a, v);
    up_primary(P, UP_PRI_X, UP_ROW_Y, UP_PRI_R, sw_morph.x, sw_run ? UP_IC_PAUSE : UP_IC_PLAY, SW_ACC, SW_INK, a, v);
  } else {
    up_title(P, "Laps", a, v);
    if (!sw_nlaps) return up_text(P, gl_up_lab, "NO LAPS YET", up_mid_lab, CX, 249, 1, UP_MUTED, a, v);
    int fast = 0, slow = 0;
    int64_t sp[SW_LAPS];
    for (int i = 0; i < sw_nlaps; i++) {
      sp[i] = sw_laps[i] - (i ? sw_laps[i - 1] : 0);
      if (sp[i] < sp[fast]) fast = i;
      if (sp[i] > sp[slow]) slow = i;
    }
    static const float RY[3] = {167, 249, 331};
    for (int r = 0; r < 3 && r < sw_nlaps; r++) {
      const int i = sw_nlaps - 1 - r;
      const bool f = sw_nlaps > 1 && i == fast, sl = sw_nlaps > 1 && i == slow;
      const uint32_t bg = f ? 0x122720 : sl ? 0x2B211B : UP_ROW, ink = f ? 0xB7EBD6 : sl ? 0xF4C18B : UP_MUTED;
      up_rrect(P, 76, RY[r] - 34, 390, RY[r] + 34, 24, bg, a, v);
      HaloItem &card = P.it[P.n - 1];
      snprintf(sw_lap[r][0], 16, "%02d", i + 1);
      char t[8];
      ck_hms(sw_lap[r][1], 16, sp[i], true, t, sizeof(t)), strlcat(sw_lap[r][1], t, 16);
      up_text(P, gl_up_num, sw_lap[r][0], up_mid_num, 101, RY[r] - (f || sl ? 9 : 0), 0, ink, a, v, &card);
      if (f || sl) up_text(P, gl_up_lab, f ? "FASTEST" : "SLOWEST", up_mid_lab, 101, RY[r] + 17, 0, ink, a, v, &card);
      up_text(P, gl_up_num, sw_lap[r][1], up_mid_num, 364, RY[r], 2, UP_WHITE, a, v, &card);
    }
  }
}
static void sw_tap(int p, float x, float y) {
  if (p) return;
  if (up_hit(x, y, UP_PRI_X, UP_ROW_Y, UP_PRI_R)) {
    if (sw_run) sw_acc += ck_now() - sw_t0, sw_run = false;
    else sw_t0 = ck_now(), sw_run = true;
  } else if (up_hit(x, y, UP_SEC_X, UP_ROW_Y, UP_SEC_R) && (sw_run || sw_elapsed() > 0)) {
    sw_second_act();
  }
}
static void sw_upd(int, float dt) {
  ck_morph(sw_morph, sw_run, dt);
  if (sw_run) halo_damage(40, 160, 426, 260);  // (the time runs: its line every frame)
}
static float sw_ring_f() {
  const int64_t e = sw_elapsed();
  return e > 0 ? (float)(e % 60000000) / 60000000.0f : -1;
}
static const UpApp SW_APP = {sw_count, sw_lay, sw_tap, sw_upd, sw_ring_f, SW_ACC};

// Timer: page 1 — set: minutes and seconds as picker columns (panels, a tile round the value, the field in focus
// outlined in the accent; drag, or tap the number above or below), Presets and Start; running — the time left, "OF 05:30",
// +1 min (paused: Cancel) and Pause / Resume; page 2 — six presets
#define TM_ACC 0xD9F28F
#define TM_BG 0x242B18
#define TM_INK 0x232D0C
static UpPicker tm_pm, tm_ps;
static int tm_focus;  // 0 — minutes, 1 — seconds
static char tm_pv[2][5][4], tm_main[16], tm_tail[8], tm_of[16], tm_chip[6][4];
static HaloSpring tm_morph;
static const int TM_PRESETS[6] = {1, 3, 5, 10, 15, 30};
static int tm_count() { return 2; }
static void tm_go_secs(int secs) {
  if (secs <= 0) return;
  tm_total = (int64_t)secs * 1000000, tm_end = ck_now() + tm_total, tm_state = TM_RUN;
}
// A picker column as Wear OS's (Google's picker references): bare numbers — the value big in the middle (white in the
// column in focus, the accent in the other), the ones before and after smaller and fading toward the edges; no panels,
// no tiles. Dragged, or a tap on the number above or below steps it
static void tm_column(UpPage &P, UpPicker &k, int col, float cx, float a, const HaloView &v) {
  const bool foc = tm_focus == col;
  const int val = up_picker_value(k);
  const float frac = k.pos.x - roundf(k.pos.x);  // (sliding while dragged)
  for (int j = -2; j <= 2; j++) {
    const int vv = val + j;
    const float d = j - frac, ad = fabsf(d);
    const bool ok = vv >= k.lo && vv <= k.hi && ad < 1.9f;
    snprintf(tm_pv[col][j + 2], 4, "%02d", ok ? vv : 0);
    const float y = 206 + d * 74;
    if (ad < 0.5f)
      up_text(P, gl_up_pick, tm_pv[col][j + 2], up_mid_pick, cx, y, 1, foc ? UP_WHITE : TM_ACC, a, v);
    else
      up_text(P, gl_up_num, tm_pv[col][j + 2], up_mid_num, cx, y, 1, UP_DIM, ok ? a * halo_clamp01(1.6f - ad) : 0, v);
  }
}
// The confirm hugging the bottom edge (Wear OS's edge button): the accent, a dark glyph
static void tm_edge(UpPage &P, const char *icon, float a, const HaloView &v) {
  HaloItem &e = up_item(P);
  e.kind = HI_EDGE, e.x = CX, e.y = CY, e.r = UI_EB_R, e.h = 96, e.w = 236, e.v = 48, e.scale = 1;
  e.color = TM_ACC, e.alpha = a, e.on_black = true, e.on = nullptr;
  up_commit(e, v);
  up_text(P, gl_up_ic43, icon, up_mid_ic, CX + 3, CY + UI_EB_R - 48, 1, TM_INK, a, v, &e);
  P.edge = true;  // (the dots step aside: the edge button holds the bottom)
}
static void tm_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p == 0) {
    if (tm_state != TM_IDLE) up_title(P, "Timer", a, v);
    if (tm_state == TM_IDLE) {
      // (Google's time picker: the focused field's name on top, the two columns, the confirm at the bottom edge)
      up_text(P, gl_up_lab, tm_focus ? "SECONDS" : "MINUTES", up_mid_lab, CX, 108, 1, UP_MUTED, a, v);
      tm_column(P, tm_pm, 0, 160, a, v);
      up_text(P, gl_up_disp, ":", up_mid_disp, CX, 200, 1, UP_DIM, a, v);
      tm_column(P, tm_ps, 1, 306, a, v);
      const bool ok = up_picker_value(tm_pm) * 60 + up_picker_value(tm_ps) > 0;
      tm_edge(P, UP_IC_PLAY, ok ? a : a * 0.35f, v);
    } else {
      ck_hms(tm_main, sizeof(tm_main), tm_left() + 999999, false, tm_tail, sizeof(tm_tail));
      up_text(P, gl_up_hero, tm_main, up_mid_hero, CX, 206, 1, UP_WHITE, a, v);
      char tot[16];
      ck_hms(tot, sizeof(tot), tm_total, false);
      snprintf(tm_of, sizeof(tm_of), "OF %s", tot);
      up_text(P, gl_up_lab, tm_of, up_mid_lab, CX, 280, 1, UP_MUTED, a, v);
      if (tm_state == TM_PAUSED) {
        up_secondary(P, UP_SEC_X, UP_ROW_Y, UP_SEC_R, UP_IC_X, TM_BG, 0xFFB4AB, a, v);
      } else {
        HaloItem &d = up_disc(P, UP_SEC_X, UP_ROW_Y, UP_SEC_R, TM_BG, a, v);
        up_text(P, gl_up_ic24, UP_IC_PLUS, halo_text_mid(gl_up_ic24, "+"), UP_SEC_X, 343, 1, TM_ACC, a, v, &d);
        up_text(P, gl_up_lab, "1 min", up_mid_lab, UP_SEC_X, 368, 1, TM_ACC, a, v, &d);
      }
      up_primary(P, UP_PRI_X, UP_ROW_Y, UP_PRI_R, tm_morph.x, tm_state == TM_RUN ? UP_IC_PAUSE : UP_IC_PLAY, TM_ACC,
                 TM_INK, a, v);
    }
  } else {
    up_title(P, "Presets", a, v);
    for (int i = 0; i < 6; i++) {
      const float x = 124 + (i % 3) * 109, y = i < 3 ? 191 : 301;
      snprintf(tm_chip[i], 4, "%d", TM_PRESETS[i]);
      HaloItem &d = up_disc(P, x, y, 46, TM_BG, a, v);
      up_text(P, gl_up_pre, tm_chip[i], up_mid_pre, x, y - 9, 1, TM_ACC, a, v, &d);
      up_text(P, gl_up_lab, "min", up_mid_lab, x, y + 18, 1, UP_MUTED, a, v, &d);
    }
  }
}
static void tm_tap(int p, float x, float y) {
  if (p == 0) {
    if (tm_state == TM_IDLE) {
      if (y > CY + UI_EB_R - 100) return tm_go_secs(up_picker_value(tm_pm) * 60 + up_picker_value(tm_ps));  // (the edge)
      for (int col = 0; col < 2; col++) {
        const float cx = col ? 306 : 160;
        UpPicker &k = col ? tm_ps : tm_pm;
        if (x < cx - 70 || x > cx + 70) continue;
        if (up_in(x, y, cx - 70, 110, cx + 70, 170)) up_picker_step(k, -1), tm_focus = col;
        else if (up_in(x, y, cx - 70, 244, cx + 70, 330)) up_picker_step(k, 1), tm_focus = col;
        else tm_focus = col;
      }
    } else if (up_hit(x, y, UP_PRI_X, UP_ROW_Y, UP_PRI_R)) {
      if (tm_state == TM_RUN) tm_left_p = tm_left(), tm_state = TM_PAUSED;
      else tm_end = ck_now() + tm_left_p, tm_state = TM_RUN;
    } else if (up_hit(x, y, UP_SEC_X, UP_ROW_Y, UP_SEC_R)) {
      if (tm_state == TM_PAUSED) tm_state = TM_IDLE;  // (paused: Cancel)
      else tm_total += 60000000, tm_end += 60000000;  // (+1 min, to what is left and to the total)
    }
  } else {
    for (int i = 0; i < 6; i++)
      if (up_hit(x, y, 124 + (i % 3) * 109, i < 3 ? 191 : 301, 46)) tm_go_secs(TM_PRESETS[i] * 60), up_go(0);
  }
}
static void tm_upd(int p, float dt) {
  static bool init;
  if (!init) {
    init = true, tm_pm.hi = 99, tm_ps.hi = 59;
    halo_spring_set(tm_pm.pos, 5), halo_spring_set(tm_ps.pos, 0);
  }
  ck_morph(tm_morph, tm_state == TM_RUN, dt);
  if (p == 0 && tm_state == TM_IDLE) {
    const bool was = tm_pm.drag || tm_ps.drag;
    up_picker_update(tm_pm, 160, 70, dt), up_picker_update(tm_ps, 306, 70, dt);
    if (!was && tm_pm.drag) tm_focus = 0;
    if (!was && tm_ps.drag) tm_focus = 1;
  }
}
static float tm_ring_f() { return tm_state == TM_IDLE ? -2 : tm_total > 0 ? (float)tm_left() / tm_total : -1; }
static const UpApp TM_APP = {tm_count, tm_lay, tm_tap, tm_upd, tm_ring_f, TM_ACC};

// World clock: a page a place — here (the weather's city), then each city picked: its name, its time big, its UTC
// offset, DAYLIGHT or NIGHT; round the rim the day — night along the track, the daylight from sunrise to sunset over it
// (worked out for the place and the date), and a dot at the time it is there. The last page opens the list of cities.
#define CK_ACC 0xCCC7FF
#define CK_DAY 0xFFCA8A
#define CK_NIGHT 0x5F5A88
static int ck_idx[CK_N + 1], ck_n;
static char ck_time[CK_N + 1][8], ck_utc[CK_N + 1][12];
static bool ck_isday[CK_N + 1];
static float ck_rise[CK_N + 1], ck_set_h[CK_N + 1], ck_now_h[CK_N + 1];
static UiPage ck_pick;
static void ck_list() {
  ck_n = 0;
  ck_idx[ck_n++] = -1;  // (here)
  for (int i = 0; i < CK_N && ck_n < UP_PAGES - 1; i++)
    if (ck_mask >> i & 1) ck_idx[ck_n++] = i;
}
static int ck_count() { return ck_n + 1; }
// Sunrise and sunset in local hours (the sun's declination and hour angle; no equation of time: within ~15 min)
static void ck_sun(float lat, float lon, float utc_h, int yday, float &rise, float &set) {
  const float decl = 23.44f * sinf(2 * HALO_PI * (284 + yday + 1) / 365.0f) * (HALO_PI / 180), la = lat * (HALO_PI / 180);
  const float c = -tanf(la) * tanf(decl);
  const float noon = 12 - lon / 15 + utc_h;
  if (c >= 1) rise = set = noon;            // (polar night)
  else if (c <= -1) rise = 0, set = 24;     // (midnight sun)
  else {
    const float h = acosf(c) * (180 / HALO_PI) / 15;
    rise = noon - h, set = noon + h;
  }
}
static void ck_fill(int p) {
  const time_t now = time(nullptr);
  struct tm u, t;
  gmtime_r(&now, &u);
  const int i = ck_idx[p];
  ck_tz_local(i < 0 ? TZ : CK_CITIES[i].tz, now, t);
  snprintf(ck_time[p], 8, fc_h24 ? "%02d:%02d" : "%d:%02d", fc_h24 ? t.tm_hour : (t.tm_hour + 11) % 12 + 1, t.tm_min);
  int off = (t.tm_yday * 1440 + t.tm_hour * 60 + t.tm_min) - (u.tm_yday * 1440 + u.tm_hour * 60 + u.tm_min);
  if (off > 1440) off -= 365 * 1440;
  if (off < -1440) off += 365 * 1440;
  snprintf(ck_utc[p], 12, "UTC%c%02d:%02d", off < 0 ? '-' : '+', abs(off) / 60, abs(off) % 60);
  const float lat = i < 0 ? wx_city[0].lat : CK_CITIES[i].lat, lon = i < 0 ? wx_city[0].lon : CK_CITIES[i].lon;
  ck_sun(lat, lon, off / 60.0f, t.tm_yday, ck_rise[p], ck_set_h[p]);
  ck_now_h[p] = t.tm_hour + t.tm_min / 60.0f;
  ck_isday[p] = ck_now_h[p] >= ck_rise[p] && ck_now_h[p] < ck_set_h[p];
}
static inline float ck_ang(float h) { return UP_A0 + UP_SWEEP * halo_clamp01(h / 24); }
static void ck_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p < ck_n) {
    ck_fill(p);
    const int i = ck_idx[p];
    const uint32_t tint = ck_isday[p] ? CK_DAY : CK_ACC;
    up_arc(P, UP_RING_R, UP_RING_W, UP_A0, UP_A0 + UP_SWEEP, CK_NIGHT, a, v);              // the night
    up_arc(P, UP_RING_R, UP_RING_W, ck_ang(ck_rise[p]), ck_ang(ck_set_h[p]), CK_DAY, a, v);  // the daylight
    float mx, my;
    const float an = ck_ang(ck_now_h[p]);
    mx = CX + UP_RING_R * sinf(an), my = CY - UP_RING_R * cosf(an);
    up_disc(P, mx, my, 10, 0x000000, a, v), up_disc(P, mx, my, 6, UP_WHITE, a, v);           // now
    up_title(P, i < 0 ? "Here" : CK_CITIES[i].name, a, v);
    up_text(P, gl_up_hero, ck_time[p], up_mid_hero, CX, 209, 1, UP_WHITE, a, v);
    up_text(P, gl_up_num, ck_utc[p], up_mid_num, CX, 303, 1, UP_MUTED, a, v);
    const char *lab = ck_isday[p] ? "DAYLIGHT" : "NIGHT";
    const float lw = halo_text_width(gl_up_lab, lab), gx = CX - (24 + 10 + lw) / 2;
    up_text(P, gl_up_ic24, ck_isday[p] ? UP_IC_SUN : UP_IC_MOON, halo_text_mid(gl_up_ic24, UP_IC_SUN), gx + 12, 350, 1, tint, a, v);
    up_text(P, gl_up_lab, lab, up_mid_lab, gx + 34, 350, 0, tint, a, v);
  } else {
    up_title(P, "Cities", a, v);
    up_primary(P, CX, 250, UP_PRI_R, 0, UP_IC_GLOBE, CK_ACC, 0x1C1A33, a, v);
    up_text(P, gl_up_lab, "PICK THE PLACES", up_mid_lab, CX, 336, 1, UP_MUTED, a, v);
  }
}
static void ck_tap(int p, float x, float y) {
  if (p == ck_n && up_hit(x, y, CX, 250, UP_PRI_R)) ui_cur = 0, ui_open(&ck_pick);
}
static uint32_t ck_listed = 0xFFFFFFFF;
static void ck_upd(int, float) {
  if (ck_listed != ck_mask) ck_listed = ck_mask, ck_list();
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
