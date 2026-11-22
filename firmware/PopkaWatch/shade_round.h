// shade_round.h — the quick settings panel of the round board's home (PopkaStudio: App → Shade, "Pills"): variant
// 23 of web/mockups/round-quick3.html, picked by the user (11 Nov 2026), made after Google's own Wear OS 6 quick
// settings (Material 3 Expressive on the Pixel Watch 3 and 4: Google's launch GIF and 9to5Google's screenshots, 2025).
//
// A layer of the home (FL_SHADE) configures the global panel. A drag down from the top edge pulls it out: the panel is a
// black disc the size of the screen sliding down from above under the finger (its lower edge is an arc; the face under
// it dims by 35 %). Let go past a third of the way or flicked faster than 700 px/s — it goes on to the end, else back:
// the rest by time (260 ms open, 220 ms back, scaled by what is left) along Material's standard curve. Up — closes.
// Open, a drag down once more — to the watch face (from any screen): the panel slides up off the face under it.
//
// Two rows of three buttons: Wi-Fi, Brightness, Always On / Screen off, Volume, Lock. Volume taps mute or restore
// the last audible level. Brightness taps cycle through three presets. Holding either control opens a temporary
// slider on its row. The same finger can then adjust the level horizontally, and release closes the slider.
// The colours: the layer's color is the accent (Wear's theme); the buttons' container — the accent deep in black, as in
// Wear's dark scheme; the icons on it — white touched by the accent.
//
// What works — as on the rectangle (shade_rect.h): Brightness — Display's (settings.h: its 8 steps, saved; without
// Display the panel keeps its own under the same flash key); Volume and Mute — the speaker's (volume.h, brought in with
// the panel); Wi-Fi — the project's (a tap: the time and the weather now; without Wi-Fi in the firmware — off); Always On
// — on/off now (not saved); Lock and Screen off — the Buttons' actions (power.h); the battery — the power chip's (read
// from loop() every 5 s).
//
// The editor: a shade layer on any screen but the home is drawn open and still (the tile pictures); the command shade
// open|close shows it on the home (the map, while the layer is selected), shade alone — open or close; shade wifi on
// NAME | off — the project's Wi-Fi (the page sends it).
// Included from face.h on the round board in a firmware with the layer (FN_SHADE), and in that board's page.

#define SH_TRAVEL 466.0f  // the finger's way = the disc's: the whole screen
#define SH_MAX 40         // items in the pool
#define SH_BW 122.0f      // a button: Google's grid — 3 × 122 + 2 × 8, rows of 102
#define SH_BH 102.0f
#define SH_SEGS 8
static constexpr float SH_ROW_TOP = 116, SH_ROW_STEP = 110, SH_SLIDE_PX = 140;
enum : uint8_t { SI_WIFI, SI_BRIGHT, SI_AOD, SI_OFF, SI_VOL, SI_LOCK, SI_N, SI_MUTED = SI_N };
static const char *const SI_ICON[] = {"\xEF\x87\xAB", "\xEF\x86\x85", "\xEF\x80\x97", "\xEF\x80\x91",
                                     "\xEF\x80\xA8", "\xEF\x80\xA3", "\xEF\x9A\xA9"};
static const char SH_IC_BAT[] = "\xEF\x89\x80", SH_IC_WIFI[] = "\xEF\x87\xAB";
extern const HaloFont gl_shr_ic30, gl_shr_ic20, gl_shr_n18;

#define SH_VOL 1  // the speaker's volume (volume.h: settings.h brings it in with the panel)
// Wi-Fi: on the board — the firmware's own (FN_WIFI); in the editor's page — the project's, sent by the page
#ifdef __EMSCRIPTEN__
static bool sh_wifi;
#else
static const bool sh_wifi = FN_WIFI;
#endif

// power.h's: Always On on/off, the Lock and Screen off actions, Wi-Fi's "now" (defined there)
static bool shade_aod_is_on();
static void shade_power(int what);

