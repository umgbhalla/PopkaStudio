// shade_round.h — the quick settings panel of the round board's home (PopkaStudio: App → Shade): Google's Wear OS 6
// pull-down, its content three fixed pages swiped left and right (qsapp.h on upager.h; Codex's measured study,
// design/SPEC_quick_settings.md — 23 Nov 2026): Controls, Levels, Shade settings.
//
// A layer of the home (FL_SHADE) turns the panel on. A drag down from the top edge pulls it out: the panel is a black
// disc the size of the screen sliding down from above under the finger (its lower edge is an arc; the face under it
// dims by 35 %). Let go past a third of the way or flicked faster than 700 px/s — it goes on to the end, else back:
// the rest by time (260 ms open, 220 ms back, scaled by what is left) along Material's standard curve. Up — closes.
// Open, a drag down once more — to the watch face (from any screen): the panel slides up off the face under it.
// Open, a drag sideways turns its pages (the curved time and the dots stay); a tap on a page is the page's (qsapp.h).
//
// What works: Brightness — Display's (settings.h: its 8 steps, saved; without Display the panel keeps its own under the
// same flash key); Volume and Mute — the speaker's (volume.h, brought in with the panel); Wi-Fi — the project's (a tap:
// the time and the weather now; without Wi-Fi in the firmware — Off); Always On — on/off now (not saved); Lock and Screen
// off — the Buttons' actions (power.h); the battery — the power chip's (read from loop() every 5 s).
//
// The editor: a shade layer on any screen but the home is drawn open and still (the tile pictures); the command shade
// open|close shows it on the home (the map, while the layer is selected), shade alone — open or close; shade wifi on
// NAME | off — the project's Wi-Fi (the page sends it).
// Included from face.h on the round board in a firmware with the layer (FN_SHADE), and in that board's page.

#define SH_TRAVEL 466.0f  // the finger's way = the disc's: the whole screen
#define SH_MAX 4          // items in the pool (the disc)
#define SH_SEGS 8
enum : uint8_t { SI_WIFI, SI_BRIGHT, SI_AOD, SI_OFF, SI_VOL, SI_LOCK, SI_N };

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
// qsapp.h's: the pages (after upager.h)
static void qs_begin();
static void qs_open_page();
static void qs_tick(float dt, bool tap, float x, float y);
static void qs_layout(const HaloView &v, float ty, bool shown);
static void qs_draw();

static HaloItem *sh_it;           // the pool (PSRAM): laid out bottom to top every frame
static int sh_n, sh_n_was;
static float sh_px;               // the live panel: how far it is out (finger travel, px)
static float sh_px0, sh_y0;       // the drag: where it started
static bool sh_drag;
static float sh_tw_from, sh_tw_to = -1, sh_tw_t, sh_tw_dur;  // the tween after the finger (to < 0 — none)
static uint8_t sh_owner;
static int sh_last_volume = 6;
static float sh_cover_y = -1;     // the rows above it are all under the disc (it is opaque): the face isn't drawn there
static HaloEase sh_ease;
static HaloSpringK K_SH_MORPH;
static HaloSpring sh_on[SI_N];    // a tile's shape: 0 — off, 1 — on (Always On: mint, less rounded)
static volatile int sh_cmd = -1;  // shade open|close (1, 0), shade alone (2)
static volatile int sh_bat = -1;  // the battery's percent (−1 — not known)

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

// ---------- The pool: the disc, through its move and the screen's view ----------
static HaloView sh_v;
static float sh_ty;
static HaloItem *sh_take() {
  if (!sh_it || sh_n >= SH_MAX) return nullptr;
  HaloItem *it = &sh_it[sh_n++];
  it->on = nullptr, it->on_black = false, it->scale = 1, it->align = 1, it->spacing = 0, it->text = nullptr;
  return it;
}
static void sh_circle(float cx, float cy, float r, uint32_t c, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_CIRCLE, it->x = cx, it->y = cy + sh_ty, it->r = r, it->color = c, it->alpha = a;
  if (it->alpha < 0.999f) halo_subpixel = true;
  halo_view_commit(*it, sh_v);
}

// ---------- What the tiles show and do ----------
static bool sh_state(uint8_t what) {
  switch (what) {
    case SI_WIFI: return sh_wifi;
    case SI_AOD: return shade_aod_is_on();
    case SI_BRIGHT: return sh_bright() >= 5;
    case SI_VOL: return vol_level == 0;
  }
  return false;
}
static void sh_volume(int v) {  // the Levels' steps: the last audible level kept for the tile's restore
  v = constrain(v, 0, 10);
  if (v > 0) sh_last_volume = v;
  vol_set(v, false);
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

// Its items, at the end of screen s's layout: the disc, then the pages on it (qsapp.h)
static void shade_layout(int s, const FaceScreen &S, int i, const HaloView &v) {
  const float p = i >= 0 ? shade_p(s) : 0;
  sh_n = 0, sh_v = v, sh_ty = 0;
  sh_cover_y = -1;
  const bool shown = p > 0 && v.a > 0;
  if (shown) {
#ifdef __EMSCRIPTEN__
    if (sh_bat < 0) sh_bat = halo_battery().percent;  // (the page: the desk board's, full)
#endif
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
  }
  qs_layout(v, sh_ty, shown);
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
  if (sh_n) qs_draw();
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
  for (int i = 0; i < SI_N; i++) halo_spring_set(sh_on[i], sh_state(i) ? 1 : 0);
  sh_bright_load();
  vol_build();
  qs_begin();
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
}

static void shade_close() {
  if (sh_px > 0) halo_damage_all();
  sh_px = 0, sh_tw_to = -1, sh_cmd = -1;
  sh_drag = false;
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
    if (open && sh_px == 0 && i >= 0) halo_heavy(), qs_open_page();
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
  sh_bright_tick(dt);
}

static void sh_act(uint8_t what) {
  switch (what) {
    case SI_AOD: shade_power(what); break;
    case SI_WIFI: if (sh_wifi) shade_power(what); break;  // (no Wi-Fi in the firmware: the tile is off, nothing to do)
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

// Handle the shade before navigation: the panel's finger is its own (PopkaWatch.ino hides it from the screens)
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
  // Open, sideways — the pages' (upager.h claims it in qs_tick); from the left edge — nobody's
  if (open && mine && f.drag_start && f.horiz && f.edge != HALO_EDGE_LEFT) f.owner = nullptr;
  if (f.drag_start && !f.horiz && !sh_drag) {
    const bool opening = sh_px == 0 && f.edge == HALO_EDGE_TOP && f.y > f.y0 && halo_finger_claim(&sh_owner);
    if (mine || opening) {
      if (opening) halo_heavy(), qs_open_page();
      sh_drag = true, sh_px0 = sh_px, sh_y0 = f.y, sh_tw_to = -1;
    }
  }
  bool tap = false;
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
  } else {
    tap = f.released && mine && f.tap && open;
  }
  if (sh_px > 0) qs_tick(dt, tap, f.x, f.y);
  shade_anim(dt);
}
