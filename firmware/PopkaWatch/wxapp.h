// wxapp.h — Weather (FL_WXAPP): one long page on the UI kit (ui.h) — Material 3 for Wear OS, configurable.
//   Mumbai ── the hero: the sky, the temperature big, what it feels like, today's high and low
//          ├─ the next 12 hours: a line of the temperature, its highest and lowest marked
//          ├─ NEXT 7 DAYS: a row a day — the sky, the low and the high
//          ├─ NOW: humidity, wind, UV, sunrise, sunset
//          ├─ [Refresh · updated N min ago]                     — with Wi-Fi
//          └─ Locations: the city shown; Use my location (by the watch's internet address, ip-api.com), kept
// The numbers are Open-Meteo's (wxfetch.h: with this app it asks for humidity, wind, UV, sunrise and sunset too); the
// page is made again when fresh weather comes (the days' skies change their icons). Round board only.
// Included from PopkaWatch.ino after sysset.h (the kit, the Wi-Fi's sync).
#ifdef HALO_ROUND
extern const HaloFont gl_wa_n80;  // the hero's temperature (studio.py ROUND_APP_FONTS["wxapp"])
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

// ---------- The hero (a custom row): the sky's icon and the temperature big, the words under ----------
static HaloItem wa_hero[5];
static char wa_t_now[8], wa_t_line[40], wa_t_hilo[24];
static float wa_mid_n80, wa_mid_ic;
static void wa_hero_lay(float x0, float y0, float w, float h, float s, float a, const HaloView &v) {
  const WxCity &c = wx_city[0];
  wx_deg(wa_t_now, sizeof(wa_t_now), c.now);
  char f[8], hi[8], lo[8];
  wx_deg(f, sizeof(f), c.feels), wx_deg(hi, sizeof(hi), c.hi), wx_deg(lo, sizeof(lo), c.lo);
  snprintf(wa_t_line, sizeof(wa_t_line), "Feels %s · %s", f, wa_sky(c.kind));
  snprintf(wa_t_hilo, sizeof(wa_t_hilo), "H %s  L %s", hi, lo);
  const float cy = y0 + 70 * s, tw = halo_text_width(gl_wa_n80, wa_t_now) * s, iw = 64 * s, gap = 18 * s;
  const float gx = CX - (iw + gap + tw) / 2;  // (the icon and the number centered as one)
  ui_circle(wa_hero[0], gx + iw / 2, cy, 32, s, 0xFFB030, a);
  ui_text(wa_hero[1], gl_ic24, wa_icon(c.kind), wa_mid_ic, gx + iw / 2, cy, 1.25f * s, 1, 0x3D2600, a, &wa_hero[0]);
  ui_text(wa_hero[2], gl_wa_n80, wa_t_now, wa_mid_n80, gx + iw + gap, cy, s, 0, GL_TEXT, a, &halo_black);
  wa_hero[2].on_black = true;
  ui_text(wa_hero[3], gl_t20, wa_t_line, ui_mid_t20, CX, y0 + 130 * s, s, 1, GL_DIM, a, &halo_black, 340);
  wa_hero[3].on_black = true;
  ui_text(wa_hero[4], gl_t20, wa_t_hilo, ui_mid_t20, CX, y0 + 160 * s, s, 1, GL_TEXT, a, &halo_black);
  wa_hero[4].on_black = true;
  for (HaloItem &it : wa_hero) halo_view_commit(it, v);
}
static void wa_hero_draw() {
  for (HaloItem &it : wa_hero) halo_item_draw(it);
}