static HaloItem *sh_it;           // the pool (PSRAM): laid out bottom to top every frame
static int sh_n, sh_n_was;
static float sh_px;               // the live panel: how far it is out (finger travel, px)
static float sh_px0, sh_y0;       // the drag: where it started
static bool sh_drag;
static float sh_tw_from, sh_tw_to = -1, sh_tw_t, sh_tw_dur;  // the tween after the finger (to < 0 — none)
static uint8_t sh_owner, sh_armed = SI_N, sh_expanded = SI_N;
static HaloSpring sh_expand;
static HaloSpringK K_SH_EXPAND;
static float sh_hold_x;
static int sh_hold_value, sh_last_volume = 6;
static float sh_cover_y = -1;     // the rows above it are all under the disc (it is opaque): the face isn't drawn there
static HaloEase sh_ease;
static HaloSpringK K_SH_MORPH;
static HaloSpring sh_on[SI_N];    // a button's shape: 0 — off (a pill), 1 — on (a rounded square in the accent)
static float sh_mid_ic[SI_N + 1], sh_mid_chip[2], sh_mid_n18;
static volatile int sh_cmd = -1;  // shade open|close (1, 0), shade alone (2)
static volatile int sh_bat = -1;  // the battery's percent (−1 — not known)
static char sh_pct[8];
struct ShHit {
  float x0, y0, x1, y1;
  uint8_t what;
};
static ShHit sh_hits[SI_N];
static int sh_hit_n;

// Brightness: Display's steps when the firmware has Display, else the panel's own (the same table and flash key)
#if HAS_DISPLAY
static int sh_bright() { return dp_value; }
static void sh_bright_set(int v) { dp_set(v); }
static void sh_bright_tick(float) {}
static void sh_bright_load() {}
static void sh_bright_save() {}
#else
static const uint8_t SH_LEVEL[SH_SEGS + 1] = {24, 34, 48, 66, 90, 118, 150, 180, 255};  // Display's (settings.h)
static volatile int sh_bv = 7;  // = 180, HaloWatch's
static float sh_bshown = 7, sh_bfrom = 7, sh_bt = 9;
static Preferences sh_prefs;
static int sh_bsaved = -1;
static uint32_t sh_bchanged;
static int sh_bright() { return sh_bv; }
static void sh_bright_set(int v) {
  v = constrain(v, 0, SH_SEGS);
  if (v == sh_bv) return;
  sh_bfrom = sh_bshown, sh_bt = 0, sh_bv = v, sh_bchanged = halo_ms();
}
static void sh_bright_tick(float dt) {  // 150 ms to the new step, as Display's
  sh_bt = fminf(sh_bt + dt, 9);
  const float k = halo_clamp01(sh_bt / 0.15f);
  sh_bshown = sh_bfrom + (sh_bv - sh_bfrom) * (1 - (1 - k) * (1 - k));
  const float s = halo_clamp01(sh_bshown / SH_SEGS) * SH_SEGS;
  const int i = min((int)s, SH_SEGS - 1);
  pw_awake_level = (int)lroundf(SH_LEVEL[i] + (SH_LEVEL[i + 1] - SH_LEVEL[i]) * (s - i));
}
static void sh_bright_load() {  // before halo_begin: the panel starts at it
  sh_prefs.begin("popkawatch", false);
  sh_bv = sh_bsaved = constrain((int)sh_prefs.getUChar("bright", 7), 0, SH_SEGS);
  sh_bshown = sh_bfrom = sh_bv;
  pw_awake_level = SH_LEVEL[sh_bv];
}
static void sh_bright_save() {  // into flash a second after it settles
  if (sh_bv != sh_bsaved && halo_ms() - sh_bchanged > 1000) sh_bsaved = sh_bv, sh_prefs.putUChar("bright", (uint8_t)sh_bv);
}
#endif
static void shade_poll() {  // loop(): the panel's own brightness into flash; the battery every 5 s (the I2C bus — here)
  sh_bright_save();
  static uint32_t last;
  if (sh_bat >= 0 && halo_ms() - last < 5000) return;
  last = halo_ms();
  const HaloBattery b = halo_battery();
  sh_bat = b.ok && b.present ? b.percent : -1;
}

