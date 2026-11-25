// muapp.h — Music on the round board (FL_MUSIC): four non-scrolling pages swiped left and right (upager.h; Codex's
// measured study, design/SPEC_music.md — 23 Nov 2026):
//   1 Now playing — the track's progress on the rim, the time played, OF its length, the title, the artist; Previous,
//     Play / Pause (the circle morphs to four lobes while it plays), Next;
//   2 Volume — the speaker's level as ten segments on the rim, the level in %, − and +;
//   3 Queue — the next three tracks in the library's order: a tap plays that one and goes back to Now playing;
//   4 Music settings — Repeat track and Background (kept: player.h), where the library comes from (read only).
// The sound, the tracks and the volume stay player.h's, music.h's (music_sd.h's: a card's) and volume.h's: this file
// only shows them and says what to do. Included from PopkaWatch.ino after upager.h.
#if defined(HALO_ROUND) && HAS_MUSIC
extern const HaloFont gl_mu_track, gl_mu_queue, gl_mu_opt, gl_mu_lab, gl_mu_vol, gl_mu_pct, gl_mu_ic31, gl_mu_ic32,
    gl_mu_ic24, gl_mu_ic26;  // (studio.py GALLERY_FONTS)
// The design's tokens
#define MU_ACC 0xE9C8FC    // orchid
#define MU_INK 0x271632    // ink on orchid
#define MU_TONE 0x31243B   // a tinted control
#define MU_PANEL 0x211827  // a tinted card
#define MU_RING 0x382C43   // the rim's track, a queued badge
#define MU_RAIL 0x4B3C56   // a switch that is off
#define MU_OFF 0.38f       // a control that can do nothing now
#define MU_IC_PREV "\xEF\x81\x88"     // U+F048
#define MU_IC_NEXT "\xEF\x81\x91"     // U+F051
#define MU_IC_MINUS "\xEF\x81\xA8"    // U+F068
#define MU_IC_PLUS "\xEF\x81\xA7"     // U+F067
#define MU_IC_SPEAKER "\xEF\x80\xA8"  // U+F028
#define MU_IC_DRIVE "\xEF\x82\xA0"    // U+F0A0
#define MU_DEG (HALO_PI / 180)
#define MU_ARC_PIECE (12 * MU_DEG)  // the progress arc's pieces: 308° in 26
#define MU_PIECES 26
// The rows of Queue and Music settings: 66 px cards 16 px apart, centred on y 165, 247, 329 (the design's frames; its
// SPEC_music.md text says 74 px queue cards on 165, 250, 335 — the frames win: one rhythm for both pages). A two-line
// card's title has its ink centred 11 px above the row's middle, the line under it 15.5 px below
static const float MU_ROW_Y[3] = {165, 247, 329};

// ---------- Type by its ink: the design centres (or edges) what is drawn, not the line or the advance ----------
struct MuInk {
  float l, r, t, b;  // from the pen's start and from the line's top, px
};
static MuInk mu_ink(const HaloFont &f, const char *s) {
  MuInk k = {1e9f, -1e9f, 1e9f, -1e9f};
  float pen = 0;
  for (const char *q = s; *q;) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(q));
    if (!g) continue;
    if (g->w > 0 && g->h > 0)
      k.l = fminf(k.l, pen + g->ox), k.r = fmaxf(k.r, pen + g->ox + g->w), k.t = fminf(k.t, g->oy), k.b = fmaxf(k.b, g->oy + g->h);
    pen += g->adv;
  }
  if (k.l > k.r) k = {0, 0, 0, 0};
  return k;
}
// A text whose ink is centred on y and, by align (0 — its left edge, 1 — its middle, 2 — its right edge), on x; fit > 0
// — no wider, the rest an ellipsis
static HaloItem &mu_text(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                         const HaloView &v, const HaloItem *on = &halo_black, float fit = 0) {
  const MuInk k = mu_ink(f, s);
  float l = k.l, w = k.r - k.l;
  if (fit > 0 && halo_text_width(f, s) > fit) l = 0, w = halo_text_fit_width(f, s, 0, fit);  // (cut: its advance)
  const float x0 = align == 0 ? x - l : align == 2 ? x - l - w : x - l - w / 2;
  HaloItem &t = up_item(P);
  ui_text(t, f, s, (k.t + k.b) / 2, x0, y, 1, 0, c, a, on, fit);
  t.on_black = on == &halo_black;
  up_commit(t, v);
  return t;
}
static const char *mu_source() {
#if defined(HALO_SD_CS) && !defined(__EMSCRIPTEN__)
  if (mu_sd) return "microSD";  // (music_sd.h: a card with tracks — they are the player's)
#endif
  return "Internal storage";
}