// ---------- The next 12 hours (a custom row): a line of dots on a tonal card, the hours under ----------
#define WA_H 12
struct WaChart {
  float x0, y0, w, h, s, a;
  float px[WA_H], py[WA_H];
  int16_t t[WA_H];
  int first, hi_i, lo_i;
  uint32_t key;
};
static WaChart wa_ch;
static HaloItem wa_card;
static void wa_chart_lay(float x0, float y0, float w, float h, float s, float a, const HaloView &v) {
  const WxCity &c = wx_city[0];
  // From this hour on (t0 is the first forecast hour)
  const time_t now = time(nullptr);
  const int first = c.t0 && now > (time_t)c.t0 ? constrain((int)((now - c.t0) / 3600), 0, WX_AHEAD - WA_H) : 0;
  int16_t lo = INT16_MAX, hi = INT16_MIN;
  for (int j = 0; j < WA_H; j++) {
    const int16_t t = c.ht[first + j];
    wa_ch.t[j] = t;
    if (t == WX_NONE) continue;
    if (t < lo) lo = t, wa_ch.lo_i = j;
    if (t > hi) hi = t, wa_ch.hi_i = j;
  }
  ui_rrect(wa_card, CX, y0 + h / 2, GL_ITEM_W, h / s, 40, s, UI_SURF, a);
  halo_view_commit(wa_card, v);
  const float ix0 = halo_view_x(v, x0 + 40 * s), ix1 = halo_view_x(v, x0 + w - 40 * s);
  const float top = halo_view_y(v, y0 + 34 * s), bot = halo_view_y(v, y0 + h - 62 * s);  // (room under for the low and the hours)
  uint32_t key = 2166136261u;
  for (int j = 0; j < WA_H; j++) {
    const int16_t t = wa_ch.t[j];
    wa_ch.px[j] = ix0 + (ix1 - ix0) * j / (WA_H - 1);
    wa_ch.py[j] = t == WX_NONE || hi <= lo ? (top + bot) / 2 : bot - (bot - top) * (t - lo) / (float)(hi - lo);
    key = (key ^ (uint32_t)lroundf(wa_ch.py[j] * 4)) * 16777619u;
  }
  key = (key ^ (uint32_t)lroundf(ix0 * 4) ^ ((uint32_t)lroundf(a * 64) << 20) ^ (uint32_t)first << 12) * 16777619u;
  wa_ch.first = first, wa_ch.s = s * v.s, wa_ch.a = a * v.a;
  if (key != wa_ch.key) {  // (moved or changed: the old place and the new one again)
    halo_damage_f(wa_ch.x0 - 4, wa_ch.y0 - 4, wa_ch.x0 + wa_ch.w + 4, wa_ch.y0 + wa_ch.h + 4);
    wa_ch.x0 = halo_view_x(v, x0), wa_ch.y0 = halo_view_y(v, y0), wa_ch.w = w * v.s, wa_ch.h = h * v.s;
    halo_damage_f(wa_ch.x0 - 4, wa_ch.y0 - 4, wa_ch.x0 + wa_ch.w + 4, wa_ch.y0 + wa_ch.h + 4);
    wa_ch.key = key;
  }
}
static void wa_chart_draw() {
  halo_item_draw(wa_card);
  const float a = wa_ch.a, s = wa_ch.s;
  if (a <= 0.01f || !halo_hits(wa_ch.x0, wa_ch.y0, wa_ch.x0 + wa_ch.w, wa_ch.y0 + wa_ch.h)) return;
  const uint32_t line = 0xFFB030;
  for (int j = 0; j + 1 < WA_H; j++) {  // the line: dots close enough to read as one
    if (wa_ch.t[j] == WX_NONE || wa_ch.t[j + 1] == WX_NONE) continue;
    const float dx = wa_ch.px[j + 1] - wa_ch.px[j], dy = wa_ch.py[j + 1] - wa_ch.py[j];
    const int n = max(1, (int)(sqrtf(dx * dx + dy * dy) / (1.6f * s)));
    for (int q = 0; q < n; q++) halo_circle(wa_ch.px[j] + dx * q / n, wa_ch.py[j] + dy * q / n, 2.2f * s, line, a);
  }
  char b[8];
  for (int j = 0; j < WA_H; j++) {
    if (wa_ch.t[j] == WX_NONE) continue;
    const bool mark = j == wa_ch.hi_i || j == wa_ch.lo_i;
    if (mark) {  // the highest and the lowest: a dot and the number over / under it
      halo_circle(wa_ch.px[j], wa_ch.py[j], 5 * s, line, a), halo_circle(wa_ch.px[j], wa_ch.py[j], 2.5f * s, UI_SURF, a);
      wx_deg(b, sizeof(b), wa_ch.t[j]);
      const float ty = j == wa_ch.hi_i ? wa_ch.py[j] - 28 * s : wa_ch.py[j] + 6 * s;
      halo_text(gl_t20, b, wa_ch.px[j], ty, GL_TEXT, a, 1, s);
    }
    if (j % 3 == 0) {  // the hours under, every third
      const time_t t = (time_t)wx_city[0].t0 + (wa_ch.first + j) * 3600;
      struct tm m;
      localtime_r(&t, &m);
      snprintf(b, sizeof(b), j ? "%d" : "Now", m.tm_hour);
      halo_text(gl_t20, b, wa_ch.px[j], wa_ch.y0 + wa_ch.h - 32 * s, GL_DIM, a, 1, s);
    }
  }
}