// ---------- Colors ----------
static uint32_t sh_mix(uint32_t a, uint32_t b, float t) {
  t = halo_clamp01(t);
  uint32_t out = 0;
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)lroundf(ca + (cb - ca) * t) << s;
  }
  return out;
}
static uint32_t sh_on_accent(uint32_t c) {  // the icon on the accent: the accent's own hue, deep
  const int r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255, m = min(r, min(g, b)) * 7 / 10;
  return (uint32_t)((r - m) * 45 / 100) << 16 | (uint32_t)((g - m) * 45 / 100) << 8 | (uint32_t)((b - m) * 45 / 100);
}

// ---------- The pool: items through a move (the disc's) and a fade, then the screen's view ----------
static HaloView sh_v;
static float sh_ty;
static HaloItem *sh_take() {
  if (!sh_it || sh_n >= SH_MAX) return nullptr;
  HaloItem *it = &sh_it[sh_n++];
  it->on = nullptr, it->on_black = false, it->scale = 1, it->align = 1, it->spacing = 0, it->text = nullptr;
  return it;
}
static void sh_commit(HaloItem *it) {
  it->y += sh_ty;
  if (it->alpha < 0.999f) halo_subpixel = true;
  halo_view_commit(*it, sh_v);
}
static void sh_rrect(float x, float y, float w, float h, float r, uint32_t c, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_RRECT, it->x = x + w / 2, it->y = y + h / 2, it->w = w, it->h = h, it->r = fminf(r, fminf(w, h) / 2);
  it->color = c, it->alpha = w > 0.5f && h > 0.5f ? a : 0;
  sh_commit(it);
}
static void sh_circle(float cx, float cy, float r, uint32_t c, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_CIRCLE, it->x = cx, it->y = cy, it->r = r, it->color = c, it->alpha = a;
  sh_commit(it);
}
// Text: x by align (0 — left edge, 1 — middle), y — the middle of its letters (mid: the font's)
static void sh_text(const HaloFont &f, float mid, const char *s, float x, float y, uint32_t c, int align = 1, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_TEXT, it->font = &f, it->text = s, it->align = align, it->color = c, it->alpha = s && *s ? a : 0;
  it->x = x, it->y = y + f.line / 2.0f - mid;
  sh_commit(it);
}
static void sh_hit(float x0, float y0, float x1, float y1, uint8_t what, bool live) {
  if (live && sh_hit_n < (int)(sizeof(sh_hits) / sizeof(sh_hits[0]))) sh_hits[sh_hit_n++] = {x0, y0 + sh_ty, x1, y1 + sh_ty, what};
}

// ---------- The parts ----------
static bool sh_control(uint8_t what) { return what == SI_BRIGHT || what == SI_VOL; }
static int sh_value(uint8_t what) { return what == SI_VOL ? vol_level : sh_bright(); }
static int sh_max(uint8_t what) { return what == SI_VOL ? 10 : SH_SEGS; }
static int sh_icon(uint8_t what) { return what == SI_VOL && vol_level == 0 ? (int)SI_MUTED : what; }

static bool sh_state(uint8_t what) {
  switch (what) {
    case SI_WIFI: return sh_wifi;
    case SI_AOD: return shade_aod_is_on();
    case SI_BRIGHT: return sh_bright() >= 5;
    case SI_VOL: return vol_level == 0;
  }
  return false;
}

static void sh_button(uint8_t what, float x, float y, uint32_t acc, uint32_t on, uint32_t box, uint32_t ink, bool live, float a) {
  const float s = sh_on[what].x, k = halo_clamp01(s);
  sh_rrect(x, y, SH_BW, SH_BH, SH_BH / 2 + (SH_BH * 0.27f - SH_BH / 2) * s, sh_mix(box, acc, k), a);
  const int ic = sh_icon(what);
  sh_text(gl_shr_ic30, sh_mid_ic[ic], SI_ICON[ic], x + SH_BW / 2, y + SH_BH / 2, sh_mix(ink, on, k), 1, a);
  sh_hit(x, y, x + SH_BW, y + SH_BH, what, live);
}