// ---------- State: the primary's morph, the switches, the queue's rows ----------
static float mu_m, mu_m0, mu_m1, mu_mt = 9;  // the morph now, from, to, s since it was asked
static float mu_sw[2];                        // the switches' thumbs 0 (off) … 1 (on)
static int mu_qn, mu_qi[3];                   // the queue's rows: how many, which tracks
static char mu_el[8], mu_of[12], mu_vol_s[4], mu_q[3][2][8];
static float mu_away;  // s on an awake screen without the Music screen laid out
static bool mu_watch;  // the Music screen was up: leaving it pauses (Background off)

static void mu_pick(int i) {  // a queued track: from its beginning, playing
  if (i < 0 || i >= mu_n) return;
  pl_prev = pl_cur, pl_cur = i, pl_change_t = 0, pl_change_dir = 1;
  mu_cmd = i;
  pl_set_playing(true);
}
static void mu_keep(int k) {  // a setting changed: kept
  if (k) pl_bg = !pl_bg, pl_prefs.putBool("bg", pl_bg);
  else pl_repeat = !pl_repeat, pl_prefs.putBool("repeat", pl_repeat);
}

// ---------- The pages ----------
static int mu_count() { return 4; }
static void mu_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (p == 0) {  // Now playing
    up_title(P, "Music", a, v);
    const bool have = mu_n > 0;
    const float fr = have ? halo_clamp01(pl_elapsed / pl_len[pl_cur]) : 0;
    up_arc(P, UP_RING_R, UP_RING_W, UP_A0, UP_A0 + UP_SWEEP, MU_RING, a, v);
    // the progress in fixed 12° pieces (same colour: their round ends meet unseen): as it grows only its last piece is
    // drawn again, and each piece's box keeps to the rim (one arc's box would cover the dial)
    const float end = UP_A0 + UP_SWEEP * fr;
    for (int k = 0; k < MU_PIECES; k++) {
      const float a0 = UP_A0 + k * MU_ARC_PIECE, a1 = fminf(a0 + MU_ARC_PIECE, end);
      up_arc(P, UP_RING_R, UP_RING_W, a0, fmaxf(a0, a1), MU_ACC, have ? a : 0, v);
    }
    if (have) {
      const MuTrack &t = mu_tracks[pl_cur];
      mmss(mu_el, sizeof(mu_el), pl_elapsed);
      mu_text(P, gl_up_disp, mu_el, CX, 178, 1, UP_WHITE, a, v);
      char len[8];
      mmss(len, sizeof(len), pl_len[pl_cur]);
      snprintf(mu_of, sizeof(mu_of), "OF %s", len);
      mu_text(P, gl_up_lab, mu_of, CX, 231, 1, MU_ACC, a, v);
      mu_text(P, gl_mu_track, t.title, CX, 265, 1, UP_WHITE, a, v, &halo_black, 300);
      mu_text(P, gl_mu_lab, t.artist, CX, 297, 1, UP_MUTED, a, v, &halo_black, 250);
    } else {  // (no tracks: where they come from)
      mu_text(P, gl_mu_opt, "No music", CX, 209, 1, UP_WHITE, a, v);
#ifdef HALO_SD_CS
      mu_text(P, gl_mu_lab, "MP3 / WAV in /music on microSD", CX, 265, 1, UP_MUTED, a, v);
#else
      mu_text(P, gl_mu_lab, "tools/halo_music.py --flash", CX, 265, 1, UP_MUTED, a, v);
#endif
    }
    const float ca = have ? a : a * MU_OFF;
    for (int k = 0; k < 2; k++) {
      const float x = k ? 342 : 124;
      HaloItem &d = up_disc(P, x, 365, 34, MU_TONE, ca, v);
      mu_text(P, gl_mu_ic31, k ? MU_IC_NEXT : MU_IC_PREV, x, 365, 1, MU_ACC, ca, v, &d);
    }
    up_primary(P, 233, 365, 52, mu_m, pl_playing ? UP_IC_PAUSE : UP_IC_PLAY, MU_ACC, MU_INK, ca, v);
  } else if (p == 1) {  // Volume
    up_title(P, "Volume", a, v);
    for (int i = 0; i < 10; i++)
      up_arc(P, UP_RING_R, UP_RING_W, (27.8f + i * 30.8f) * MU_DEG, (55 + i * 30.8f) * MU_DEG, i < vol_level ? MU_ACC : MU_RING, a, v);
    // the level, white, its ink centred on y 209; the % in orchid 7 px after it, the bottoms aligned; the pair centred
    snprintf(mu_vol_s, sizeof(mu_vol_s), "%d", vol_level * 10);
    const MuInk d = mu_ink(gl_mu_vol, mu_vol_s), q = mu_ink(gl_mu_pct, "%");
    const float x0 = CX - (d.r - d.l + 7 + q.r - q.l) / 2, bot = 209 + (d.b - d.t) / 2;
    mu_text(P, gl_mu_vol, mu_vol_s, x0, 209, 0, UP_WHITE, a, v);
    mu_text(P, gl_mu_pct, "%", x0 + d.r - d.l + 7, bot - (q.b - q.t) / 2, 0, MU_ACC, a, v);
    // the speaker and SPEAKER on y 288, 8 px apart, the pair centred
    const MuInk s = mu_ink(gl_mu_ic24, MU_IC_SPEAKER), l = mu_ink(gl_up_lab, "SPEAKER");
    const float sx = CX - (s.r - s.l + 8 + l.r - l.l) / 2;
    mu_text(P, gl_mu_ic24, MU_IC_SPEAKER, sx, 288, 0, MU_ACC, a, v);
    mu_text(P, gl_up_lab, "SPEAKER", sx + s.r - s.l + 8, 288, 0, UP_MUTED, a, v);
    for (int k = 0; k < 2; k++) {  // − (tonal) and + (orchid): the one at its end can do nothing
      const float x = k ? 306 : 160, ba = (k ? vol_level < 10 : vol_level > 0) ? a : a * MU_OFF;
      HaloItem &b = up_disc(P, x, 355, 42, k ? MU_ACC : MU_TONE, ba, v);
      mu_text(P, gl_mu_ic32, k ? MU_IC_PLUS : MU_IC_MINUS, x, 355, 1, k ? MU_INK : MU_ACC, ba, v, &b);
    }
  } else if (p == 2) {  // Queue
    up_title(P, "Queue", a, v);
    mu_qn = mu_n > 1 ? min(3, mu_n - 1) : 0;
    for (int k = 0; k < mu_qn; k++) {
      const int i = (pl_cur + 1 + k) % mu_n;
      const float y = MU_ROW_Y[k];
      mu_qi[k] = i;
      up_rrect(P, 77, y - 33, 389, y + 33, 22, k ? MU_PANEL : MU_TONE, a, v);
      HaloItem &card = P.it[P.n - 1];
      HaloItem &badge = up_disc(P, 106, y, 18, k ? MU_RING : MU_ACC, a, v);
      snprintf(mu_q[k][0], 8, "%02d", i + 1);
      mu_text(P, gl_up_lab, mu_q[k][0], 106, y, 1, k ? MU_ACC : MU_INK, a, v, &badge);
      mu_text(P, gl_mu_queue, mu_tracks[i].title, 137, y - 11, 0, UP_WHITE, a, v, &card, 227);
      mu_text(P, gl_mu_lab, mu_tracks[i].artist, 137, y + 15.5f, 0, UP_MUTED, a, v, &card, 177);
      mmss(mu_q[k][1], 8, pl_len[i]);
      mu_text(P, gl_up_lab, mu_q[k][1], 364, y + 15.5f, 2, MU_ACC, a, v, &card);
    }
    if (!mu_qn) mu_text(P, gl_mu_opt, "No upcoming tracks", CX, 233, 1, UP_MUTED, a, v);
  } else {  // Music settings
    up_title(P, "Music settings", a, v);
    for (int k = 0; k < 2; k++) {
      const float y = MU_ROW_Y[k], s = halo_clamp01(mu_sw[k]);
      up_rrect(P, 77, y - 33, 389, y + 33, 22, MU_PANEL, a, v);
      HaloItem &row = P.it[P.n - 1];
      mu_text(P, gl_mu_opt, k ? "Background" : "Repeat track", 101, y, 0, UP_WHITE, a, v, &row);
      up_rrect(P, 305, y - 17, 365, y + 17, 17, ui_mix(MU_RAIL, MU_ACC, s), a, v);
      up_disc(P, 323 + 24 * s, y, 11, ui_mix(UP_MUTED, MU_INK, s), a, v);
    }
    up_rrect(P, 91, 296, 375, 362, 22, MU_PANEL, a, v);
    HaloItem &card = P.it[P.n - 1];
    mu_text(P, gl_mu_ic26, MU_IC_DRIVE, 121, 329, 1, MU_ACC, a, v, &card);
    mu_text(P, gl_mu_opt, "Library source", 149, 317, 0, UP_WHITE, a, v, &card);
    mu_text(P, gl_mu_lab, mu_source(), 149, 344.5f, 0, UP_MUTED, a, v, &card);
  }
}
// Taps by the design's hit boxes (each at least 66 px both ways)
static void mu_tap(int p, float x, float y) {
  if (p == 0) {
    if (!mu_n) return;
    if (up_in(x, y, 90, 331, 158, 399)) pl_change_track(-1);
    else if (up_in(x, y, 181, 313, 285, 417)) pl_set_playing(!pl_playing);
    else if (up_in(x, y, 308, 331, 376, 399)) pl_change_track(1);
  } else if (p == 1) {
    if (up_in(x, y, 118, 313, 202, 397) && vol_level > 0) vol_set(vol_level - 1, false);
    else if (up_in(x, y, 264, 313, 348, 397) && vol_level < 10) vol_set(vol_level + 1, false);
  } else if (p == 2) {
    for (int k = 0; k < mu_qn; k++) {
      const float cy = MU_ROW_Y[k];
      if (up_in(x, y, 77, cy - 33, 389, cy + 33)) return mu_pick(mu_qi[k]), up_go(0);
    }
  } else {
    for (int k = 0; k < 2; k++) {
      const float cy = MU_ROW_Y[k];
      if (up_in(x, y, 77, cy - 33, 389, cy + 33)) mu_keep(k);
    }
  }
}
// Every frame: the primary's morph eases to the state in 180 ms (1 − (1 − u)³); the switches' thumbs slide
static void mu_upd(int, float dt) {
  const float want = pl_playing ? 1 : 0;
  if (want != mu_m1) mu_m0 = mu_m, mu_m1 = want, mu_mt = 0;
  mu_mt = fminf(mu_mt + dt, 9);
  const float u = halo_clamp01(mu_mt / 0.18f), e = 1 - (1 - u) * (1 - u) * (1 - u);
  mu_m = mu_m0 + (mu_m1 - mu_m0) * e;
  const bool on[2] = {pl_repeat, pl_bg};
  for (int k = 0; k < 2; k++) {
    const float to = on[k] ? 1 : 0, step = dt / 0.15f;
    mu_sw[k] += mu_sw[k] < to ? fminf(step, to - mu_sw[k]) : -fminf(step, mu_sw[k] - to);
  }
}
static const UpApp MU_APP = {mu_count, mu_lay, mu_tap, mu_upd, nullptr, MU_ACC};

