// wxapp.h — Weather (FL_WXAPP): five non-scrolling pages swiped left and right (upager.h; Codex's measured study,
// design/SPEC_weather.md — 23 Nov 2026):
//   1 Now — the city, the sky, the temperature big, the sky and feels-like, HIGH / LOW;  2 Next 12 hours — an area chart;
//   3 Next 7 days — sky, low, high, the range on one scale;  4 Details — humidity, wind, UV, sunrise → sunset;
//   5 Settings — °C / °F (kept), the location (Use my location — by the watch's internet address, ip-api.com — kept).
// The numbers are Open-Meteo's (wxfetch.h). Round board only. Included from PopkaWatch.ino after upager.h.
#ifdef HALO_ROUND
#define WA_IC_SUN "\xEF\x86\x85"
#define WA_IC_CLOUD "\xEF\x83\x82"
#define WA_IC_PARTLY "\xEF\x9B\x84"
#define WA_IC_RAIN "\xEF\x9C\xBD"
#define WA_IC_SNOW "\xEF\x8B\x9C"
#define WA_IC_DROP "\xEF\x81\x83"
#define WA_IC_WIND "\xEF\x9C\xAE"
#define WA_IC_LOC "\xEF\x8F\x85"
#define WA_IC_CLOCK "\xEF\x80\x97"
#define WA_IC_MOON "\xEF\x86\x86"

static const char *wa_icon(uint8_t kind, bool snow = false) {
  return snow ? WA_IC_SNOW : kind == WX_SUN ? WA_IC_SUN : kind == WX_PARTLY ? WA_IC_PARTLY : kind == WX_RAIN ? WA_IC_RAIN : WA_IC_CLOUD;
}
static const char *wa_sky(uint8_t kind) {
  return kind == WX_SUN ? "Clear" : kind == WX_PARTLY ? "Partly cloudy" : kind == WX_RAIN ? "Rain" : "Cloudy";
}
static void wa_hhmm(char *o, int n, uint32_t t) {
  if (!t) return (void)strlcpy(o, "—", n);
  const time_t tt = t;
  struct tm m;
  localtime_r(&tt, &m);
  snprintf(o, n, "%d:%02d", m.tm_hour, m.tm_min);
}

// ---------- The days, the details, refresh ----------
static void wa_city(char *o, int n) { strlcpy(o, wx_city[0].name, n); }
static void wa_refresh() {
#if SS_WIFI
  xTaskNotifyGive(hclk_handle);
#endif
}
static void wa_here() {
  wx_geo_req = true;
  wa_refresh();
}