static void sh_slider_put(uint8_t what, float k, uint32_t acc, uint32_t box, uint32_t ink) {
  const float y = SH_ROW_TOP + (what == SI_VOL ? SH_ROW_STEP : 0), cy = y + SH_BH / 2;
  const float w = SH_BW + (340 - SH_BW) * k, x = CX - w / 2;
  sh_rrect(x, y, w, SH_BH, SH_BH / 2, box, k);
  const int ic = sh_icon(what);
  sh_text(gl_shr_ic30, sh_mid_ic[ic], SI_ICON[ic], x + 35, cy, ink, 1, k);
  const float left = x + 70, track = w - 99;
  const float level = track * sh_value(what) / sh_max(what);
  sh_rrect(left, cy - 7, track, 14, 7, sh_mix(box, ink, 0.22f), k);
  sh_rrect(left, cy - 7, level, 14, 7, acc, k);
  sh_circle(left + level, cy, 11, acc, k);
}
// The status chip: the battery's percent and Wi-Fi
static void sh_chip(uint32_t box, uint32_t ink) {
  const bool bat = sh_bat >= 0;
  if (bat) snprintf(sh_pct, sizeof(sh_pct), "%d%%", (int)sh_bat);
  const float cy = 387, bw = bat ? halo_text_width(gl_shr_ic20, SH_IC_BAT) : 0, tw = bat ? halo_text_width(gl_shr_n18, sh_pct) : 0;
  const float ww = sh_wifi ? halo_text_width(gl_shr_ic20, SH_IC_WIFI) : 0;
  const float all = (bat ? bw + 7 + tw : 0) + (bat && sh_wifi ? 10 : 0) + ww;
  sh_rrect(158, cy - 23, 150, 46, 23, box, all > 0 ? 1 : 0);
  float x = CX - all / 2;
  if (bat) {
    sh_text(gl_shr_ic20, sh_mid_chip[0], SH_IC_BAT, x, cy, ink, 0), x += bw + 7;
    sh_text(gl_shr_n18, sh_mid_n18, sh_pct, x, cy, ink, 0), x += tw + 10;
  }
  if (sh_wifi) sh_text(gl_shr_ic20, sh_mid_chip[1], SH_IC_WIFI, x, cy, ink, 0);
}

// ---------- The layer ----------
static int shade_index(const FaceScreen &S) {
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_SHADE && !S.layers[i].hidden) return i;
  return -1;
}
// How far the panel of screen s is out: the home — the live one; any other screen — open (the editor's tile pictures)
static float shade_p(int s) {
  return s == 0 ? halo_clamp01(sh_px / SH_TRAVEL) : 1;
}
// The view the rest of screen s is laid out through: dimmed under the disc
static HaloView shade_under(int s, const FaceScreen &S, const HaloView &v, bool preview) {
  const float p = preview && shade_index(S) < 0 ? 0 : shade_p(preview ? s : 0);
  if (p <= 0) return v;
  HaloView u = v;
  u.a *= 1 - 0.35f * p;
  return u;
}
static float shade_top_fade(int, const FaceScreen &, int) {
  return 1;
}