// Every frame, whatever is on screen (player_tick): Background off — the Music screen gone from an awake screen for
// half a second pauses it. The page has no sound: its position runs by the clock instead, so the pages move as on the
// board (and "muat S" puts it S seconds in — the design's sample is 74 s of 190)
static void mu_tick(float dt) {
  mu_away = pw_mode == PW_ACTIVE ? mu_away + dt : 0;
  if (mu_watch && mu_away > 0.5f) {
    mu_watch = false;
    if (!pl_bg && pl_playing) pl_set_playing(false);
  }
#ifdef __EMSCRIPTEN__
  static bool cmd;
  if (!cmd) cmd = true, halo_command("muat", [](const char *a) { mu_cur = pl_cur, mu_pos = (uint32_t)(atof(a) * mu_rate); });
  if (mu_cmd >= 0) mu_cur = mu_cmd, mu_cmd = -1, mu_pos = 0;
  if (mu_play && mu_cur >= 0 && mu_cur < mu_n) {
    mu_pos += (uint32_t)(dt * mu_rate);
    if (mu_pos >= mu_tracks[mu_cur].samples) mu_ended = true;
  }
#endif
}

// The screen (settings.h calls these)
static void player_enter() {
  mu_m = mu_m0 = mu_m1 = pl_playing ? 1 : 0, mu_mt = 9;
  mu_sw[0] = pl_repeat, mu_sw[1] = pl_bg;
  up_open(&MU_APP);
}
static void player_update(float dt) {
  pl_active = true;
  up_update(dt);
}
static void player_layout(const HaloView &v) {
  mu_away = 0, mu_watch = true;  // (on screen: Background off pauses once it is gone)
  up_layout(v);
}
static void player_draw() { up_draw(); }
#endif
