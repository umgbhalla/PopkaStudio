// wxapp.h — Weather (FL_WXAPP): a page a city, swiped left and right, Settings last (round from both ends); each page a
// Wear OS list on upager.h's list mode — the same scroll as the Apps list (cards shrink and fade at the rim, the scroll
// indicator, the time going away). The numbers are Open-Meteo's (wxfetch.h). Round board only. Included from
// PopkaWatch.ino after upager.h.
#ifdef HALO_ROUND
#define WA_IC_LOC "\xEF\x8F\x85"
static void wa_hhmm(char *o, int n, uint32_t t) {
  if (!t) return (void)strlcpy(o, "—", n);
  const time_t tt = t;
  struct tm m;
  localtime_r(&tt, &m);
  snprintf(o, n, "%02d:%02d", m.tm_hour, m.tm_min);  // (06:48, as 17:59: one width)
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

// ---------- The pages: Wear OS lists (upager.h's list mode — the gallery's scroll: cards shrink and fade at the rim) ----------
// One page a city, cities left and right, Settings last (round from both ends). A city's page, top to bottom: the hero
// (the city, the sky beside the temperature, the sky's word, high and low — the rim's arc: where now is between today's
// low and high), Next 12 hours (three cards of four hours), 7 days (a pill a day: the sky, low, the range on one scale,
// high), Details (Wear OS progress cards: humidity, wind, UV with a ring each; the sun's day as an arc), the source.
// The house's tokens (the Apps list, gallery.h): cards 0x2E3034, full pills 410 wide, the Weather card's amber, warm
// white and grey; the sky drawn in shapes (the face's pill: a yellow sun, a white cloud), no icon font.
extern const HaloFont gl_wx_hero, gl_wx_line, gl_wx_val, gl_wx_week, gl_title, gl_t24, gl_t20;  // (studio.py GALLERY_FONTS)
#define WA_ACC 0xFFB030    // the Weather card's colour (the Apps list)
#define WA_CARD 0x2E3034   // the house's card
#define WA_TXT 0xFFF8EE    // warm white
#define WA_DIM 0x9E9E9E    // a card's second line
#define WA_TRACK 0x45474C  // a ring's or a bar's track
#define WA_SUNC 0xFCC425   // the sky: the sun, the cloud, rain, snow
#define WA_CLOUDC 0xEEF0F3
#define WA_RAINC 0x8AB4F8
#define WA_W 410.0f        // a card's width (GL_ITEM_W)
#define WA_X0 (CX - WA_W / 2)
#define WA_X1 (CX + WA_W / 2)
static char wa_now[8], wa_line[40], wa_hl[32], wa_hour[12][8], wa_ht[12][8], wa_day[7][2][8], wa_det[3][8], wa_src[40];
static char wa_rise_s[8], wa_set_s[8];
static UiPage wa_locs;
static int wa_cities();
static int wa_count() { return wa_cities() + 1; }
static float wa_mid(const HaloFont &f) { return halo_text_mid(f, "0123456789"); }
static int wa_cities() { return max(1, min((int)WXD_N, UP_PAGES - 1)); }

// The parts, in an item's own coordinates (upager.h's lay_item: x — the screen's, y — from the item's top)
static HaloItem &wa_card(UpPage &P, float y0, float h, float abg, const HaloView &v, float r = -1, uint32_t c = WA_CARD) {
  HaloItem &b = up_item(P);
  ui_rrect(b, CX, y0 + h / 2, WA_W, h, r < 0 ? fminf(h / 2, 52) : r, 1, c, abg);
  b.on_black = true;
  up_commit(b, v);
  return b;
}
static void wa_txt(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                   const HaloView &v, const HaloItem *on = &halo_black, float fit = 0) {
  HaloItem &t = up_item(P);
  ui_text(t, f, s, halo_text_mid(f, s), x, y, 1, align, c, a, on, fit);
  t.on_black = on == &halo_black;
  up_commit(t, v);
}
static void wa_num(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                   const HaloView &v, const HaloItem *on = &halo_black) {  // (digits: one middle for all, nothing jumps)
  HaloItem &t = up_item(P);
  ui_text(t, f, s, wa_mid(f), x, y, 1, align, c, a, on);
  t.on_black = on == &halo_black;
  up_commit(t, v);
}
static void wa_dot(UpPage &P, float x, float y, float r, uint32_t c, float a, const HaloView &v) {
  HaloItem &b = up_item(P);
  ui_circle(b, x, y, r, 1, c, a);
  b.on_black = true;
  up_commit(b, v);
}
static void wa_bar(UpPage &P, float x0, float x1, float y, float h, uint32_t c, float a, const HaloView &v) {
  HaloItem &b = up_item(P);
  ui_rrect(b, (x0 + x1) / 2, y, x1 - x0, h, h / 2, 1, c, a);
  b.on_black = true;
  up_commit(b, v);
}
// A cloud of three bumps on a base, k — its size (1: 64 px wide), grown by g px all round (the gap round it)
static void wa_cloud(UpPage &P, float x, float y, float k, float g, uint32_t c, float a, const HaloView &v) {
  wa_dot(P, x - 17 * k, y + 5 * k, 15 * k + g, c, a, v);
  wa_dot(P, x + 2 * k, y - 6 * k, 20 * k + g, c, a, v);
  wa_dot(P, x + 19 * k, y + 6 * k, 13 * k + g, c, a, v);
  wa_bar(P, x - 30 * k - g, x + 30 * k + g, y + 11 * k, 22 * k + 2 * g, c, a, v);
}
// The sky (WX_PARTLY … WX_SNOW) in a box `size` px tall centred at x, y; ground — what it lies on (the gap's colour)
static void wa_sky_icon(UpPage &P, uint8_t sky, float x, float y, float size, uint32_t ground, float a, const HaloView &v) {
  const float k = size / 64;
  if (sky == WX_SUN) return wa_dot(P, x, y, 25 * k, WA_SUNC, a, v);
  if (sky == WX_PARTLY) {
    wa_dot(P, x + 10 * k, y - 11 * k, 19 * k, WA_SUNC, a, v);
    wa_cloud(P, x - 5 * k, y + 7 * k, 0.8f * k, fmaxf(2, 3 * k), ground, a, v);
    return wa_cloud(P, x - 5 * k, y + 7 * k, 0.8f * k, 0, WA_CLOUDC, a, v);
  }
  const bool fall = sky == WX_RAIN || sky == WX_SNOW;
  wa_cloud(P, x, y - (fall ? 8 : 0) * k, 0.9f * k, 0, WA_CLOUDC, a, v);
  if (sky == WX_RAIN)
    for (int i = -1; i <= 1; i++) wa_bar(P, x + 16 * i * k - 2.5f * k, x + 16 * i * k + 2.5f * k, y + 24 * k, 12 * k, WA_RAINC, a, v);
  else if (sky == WX_SNOW)
    for (int i = -1; i <= 1; i++) wa_dot(P, x + 16 * i * k, y + 24 * k, 4 * k, WA_CLOUDC, a, v);
}
static const char *wa_sky(uint8_t sky) {
  return sky == WX_SUN ? "Clear" : sky == WX_PARTLY ? "Partly cloudy" : sky == WX_RAIN ? "Rain" : sky == WX_SNOW ? "Snow" : "Cloudy";
}
static inline uint8_t wa_sky_now(const WxCity &c) {
#if WX_MORE
  return wx_sky_of(c.kind, c.snow);
#else
  return c.kind & 3;
#endif
}

// ---------- A city's items ----------
enum : uint8_t { WI_HERO, WI_HEAD, WI_HOURS, WI_DAY, WI_DET, WI_SUN, WI_FOOT, WI_SET_UNITS, WI_SET_LOC };
struct WaItem {
  uint8_t kind, arg;
};
static WaItem wa_items[UP_PAGES][HALO_LIST_MAX];
static int wa_ni[UP_PAGES];
static const char *const WA_HEADS[] = {"Next 12 hours", "7 days", "Details", "Weather"};
static int wa_hours_from(const WxCity &c) {  // the first hour strictly after now
  const uint32_t now = time(nullptr);
  int first = 0;
  while (first < WX_AHEAD && c.t0 && c.t0 + first * 3600 <= now) first++;
  return first;
}
static int wa_build(int p) {
  int n = 0;
  WaItem *it = wa_items[p];
  auto add = [&](uint8_t k, uint8_t a = 0) { if (n < HALO_LIST_MAX) it[n++] = {k, a}; };
  if (p >= wa_cities()) {  // Settings
    add(WI_HEAD, 3), add(WI_SET_UNITS), add(WI_SET_LOC);
    return wa_ni[p] = n;
  }
  const WxCity &c = wx_city[p];
  add(WI_HERO);
  const int first = wa_hours_from(c);
  int hours = 0;
  while (hours < 12 && first + hours < WX_AHEAD && c.ht[first + hours] != WX_NONE) hours++;
  if (hours >= 4) {
    add(WI_HEAD, 0);
    for (int k = 0; k + 4 <= hours; k += 4) add(WI_HOURS, k);
  }
#if WX_MORE
  if (c.dhi[0] != WX_NONE) {
    add(WI_HEAD, 1);
    for (int d = 0; d < 7 && d < WX_DAYS && c.dhi[d] != WX_NONE; d++) add(WI_DAY, d);
  }
#endif
#if WX_APP
  add(WI_HEAD, 2), add(WI_DET, 0), add(WI_DET, 1), add(WI_DET, 2);
  if (c.rise && c.set > c.rise) add(WI_SUN);
#endif
  add(WI_FOOT);
  return wa_ni[p] = n;
}
static int wa_n_items(int p) { return wa_build(p); }
static float wa_item_h(int p, int k) {
  switch (wa_items[p][k].kind) {
    case WI_HERO: return 410;
    case WI_HEAD: return p >= wa_cities() ? 70 : 58;
    case WI_HOURS: return 150;
    case WI_DAY: return 84;
    case WI_DET: return 120;
    case WI_SUN: return 170;
    case WI_SET_UNITS: return 156;
    case WI_SET_LOC: return 104;
    default: return 64;
  }
}

static void wa_lay_item(int p, int k, UpPage &P, float abg, float ac, const HaloView &v) {
  const WaItem &w = wa_items[p][k];
  const WxCity &c = wx_city[p < wa_cities() ? p : 0];
  const float h = wa_item_h(p, k);
  switch (w.kind) {
    case WI_HERO: {  // (its mass centred ~16 px over the screen's middle: the optical centre) one centre line, the whole circle at rest (nothing of the next item shows under it): the city
                     // under the curved time, the sky, the temperature by its digits (the ° hangs past them), the sky's
                     // word and feels-like, high and low
      wa_txt(P, gl_title, c.name, CX, 34, 1, WA_TXT, ac, v, &halo_black, 300);
      wa_sky_icon(P, wa_sky_now(c), CX, 84, 48, 0x000000, ac, v);
      wx_deg(wa_now, sizeof(wa_now), c.now, "--");
      char digits[8];
      strlcpy(digits, wa_now, sizeof(digits));
      if (char *deg = strstr(digits, "\xC2\xB0")) *deg = 0;
      const float dw = halo_text_width(gl_wx_hero, digits);
      wa_num(P, gl_wx_hero, wa_now, CX - dw / 2, 174, 0, WA_TXT, ac, v);
      char f[8];
      wx_deg(f, sizeof(f), c.feels, "--");
      snprintf(wa_line, sizeof(wa_line), "%s \xC2\xB7 feels %s", wa_sky(wa_sky_now(c)), f);
      wa_txt(P, gl_wx_line, wa_line, CX, 258, 1, WA_DIM, ac, v, &halo_black, 330);
      char hi[8], lo[8];
      wx_deg(hi, sizeof(hi), c.hi, "--"), wx_deg(lo, sizeof(lo), c.lo, "--");
      snprintf(wa_hl, sizeof(wa_hl), "H %s  \xC2\xB7  L %s", hi, lo);
      wa_txt(P, gl_wx_line, wa_hl, CX, 298, 1, WA_TXT, ac, v);
      break;
    }
    case WI_HEAD: {
      const bool big = p >= wa_cities();
      wa_txt(P, big ? gl_title : gl_t24, WA_HEADS[w.arg], CX, big ? 34 : h / 2 + 4, 1, big ? WA_TXT : WA_ACC, ac, v);
      break;
    }
    case WI_HOURS: {  // four hours: the time, the sky, the degrees (the Pixel's weather tile)
      HaloItem &card = wa_card(P, 0, h, abg, v, 48);
      const int first = wa_hours_from(c);
      for (int j = 0; j < 4; j++) {
        const int i = w.arg + j, hh = first + i;
        const float x = CX + (j - 1.5f) * 92;
        const time_t t = c.t0 + hh * 3600;
        struct tm m;
        localtime_r(&t, &m);
        char *hs = wa_hour[min(i, 11)], *ts = wa_ht[min(i, 11)];
        if (i == 0) strlcpy(hs, "Now", 8);
        else snprintf(hs, 8, "%02d:00", m.tm_hour);
        wa_txt(P, gl_t20, hs, x, 32, 1, i == 0 ? WA_ACC : WA_DIM, ac, v, &card);
#if WX_MORE
        const uint8_t sky = wx_sky_of(c.hk[hh], c.hsnow >> hh & 1);
#else
        const uint8_t sky = c.hk[hh] & 3;
#endif
        wa_sky_icon(P, sky, x, 76, 40, WA_CARD, ac, v);
        wx_deg(ts, 8, c.ht[hh], "--");
        wa_num(P, gl_wx_week, ts, x + 4, 120, 1, WA_TXT, ac, v, &card);
      }
      break;
    }
#if WX_MORE
    case WI_DAY: {  // a pill: the day, the sky, low, the range on the week's one scale, high
      HaloItem &card = wa_card(P, 0, h, abg, v);
      const int d = w.arg;
      static const char *const WD[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      const time_t t = time(nullptr) + d * 86400;
      struct tm m;
      localtime_r(&t, &m);
      const float y = h / 2;
      wa_txt(P, gl_t24, d ? WD[m.tm_wday] : "Today", WA_X0 + 38, y, 0, WA_TXT, ac, v, &card);
      wa_sky_icon(P, wx_sky_of(c.dk[d], c.dsnow >> d & 1), WA_X0 + 150, y, 36, WA_CARD, ac, v);
      int lo = 999, hi = -999;
      for (int j = 0; j < WX_DAYS && c.dhi[j] != WX_NONE; j++) lo = min(lo, (int)c.dlo[j]), hi = max(hi, (int)c.dhi[j]);
      const float span = max(4, hi - lo), bx0 = 262, bx1 = 330;
      wx_deg(wa_day[d][0], 8, c.dlo[d], "--"), wx_deg(wa_day[d][1], 8, c.dhi[d], "--");
      wa_num(P, gl_wx_week, wa_day[d][0], bx0 - 12, y, 2, WA_DIM, ac, v, &card);
      wa_bar(P, bx0, bx1, y, 8, WA_TRACK, ac, v);
      const float f0 = bx0 + (bx1 - bx0) * (c.dlo[d] - lo) / span, f1 = bx0 + (bx1 - bx0) * (c.dhi[d] - lo) / span;
      wa_bar(P, f0, fmaxf(f1, f0 + 8), y, 8, WA_ACC, ac, v);
      wa_num(P, gl_wx_week, wa_day[d][1], WA_X1 - 34, y, 2, WA_TXT, ac, v, &card);
      break;
    }
#endif
#if WX_APP
    case WI_DET: {  // a progress card (Wear OS): the label in the accent, the reading big, a ring at the right
      HaloItem &card = wa_card(P, 0, h, abg, v);
      static const char *const LAB[3] = {"Humidity", "Wind", "UV index"};
      const int u = c.uv10;
      float fr = 0;
      const char *sub = "";
      if (w.arg == 0) snprintf(wa_det[0], 8, "%d%%", c.hum), fr = c.hum / 100.0f;
      else if (w.arg == 1) snprintf(wa_det[1], 8, "%d", c.wind), fr = c.wind / (wx_fahr ? 40.0f : 60.0f), sub = wx_fahr ? "mph" : "km/h";
      else snprintf(wa_det[2], 8, "%d", (u + 5) / 10), fr = u / 110.0f,
           sub = u < 30 ? "Low" : u < 60 ? "Moderate" : u < 80 ? "High" : "Very high";
      const float lx = WA_X0 + 52;
      wa_txt(P, gl_t20, LAB[w.arg], lx, 36, 0, WA_ACC, ac, v, &card);
      wa_num(P, gl_wx_val, wa_det[w.arg], lx, 78, 0, WA_TXT, ac, v, &card);
      if (*sub) wa_txt(P, gl_t20, sub, lx + halo_text_width(gl_wx_val, wa_det[w.arg]) + 10, 82, 0, WA_DIM, ac, v, &card);
      HaloItem &ring = up_item(P);
      ring.kind = HI_RING, ring.x = WA_X1 - 82, ring.y = h / 2, ring.r = 30, ring.w = 9, ring.v = halo_clamp01(fr);
      ring.scale = 1, ring.color = WA_ACC, ring.color2 = WA_TRACK, ring.alpha = ac, ring.on_black = true, ring.on = nullptr;
      up_commit(ring, v);
      break;
    }
    case WI_SUN: {  // the day between sunrise and sunset: an arc, the sun on it now
      wa_card(P, 0, h, abg, v, 52);
      const float ay = 112, r = 64;
      const time_t now = time(nullptr);
      const float fr = halo_clamp01((float)(now - (time_t)c.rise) / (c.set - c.rise));
      for (int j = 0; j < 2; j++) {
        HaloItem &arc = up_item(P);
        arc.kind = HI_ARC, arc.x = CX, arc.y = ay, arc.r = r, arc.w = 6, arc.scale = 1, arc.a0 = -HALO_PI / 2;
        arc.a1 = j ? -HALO_PI / 2 + HALO_PI * fr : HALO_PI / 2, arc.color = j ? WA_ACC : WA_TRACK;
        arc.alpha = j && fr < 0.01f ? 0 : ac, arc.on_black = true, arc.on = nullptr;
        up_commit(arc, v);
      }
      const float an = -HALO_PI / 2 + HALO_PI * fr;
      if (fr > 0 && fr < 1) wa_dot(P, CX + r * sinf(an), ay - r * cosf(an), 11, WA_SUNC, ac, v);
      wa_txt(P, gl_t20, "Sunrise", WA_X0 + 70, 124, 1, WA_DIM, ac, v);
      wa_txt(P, gl_t20, "Sunset", WA_X1 - 70, 124, 1, WA_DIM, ac, v);
      wa_hhmm(wa_rise_s, 8, c.rise), wa_hhmm(wa_set_s, 8, c.set);
      wa_txt(P, gl_t24, wa_rise_s, WA_X0 + 70, 92, 1, WA_TXT, ac, v);
      wa_txt(P, gl_t24, wa_set_s, WA_X1 - 70, 92, 1, WA_TXT, ac, v);
      break;
    }
#endif
    case WI_FOOT: {
      if (wx_fetched_ms) {
        const time_t t = time(nullptr) - (halo_ms() - wx_fetched_ms) / 1000;
        struct tm m;
        localtime_r(&t, &m);
        snprintf(wa_src, sizeof(wa_src), "Open-Meteo \xC2\xB7 %02d:%02d", m.tm_hour, m.tm_min);
      } else {
        strlcpy(wa_src, "Open-Meteo", sizeof(wa_src));
      }
      wa_txt(P, gl_t20, wa_src, CX, h / 2, 1, WA_DIM, ac, v);
      break;
    }
    case WI_SET_UNITS: {  // a card: Units, and the two choices as pills (the picked one filled in the accent)
      HaloItem &card = wa_card(P, 0, h, abg, v, 48);
      wa_txt(P, gl_t20, "Units", CX, 34, 1, WA_DIM, ac, v, &card);
      for (int j = 0; j < 2; j++) {
        const bool on = (j == 1) == wx_fahr;
        const float x0 = j ? CX + 6 : WA_X0 + 34, x1 = j ? WA_X1 - 34 : CX - 6;
        HaloItem &pill = up_item(P);
        ui_rrect(pill, (x0 + x1) / 2, 98, x1 - x0, 72, 36, 1, on ? WA_ACC : WA_TRACK, ac);
        pill.on_black = true;
        up_commit(pill, v);
        wa_txt(P, gl_wx_val, j ? "\xC2\xB0""F" : "\xC2\xB0""C", (x0 + x1) / 2, 98, 1, on ? 0x2A1A00 : WA_TXT, ac, v, &pill);
      }
      break;
    }
    case WI_SET_LOC: {  // the Apps list's card: a tonal disc, the title, the line under it
      HaloItem &card = wa_card(P, 0, h, abg, v);
      const float ix = WA_X0 + 56;
      wa_dot(P, ix, h / 2, 32, WA_ACC, ac, v);
      HaloItem &pin = P.it[P.n - 1];
      HaloItem &g = up_item(P);
      ui_text(g, gl_ic24, WA_IC_LOC, halo_text_mid(gl_ic24, WA_IC_LOC), ix, h / 2, 1, 1, 0x1B1B1F, ac, &pin), up_commit(g, v);
      wa_txt(P, gl_t24, "Location", WA_X0 + 104, 38, 0, WA_TXT, ac, v, &card);
      wa_txt(P, gl_t20, wx_city[0].name, WA_X0 + 104, 72, 0, WA_DIM, ac, v, &card, WA_W - 104 - 40);
      break;
    }
  }
}
static void wa_lay(int, UpPage &, float, const HaloView &) {}
static void wa_tap(int p, float x, float y) {
  const int k = up_list_hit(p, y);
  if (k < 0 || fabsf(x - CX) > WA_W / 2) return;
  const WaItem &w = wa_items[p][k];
  if (w.kind == WI_SET_UNITS) {
    const float top = up_ls[p].fx[k].top, s = up_ls[p].fx[k].s;
    if ((y - top) / s < 56) return;  // (the label row)
    if (x < CX && wx_fahr) sy_set_units(0);
    else if (x >= CX && !wx_fahr) sy_set_units(1);
  } else if (w.kind == WI_SET_LOC) {
    ui_cur = 0, ui_open(&wa_locs);
  }
}
static void wa_upd(int, float) {
  static uint32_t seen = 1;
  if (wx_fetched_ms != seen) seen = wx_fetched_ms, halo_damage_all();  // (fresh weather: every page again)
}
static const UpApp WA_APP = {wa_count, wa_lay, wa_tap, wa_upd, nullptr, WA_ACC, nullptr, true, wa_n_items, wa_item_h, wa_lay_item};
static void wa_build_locs() {
  wa_locs = {"Location", {}, 0, WA_ACC};
  UiPage &l = wa_locs;
  l.rows[l.n++] = ur_info(WA_IC_LOC, "Showing", wa_city);
#if SS_WIFI
  l.rows[l.n++] = ur_action("Use my location", wa_here, WA_ACC);
  l.rows[l.n++] = ur_action("Refresh now", wa_refresh, WA_ACC);
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