// Its items, at the end of screen s's layout
static void shade_layout(int s, const FaceScreen &S, int i, const HaloView &v) {
  const bool live = s == 0;
  const float p = i >= 0 ? shade_p(s) : 0;
  sh_n = 0, sh_v = v, sh_ty = 0;
  if (live) sh_hit_n = 0;
  sh_cover_y = -1;
  if (p > 0 && v.a > 0) {
#ifdef __EMSCRIPTEN__
    if (sh_bat < 0) sh_bat = halo_battery().percent;  // (the page: the desk board's, full)
#endif
    const uint32_t acc = S.layers[i].color, on = sh_on_accent(acc), box = sh_mix(0x0D0D0D, acc, 0.21f),
                   ink = sh_mix(0xFFFFFF, acc, 0.08f);
    sh_ty = -(1 - p) * SH_TRAVEL;
    sh_circle(CX, CY, CX + 1, 0x000000);  // the disc: its lower edge is an arc while it comes
    // Where it covers the round screen whole: a row is under it while its chord of the disc holds the screen's chord —
    // with both circles on the same middle line that is every row above (screen + disc)/2 + (rd² − r²) / 2a, a — how
    // far the disc's middle is above the screen's (the slide redraws the whole screen each frame: the face under the
    // disc was half of it)
    if (v.a >= 0.999f && fabsf(v.x) < 0.01f) {
      const float dy = halo_view_y(v, CY + sh_ty), rd = (CX + 1) * v.s, a = CY - dy;
      if (a > 0.5f) sh_cover_y = (CY + dy) / 2 + (rd * rd - CX * CX) / (2 * a);
      else if (a > -0.5f && rd >= CX + 0.5f) sh_cover_y = HALO_H;
    }
    const float expanded = live ? halo_clamp01(sh_expand.x) : 0;
    static const uint8_t GRID[] = {SI_WIFI, SI_BRIGHT, SI_AOD, SI_OFF, SI_VOL, SI_LOCK};
    for (int k = 0; k < SI_N; k++) {
      const int row = k / 3;
      const bool fading = expanded > 0 && row == (sh_expanded == SI_VOL ? 1 : 0);
      sh_button(GRID[k], 42 + (k % 3) * 130, SH_ROW_TOP + row * SH_ROW_STEP, acc, on, box, ink, live, fading ? 1 - expanded : 1);
    }
    if (expanded > 0) sh_slider_put(sh_expanded, expanded, acc, box, ink);
    sh_chip(box, ink);
  }
  for (int k = sh_n; k < sh_n_was; k++) sh_it[k].alpha = 0, halo_item_commit(sh_it[k]);
  sh_n_was = sh_n;
  if (i >= 0) {
    FaceRun &R = S.run[i];
    R.box[0] = 0, R.box[1] = 0, R.box[2] = p > 0 ? HALO_W : -1, R.box[3] = p > 0 ? HALO_H : -1;
    R.ax = CX, R.ay = CY;
  }
}

// face.h, before the face's layers: this band without the rows under the disc (2 px for its soft edge); false — none left
static bool shade_cut_under() {
  if (sh_cover_y < 0) return true;
  const int y = (int)sh_cover_y - 2;
  if (y <= hb.y) return true;
  if (y >= hb.y + hb.rows) return false;
  const int k = y - hb.y;
  hb.buf += k * hb.w, hb.y += k, hb.rows -= k;
  return true;
}

static void shade_draw() {
  for (int k = 0; k < sh_n; k++) halo_item_draw(sh_it[k]);
}

static void shade_overlay_draw() {
  if (!sh_n) return halo_nav_draw();
  const HaloBand original = hb;
  if (shade_cut_under()) halo_nav_draw();
  hb = original;
  shade_draw();
}