// ---------- The pages (upager.h; Codex's measured study, design/SPEC_weather.md — 23 Nov 2026) ----------
// 1 Now — the city, the sky in a honey disc, the temperature big, the sky and feels-like, HIGH and LOW cards;
// 2 Next 12 hours — an area chart on a honey card; 3 Next 7 days — each day's sky, low, high and its range on one scale;
// 4 Details — humidity, wind, UV, and the day so far between sunrise and sunset; 5 Settings — °C / °F, the location.
extern const HaloFont gl_wx_hero, gl_wx_line, gl_wx_val, gl_wx_week, gl_wx_ic38;  // (studio.py GALLERY_FONTS)
#define WA_HONEY 0xFFC983
#define WA_COOL 0xAED5FA
#define WA_HCARD 0x281D13
#define WA_HROW 0x3B2918
#define WA_ICE 0x192331
#define WA_NEUTRAL 0x1C1B20
#define WA_GRID 0x3C3230
#define WA_AREA 0x4D3625
#define WA_WTRACK 0x423A39
#define WA_DARK 0x30200F
static char wa_now[8], wa_line[40], wa_hi[8], wa_lo[8], wa_hours[4][6], wa_grid[3][6], wa_day[7][2][8], wa_det[3][8];
static char wa_rise_s[8], wa_set_s[8];
static float wa_hy[12];  // the chart's points (y), x = 116 + 24 i
static int wa_hn;
static UiPage wa_locs;
static int wa_count() { return 5; }
static float wa_mid(const HaloFont &f) { return halo_text_mid(f, "0123456789"); }
static void wa_deg(char *o, int n, int16_t t) {
  if (t == WX_NONE) strlcpy(o, "--", n);
  else snprintf(o, n, "%d\xC2\xB0", (int)t);
}
// The chart, per pixel in its band: the area under the line, the rules, the line (4 px, smoothed), a dot at each hour
static void wa_chart_paint(const UpBlob &b) {
  const float ox = b.x - 233, oy = b.y - 254;  // (where the card is now: the page's shift)
  const int y0 = max(hb.y, (int)(b.y - b.r)), y1 = min(hb.y + hb.rows - 1, (int)(b.y + b.r));
  if (wa_hn < 2) return;
  const int ar = (WA_AREA >> 16 & 255) >> 3, ag = (WA_AREA >> 8 & 255) >> 2, ab = (WA_AREA & 255) >> 3;
  const int lr = (WA_HONEY >> 16 & 255) >> 3, lg = (WA_HONEY >> 8 & 255) >> 2, lb = (WA_HONEY & 255) >> 3;
  const int gr = (WA_GRID >> 16 & 255) >> 3, gg = (WA_GRID >> 8 & 255) >> 2, gb = (WA_GRID & 255) >> 3;
  const int A = (int)(b.a * 256);
  for (int y = y0; y <= y1; y++) {
    uint16_t *row = halo_row(y);
    const float py = y + 0.5f - oy;
    for (int x = max(hb.x0, (int)(ox + 112)); x <= min(hb.x1, (int)(ox + 390)); x++) {
      const float px = x + 0.5f - ox;
      for (const float gy : {183.0f, 247.0f, 311.0f})  // the rules
        if (fabsf(py - gy) < 0.5f && px >= 113 && px <= 388) halo_blend(row + x, gr, gg, gb, A);
      if (px < 116 || px > 116 + 24 * (wa_hn - 1)) continue;
      const int k = min(wa_hn - 2, (int)((px - 116) / 24));
      const float f = (px - 116 - 24 * k) / 24, ly = wa_hy[k] + (wa_hy[k + 1] - wa_hy[k]) * f;
      const float slope = (wa_hy[k + 1] - wa_hy[k]) / 24;
      if (py > ly && py <= 311) halo_blend(row + x, ar, ag, ab, A);  // (the area)
      const float d = fabsf(py - ly) / sqrtf(1 + slope * slope);
      float cov = halo_clamp01(2.5f - d);
      const int kn = (int)roundf((px - 116) / 24);  // (the nearest hour's dot, r 2.5 over the line)
      if (kn >= 0 && kn < wa_hn) {
        const float dx = px - (116 + 24 * kn), dy = py - wa_hy[kn];
        cov = fmaxf(cov, halo_clamp01(3.5f - sqrtf(dx * dx + dy * dy)));
      }
      if (cov > 0) halo_blend(row + x, lr, lg, lb, (int)(cov * A));
    }
  }
}
static void wa_lay(int p, UpPage &P, float a, const HaloView &v) {
  const WxCity &c = wx_city[0];
  if (p == 0) {  // Now
    up_title(P, c.name, a, v);
    HaloItem &disc = up_disc(P, CX, 144, 26, WA_HCARD, a, v);
    up_text(P, gl_wx_ic38, wa_icon(c.kind), halo_text_mid(gl_wx_ic38, WA_IC_SUN), CX, 144, 1, WA_HONEY, a, v, &disc);
    wa_deg(wa_now, sizeof(wa_now), c.now);
    up_text(P, gl_wx_hero, wa_now, wa_mid(gl_wx_hero), CX, 231, 1, UP_WHITE, a, v);
    char f[8];
    wa_deg(f, sizeof(f), c.feels);
    snprintf(wa_line, sizeof(wa_line), "%s \xC2\xB7 feels %s", wa_sky(c.kind), f);
    up_text(P, gl_wx_line, wa_line, halo_text_mid(gl_wx_line, "Sfx"), CX, 311, 1, UP_MUTED, a, v);
    wa_deg(wa_hi, sizeof(wa_hi), c.hi), wa_deg(wa_lo, sizeof(wa_lo), c.lo);
    for (int k = 0; k < 2; k++) {
      const float x = k ? 295 : 171;
      up_rrect(P, x - 55, 335, x + 55, 395, 24, k ? WA_ICE : WA_HCARD, a, v);
      HaloItem &card = P.it[P.n - 1];
      up_text(P, gl_up_lab, k ? "LOW" : "HIGH", up_mid_lab, x, 350, 1, k ? WA_COOL : WA_HONEY, a, v, &card);
      up_text(P, gl_wx_val, k ? wa_lo : wa_hi, wa_mid(gl_wx_val), x, 375, 1, UP_WHITE, a, v, &card);
    }
  } else if (p == 1) {  // Next 12 hours
    up_title(P, "Next 12 hours", a, v);
    up_rrect(P, 47, 143, 419, 365, 37, WA_HCARD, a, v);
    // the next twelve hours from now (strictly future), their range padded to the rules 183 … 311
    const uint32_t now = time(nullptr);
    int first = 0;
    while (first < WX_AHEAD && c.t0 && c.t0 + first * 3600 <= now) first++;
    int lo = 999, hi = -999;
    wa_hn = 0;
    for (int i = 0; i < 12 && first + i < WX_AHEAD; i++) {
      const int t = c.ht[first + i];
      if (t == WX_NONE) break;
      lo = min(lo, t), hi = max(hi, t), wa_hn++;
    }
    if (wa_hn >= 2) {
      const float l = lo - 1, h = max(hi + 1, lo + 3);
      uint32_t key = 0;
      for (int i = 0; i < wa_hn; i++) wa_hy[i] = 311 - 128 * (c.ht[first + i] - l) / (h - l), key = key * 31 + c.ht[first + i];
      up_paint(P, 233, 254, 178, key + first, wa_chart_paint, a, v);
      const int g[3] = {(int)roundf(h), (int)roundf((h + l) / 2), (int)roundf(l)};
      for (int k = 0; k < 3; k++) {
        snprintf(wa_grid[k], 6, "%d\xC2\xB0", g[k]);
        up_text(P, gl_up_lab, wa_grid[k], up_mid_lab, 94, 183 + 64 * k, 2, UP_MUTED, a, v);
      }
      static const int IX[4] = {0, 4, 8, 11};
      for (int k = 0; k < 4; k++) {
        const int i = min(IX[k], wa_hn - 1);
        const time_t t = c.t0 + (first + i) * 3600;
        struct tm m;
        localtime_r(&t, &m);
        snprintf(wa_hours[k], 6, "%02d:00", m.tm_hour);
        up_text(P, gl_up_lab, wa_hours[k], up_mid_lab, 116 + 24 * i, 339, 1, UP_MUTED, a, v);
      }
    } else {
      up_text(P, gl_up_lab, "NO FORECAST YET", up_mid_lab, CX, 254, 1, UP_MUTED, a, v);
    }
  } else if (p == 2) {  // Next 7 days
    up_title(P, "Next 7 days", a, v);
#if WX_MORE
    up_rrect(P, 61, 125, 405, 396, 56, WA_NEUTRAL, a, v);
    up_rrect(P, 79, 131, 387, 165, 17, WA_HROW, a, v);
    int lo = 999, hi = -999;
    for (int d = 0; d < 7; d++) lo = min(lo, (int)c.dlo[d]), hi = max(hi, (int)c.dhi[d]);
    const float span = max(4, hi - lo);
    static const char *const WD[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const time_t now = time(nullptr);
    for (int d = 0; d < 7; d++) {
      const float y = 148 + 37 * d;
      const time_t t = now + d * 86400;
      struct tm m;
      localtime_r(&t, &m);
      up_text(P, gl_up_lab, WD[m.tm_wday], up_mid_lab, 95, y, 0, d ? UP_MUTED : UP_WHITE, a, v);
      const bool rain = c.dk[d] == WX_RAIN;
      HaloItem &ic = up_item(P);
      ui_text(ic, gl_ic24, wa_icon(c.dk[d], c.dsnow >> d & 1), halo_text_mid(gl_ic24, WA_IC_SUN), 152, y, 0.875f, 1,
              rain ? WA_COOL : WA_HONEY, a, &halo_black);
      ic.on_black = true, up_commit(ic, v);
      wa_deg(wa_day[d][0], 8, c.dlo[d]), wa_deg(wa_day[d][1], 8, c.dhi[d]);
      up_text(P, gl_wx_week, wa_day[d][0], wa_mid(gl_wx_week), 218, y, 2, WA_COOL, a, v);
      up_text(P, gl_wx_week, wa_day[d][1], wa_mid(gl_wx_week), 369, y, 2, UP_WHITE, a, v);
      up_rrect(P, 239, y - 3, 308, y + 3, 3, WA_WTRACK, a, v);
      const float x0 = 239 + 69 * (c.dlo[d] - lo) / span, x1 = 239 + 69 * (c.dhi[d] - lo) / span;
      up_rrect(P, x0, y - 3, fmaxf(x1, x0 + 6), y + 3, 3, WA_HONEY, a, v);
    }
#endif
  } else if (p == 3) {  // Details
    up_title(P, "Details", a, v);
#if WX_APP
    snprintf(wa_det[0], 8, "%d%%", c.hum), snprintf(wa_det[1], 8, "%d", c.wind), snprintf(wa_det[2], 8, "%d", (c.uv10 + 5) / 10);
    static const char *const LAB[3] = {"HUMIDITY", "WIND", "UV INDEX"};
    const uint32_t fill[3] = {WA_ICE, WA_NEUTRAL, WA_HCARD}, ink[3] = {WA_COOL, UP_WHITE, WA_HONEY};
    const char *icon[3] = {WA_IC_DROP, WA_IC_WIND, WA_IC_SUN};
    const int u = c.uv10;
    const char *sub[3] = {"", wx_fahr ? "mph" : "km/h", u < 30 ? "Low" : u < 60 ? "Moderate" : u < 80 ? "High" : "Very high"};
    for (int k = 0; k < 3; k++) {
      const float x = 117 + 116 * k;
      up_rrect(P, x - 53, 143, x + 53, 243, 26, fill[k], a, v);
      HaloItem &tile = P.it[P.n - 1];
      HaloItem &ic = up_item(P);
      ui_text(ic, gl_ic24, icon[k], halo_text_mid(gl_ic24, WA_IC_SUN), x, 162, 0.75f, 1, ink[k], a, &tile), up_commit(ic, v);
      up_text(P, gl_up_lab, LAB[k], up_mid_lab, x, 183, 1, ink[k], a, v, &tile);
      up_text(P, gl_wx_val, wa_det[k], wa_mid(gl_wx_val), x, k ? 208 : 215, 1, UP_WHITE, a, v, &tile);
      if (k) up_text(P, gl_up_lab, sub[k], up_mid_lab, x, 233, 1, UP_MUTED, a, v, &tile);
    }
    up_rrect(P, 84, 256, 382, 390, 36, WA_HCARD, a, v);
    up_arc(P, 54, 3, -HALO_PI / 2, HALO_PI / 2, WA_GRID, a, v);  // (centred at 233,233 by up_arc: moved below)
    HaloItem &tr = P.it[P.n - 1];
    tr.y = 328, halo_view_commit(tr, v);
    const time_t now = time(nullptr);
    float fr = c.rise && c.set > c.rise ? halo_clamp01((float)(now - c.rise) / (c.set - c.rise)) : 0;
    up_arc(P, 54, 3, -HALO_PI / 2, -HALO_PI / 2 + HALO_PI * fr, WA_HONEY, fr > 0.01f ? a : 0, v);
    HaloItem &el = P.it[P.n - 1];
    el.y = 328, halo_view_commit(el, v);
    const float an = -HALO_PI / 2 + HALO_PI * fr;
    up_disc(P, CX + 54 * sinf(an), 328 - 54 * cosf(an), 7, WA_HONEY, fr > 0 ? a : 0, v);
    up_rrect(P, 112, 328.5f, 354, 329.5f, 0.5f, WA_GRID, a, v);
    wa_hhmm(wa_rise_s, 8, c.rise), wa_hhmm(wa_set_s, 8, c.set);
    up_text(P, gl_up_lab, "SUNRISE", up_mid_lab, 154, 348, 1, UP_MUTED, a, v);
    up_text(P, gl_up_lab, "SUNSET", up_mid_lab, 312, 348, 1, UP_MUTED, a, v);
    up_text(P, gl_wx_val, wa_rise_s, wa_mid(gl_wx_val), 154, 373, 1, UP_WHITE, a, v);
    up_text(P, gl_wx_val, wa_set_s, wa_mid(gl_wx_val), 312, 373, 1, UP_WHITE, a, v);
#endif
  } else {  // Settings
    up_title(P, "Weather settings", a, v);
    up_rrect(P, 77, 138, 389, 270, 33, WA_HCARD, a, v);
    up_text(P, gl_up_lab, "UNITS", up_mid_lab, CX, 165, 1, WA_HONEY, a, v);
    for (int k = 0; k < 2; k++) {
      const bool on = (k == 1) == wx_fahr;
      const float x0 = k ? 237 : 94;
      up_rrect(P, x0, 185, x0 + 135, 254, 30, on ? WA_HONEY : WA_HROW, a, v);
      up_text(P, gl_wx_val, k ? "\xC2\xB0""F" : "\xC2\xB0""C", wa_mid(gl_wx_val), x0 + 67.5f, 219.5f, 1, on ? WA_DARK : UP_WHITE, a, v,
              &P.it[P.n - 1]);
    }
    up_rrect(P, 91, 293, 375, 380, 34, WA_ICE, a, v);
    HaloItem &card = P.it[P.n - 1];
    HaloItem &pin = up_item(P);
    ui_text(pin, gl_ic24, WA_IC_LOC, halo_text_mid(gl_ic24, WA_IC_LOC), 132, 337, 1.2f, 1, WA_COOL, a, &card), up_commit(pin, v);
    HaloItem &chev = up_item(P);
    ui_text(chev, gl_ic24, "\xEF\x81\x94", halo_text_mid(gl_ic24, "\xEF\x81\x94"), 337, 337, 0.9f, 1, WA_COOL, a, &card);
    up_commit(chev, v);
    up_text(P, gl_up_lab, "LOCATION", up_mid_lab, 167, 323, 0, WA_COOL, a, v, &card);
    up_text(P, gl_up_title, c.name, up_mid_title, 167, 351, 0, UP_WHITE, a, v, &card);
  }
}
static void wa_tap(int p, float x, float y) {
  if (p != 4) return;
  if (up_in(x, y, 94, 185, 229, 254) && wx_fahr) sy_set_units(0);
  else if (up_in(x, y, 237, 185, 372, 254) && !wx_fahr) sy_set_units(1);
  else if (up_in(x, y, 91, 293, 375, 380)) ui_cur = 0, ui_open(&wa_locs);
}
static void wa_upd(int, float) {
  static uint32_t seen = 1;
  if (wx_fetched_ms != seen) seen = wx_fetched_ms, halo_damage_all();  // (fresh weather: every page again)
}
static const UpApp WA_APP = {wa_count, wa_lay, wa_tap, wa_upd, nullptr, WA_HONEY};
static void wa_build_locs() {
  wa_locs = {"Location", {}, 0, WA_HONEY};
  UiPage &l = wa_locs;
  l.rows[l.n++] = ur_info(WA_IC_LOC, "Showing", wa_city);
#if SS_WIFI
  l.rows[l.n++] = ur_action("Use my location", wa_here, WA_HONEY);
  l.rows[l.n++] = ur_action("Refresh now", wa_refresh, WA_HONEY);
#endif
}

// The location kept (Use my location): the first city at start
static Preferences wa_prefs;
static void wxapp_begin() {
  wa_prefs.begin("wx", false);
  if (wa_prefs.isKey("lat")) {
    if (wa_prefs.getBytes("name", wx_city[0].name, sizeof(wx_city[0].name)) > 0) wx_city[0].name[sizeof(wx_city[0].name) - 1] = 0;
    wx_city[0].lat = wa_prefs.getFloat("lat", wx_city[0].lat), wx_city[0].lon = wa_prefs.getFloat("lon", wx_city[0].lon);
  }
}
static void wa_keep_location() {
  if (!wx_geo_done) return;
  wx_geo_done = false;
  wa_prefs.putBytes("name", wx_city[0].name, strlen(wx_city[0].name) + 1);
  wa_prefs.putFloat("lat", wx_city[0].lat), wa_prefs.putFloat("lon", wx_city[0].lon);
}

// The screen (settings.h calls these)
static void wxapp_enter() { wa_build_locs(), up_open(&WA_APP); }
static void wxapp_update(float dt) {
  wa_keep_location();
  up_update(dt);
}
static void wxapp_layout(const HaloView &v) { up_layout(v); }
static void wxapp_draw() { up_draw(); }
#endif