// ---------- The days, the details, refresh ----------
template <int D> static void wa_day_line(char *o, int n) {
  const WxCity &c = wx_city[0];
  if (!c.t0 || (c.dlo[D] == 0 && c.dhi[D] == 0)) return (void)strlcpy(o, "—", n);  // (no forecast yet: the editor's copy)
  char lo[8], hi[8];
  wx_deg(lo, sizeof(lo), c.dlo[D]), wx_deg(hi, sizeof(hi), c.dhi[D]);
  snprintf(o, n, "%s – %s · %s", lo, hi, wa_sky(c.dk[D]));
}
static const char *wa_day_name(int d) {
  static const char *const WD[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  if (d == 0) return "Today";
  const time_t t = time(nullptr) + d * 86400;
  struct tm m;
  localtime_r(&t, &m);
  return WD[m.tm_wday];
}
static void wa_hum(char *o, int n) { snprintf(o, n, "%d%%", wx_city[0].hum); }
static void wa_wind(char *o, int n) { snprintf(o, n, "%d %s", wx_city[0].wind, wx_fahr ? "mph" : "km/h"); }
static void wa_uv(char *o, int n) {
  const int u = wx_city[0].uv10;
  snprintf(o, n, "%d.%d · %s", u / 10, u % 10, u < 30 ? "low" : u < 60 ? "moderate" : u < 80 ? "high" : "very high");
}
static void wa_rise(char *o, int n) { wa_hhmm(o, n, wx_city[0].rise); }
static void wa_set(char *o, int n) { wa_hhmm(o, n, wx_city[0].set); }
static void wa_city(char *o, int n) { strlcpy(o, wx_city[0].name, n); }
static void wa_refresh_label(char *o, int n) {
  if (!wx_fetched_ms) return (void)strlcpy(o, "Refresh", n);
  const uint32_t m = (halo_ms() - wx_fetched_ms) / 60000;
  if (m < 1) strlcpy(o, "Refresh · just now", n);
  else snprintf(o, n, "Refresh · %u min ago", (unsigned)m);
}
static void wa_refresh() {
#if SS_WIFI
  xTaskNotifyGive(hclk_handle);
#endif
}
static void wa_here() {
  wx_geo_req = true;
  wa_refresh();
}

static UiPage wa_page, wa_locs;
static uint32_t wa_built_ms = 1;
static void wa_build() {
  wa_mid_n80 = halo_text_mid(gl_wa_n80, "0123456789"), wa_mid_ic = halo_text_mid(gl_ic24, WA_IC_SUN);
  const WxCity &c = wx_city[0];
  wa_page = {c.name, {}, 0, 0xFFB030};
  UiPage &p = wa_page;
  p.rows[p.n++] = ur_custom(184, wa_hero_lay, wa_hero_draw);
  p.rows[p.n++] = ur_custom(150, wa_chart_lay, wa_chart_draw);
  p.rows[p.n++] = ur_head("NEXT 7 DAYS");
  static void (*const DAY[7])(char *, int) = {wa_day_line<0>, wa_day_line<1>, wa_day_line<2>, wa_day_line<3>,
                                              wa_day_line<4>, wa_day_line<5>, wa_day_line<6>};
  for (int d = 0; d < 7 && p.n < UI_ROWS; d++)
    p.rows[p.n++] = ur_info(wa_icon(c.dk[d], c.dsnow >> d & 1), wa_day_name(d), DAY[d]);
  // (the page has room for UI_ROWS: the details go on their own short page under NOW)
  wa_locs = {"Weather", {}, 0, 0xFFB030};
  UiPage &l = wa_locs;
  l.rows[l.n++] = ur_head("NOW");
  l.rows[l.n++] = ur_info(WA_IC_DROP, "Humidity", wa_hum);
  l.rows[l.n++] = ur_info(WA_IC_WIND, "Wind", wa_wind);
  l.rows[l.n++] = ur_info(WA_IC_SUN, "UV index", wa_uv);
  l.rows[l.n++] = ur_info(WA_IC_SUN, "Sunrise", wa_rise);
  l.rows[l.n++] = ur_info(WA_IC_MOON, "Sunset", wa_set);
  l.rows[l.n++] = ur_head("LOCATION");
  l.rows[l.n++] = ur_info(WA_IC_LOC, "Showing", wa_city);
#if SS_WIFI
  l.rows[l.n++] = ur_action("Use my location", wa_here, 0xFFB030);
#endif
#if SS_WIFI
  if (p.n < UI_ROWS) {
    UiRow r = ur_action("Refresh", wa_refresh, 0xFFB030);
    r.text = wa_refresh_label;
    p.rows[p.n++] = r;
  }
#endif
  if (p.n < UI_ROWS) p.rows[p.n++] = ur_nav(WA_IC_DROP, "Details and location", &wa_locs, wa_city, 0xFFB030);
  wa_built_ms = wx_fetched_ms;
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
static void wxapp_enter() {
  wa_build();
  ui_cur = 0;
  ui_set_page(0, &wa_page);
}
static void wxapp_update(float dt) {
  wa_keep_location();
  if (wx_fetched_ms != wa_built_ms && ui_run && ui_run[0].pg == &wa_page) {  // (fresh weather: the rows again, in place)
    const float scroll = ui_run[0].list.scroll;
    wa_build();
    ui_set_page(0, &wa_page);
    ui_run[0].list.scroll = scroll;
  }
  ui_update(0, dt);
}
static void wxapp_layout(const HaloView &v) { ui_layout(0, v); }
static void wxapp_draw() { ui_draw(0); }
#endif