static void shade_layer_build() {  // (face_build: once)
  static bool built;
  if (built) return;
  built = true;
#ifdef __EMSCRIPTEN__
  sh_it = new HaloItem[SH_MAX]();
#else
  sh_it = (HaloItem *)heap_caps_calloc(SH_MAX, sizeof(HaloItem), MALLOC_CAP_SPIRAM);
  if (!sh_it) sh_it = (HaloItem *)calloc(SH_MAX, sizeof(HaloItem));
  for (int i = 0; i < SH_MAX; i++) new (sh_it + i) HaloItem();
#endif
  halo_ease_make(sh_ease, 0.2f, 0, 0, 1);  // Material standard
  K_SH_MORPH = halo_spring(0.3f, 0.8f);    // (the shape: a little past and back, as Wear OS 6's morph)
  K_SH_EXPAND = halo_spring(0.18f, 1);
  for (int i = 0; i <= SI_N; i++) sh_mid_ic[i] = halo_text_mid(gl_shr_ic30, SI_ICON[i]);
  sh_mid_chip[0] = halo_text_mid(gl_shr_ic20, SH_IC_BAT), sh_mid_chip[1] = halo_text_mid(gl_shr_ic20, SH_IC_WIFI);
  sh_mid_n18 = halo_text_mid(gl_shr_n18, "0");
  for (int i = 0; i < SI_N; i++) halo_spring_set(sh_on[i], sh_state(i) ? 1 : 0);
  sh_bright_load();
  vol_build();
  halo_command("shade", [](const char *a) {  // shade open | close; alone — the other way; shade wifi on NAME | off (the page)
#ifdef __EMSCRIPTEN__
    if (!strncmp(a, "wifi", 4)) {
      a += 4;
      while (*a == ' ') a++;
      sh_wifi = !strncmp(a, "on", 2);
      return;
    }
#endif
    if (!strncmp(a, "look", 4)) return;  // (one look on the round board)
    sh_cmd = !strncmp(a, "open", 4) ? 1 : !strncmp(a, "close", 5) ? 0 : 2;
  });
}

// ---------- Every frame ----------
static void sh_tween(float to, float ms) {
  const float left = fabsf(to - sh_px) / SH_TRAVEL;
  sh_tw_from = sh_px, sh_tw_to = to, sh_tw_t = 0, sh_tw_dur = ms / 1000 * fmaxf(0.45f, left);
  if (to < sh_px) sh_armed = SI_N, sh_expand.target = 0;
}

static void shade_close() {
  if (sh_px > 0) halo_damage_all();
  sh_px = 0, sh_tw_to = -1, sh_cmd = -1;
  sh_drag = false;
  sh_armed = sh_expanded = SI_N;
  halo_spring_set(sh_expand, 0);
}

static void shade_dismiss() {
  if (sh_px <= 0) return shade_close();
  sh_drag = false;
  sh_cmd = -1;
  sh_tween(0, 176);
}
// The tween, the shapes, the brightness — also in the editor's still copies (no finger there)
static void shade_anim(float dt) {
  const int i = shade_index(face_scr[0]);
  if (sh_cmd >= 0) {
    const bool open = sh_cmd == 2 ? sh_px < 1 : sh_cmd == 1;
    if (open && sh_px == 0 && i >= 0) halo_heavy();
    if (i >= 0) sh_tween(open ? SH_TRAVEL : 0, open ? 260 : 220);
    sh_cmd = -1;
  }
  if (i < 0 && sh_px > 0) shade_close();
  if (sh_tw_to >= 0 && !sh_drag) {
    sh_tw_t += dt;
    const float k = sh_tw_dur > 0 ? halo_clamp01(sh_tw_t / sh_tw_dur) : 1;
    sh_px = sh_tw_from + (sh_tw_to - sh_tw_from) * halo_ease(sh_ease, k);
    if (k >= 1) sh_px = sh_tw_to, sh_tw_to = -1;
  }
  for (int w = 0; w < SI_N; w++) {
    sh_on[w].target = sh_state(w) ? 1 : 0;
    halo_spring_run(sh_on[w], K_SH_MORPH, dt);
  }
  halo_spring_run(sh_expand, K_SH_EXPAND, dt);
  if (sh_expand.x <= 0 && sh_expand.target == 0) sh_expanded = SI_N;
  sh_bright_tick(dt);
}

static void sh_act(uint8_t what) {
  switch (what) {
    case SI_AOD: shade_power(what); break;
    case SI_WIFI: if (sh_wifi) shade_power(what); break;  // (no Wi-Fi in the firmware: the button is off, nothing to do)
    case SI_LOCK: case SI_OFF:
      shade_close();
      shade_power(what);
      break;
    case SI_VOL:
      if (vol_level > 0) sh_last_volume = vol_level, vol_set(0, false);
      else vol_set(sh_last_volume, false);
      break;
    case SI_BRIGHT: {
      const int v = sh_bright();
      sh_bright_set(v < 2 || v >= 8 ? 2 : v < 5 ? 5 : 8);
      break;
    }
  }
}

static uint8_t sh_hit_at(float x, float y) {
  for (int k = 0; k < sh_hit_n; k++) {
    const ShHit &h = sh_hits[k];
    if (x >= h.x0 && x <= h.x1 && y >= h.y0 && y <= h.y1) return h.what;
  }
  return SI_N;
}

// Handle the shade before navigation so a held slider keeps ownership of the same finger.
static void shade_tick(float dt, bool awake) {
  const int i = shade_index(face_scr[0]);
  HaloFinger &f = halo_finger;
  if (i < 0 || !awake) {
    shade_close();
    return shade_anim(dt);
  }
  const float T = SH_TRAVEL;
  const bool open = sh_px >= T - 0.5f && sh_tw_to < 0;
  if (f.pressed && sh_px > 0.5f) halo_finger_claim(&sh_owner);
  const bool mine = f.owner == &sh_owner;
  if (f.pressed) sh_armed = open && mine ? sh_hit_at(f.x0, f.y0) : (uint8_t)SI_N;
  if (open && mine && f.down && sh_control(sh_armed) && !f.dragging && halo_ms() - f.t0 >= 400 && sh_expand.target == 0) {
    sh_expanded = sh_armed;
    sh_expand.target = 1;
    sh_hold_x = f.x;
    sh_hold_value = sh_value(sh_expanded);
    if (sh_expanded == SI_VOL && sh_hold_value > 0) sh_last_volume = sh_hold_value;
  }
  if (sh_expand.target > 0) {
    if (mine && (f.down || f.released)) {
      const int cap = sh_max(sh_expanded);
      const int value = constrain(sh_hold_value + (int)lroundf((f.x - sh_hold_x) * cap / SH_SLIDE_PX), 0, cap);
      if (value != sh_value(sh_expanded)) {
        if (sh_expanded == SI_VOL) {
          if (value > 0) sh_last_volume = value;
          vol_set(value, false);
        } else {
          sh_bright_set(value);
        }
      }
    }
    if (!f.down || !mine) sh_armed = SI_N, sh_expand.target = 0;
    return shade_anim(dt);
  }
  if (f.drag_start && !f.horiz && !sh_drag) {
    const bool opening = sh_px == 0 && f.edge == HALO_EDGE_TOP && f.y > f.y0 && halo_finger_claim(&sh_owner);
    if (mine || opening) {
      if (opening) halo_heavy();
      sh_drag = true, sh_px0 = sh_px, sh_y0 = f.y, sh_tw_to = -1;
    }
  }
  if (sh_drag) {
    if (f.down && f.owner == &sh_owner) {
      sh_px = halo_clamp01((sh_px0 + f.y - sh_y0) / T) * T;
    } else {
      sh_drag = false;
      const bool from_open = sh_px0 > T / 2;
      // Open, and pulled down once more (it has no further to go): to the clock — the panel slides up off the face
      // under it (the user's way: swipe down — the quick settings, swipe down again — the watch face)
      if (from_open && sh_px >= T - 0.5f && (f.y - sh_y0 > 60 || f.vy > 700)) {
        sh_tween(0, 260);
        if (!halo_nav_on_face()) halo_nav_home_now();
        halo_damage_all();
        return shade_anim(dt);
      }
      const bool go = f.vy > 700 || (f.vy > -700 && (from_open ? sh_px > T * 2 / 3 : sh_px > T / 3));
      sh_tween(go ? T : 0, go ? 260 : 220);
    }
  } else if (f.released && mine && f.tap && open && sh_hit_at(f.x, f.y) == sh_armed) {
    sh_act(sh_armed);
  }
  if (f.released) sh_armed = SI_N;
  shade_anim(dt);
}
