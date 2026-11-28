// radioapp.h — the Radio (FL_RADIO) as four non-scrolling pages swiped left and right (upager.h; Codex's measured
// study, design/SPEC_radio.md — 23 Nov 2026), the round board's:
//   1 Station — the poster (the monogram between paired level bars, the name, LIVE or READY and the bitrate or the
//     song), Previous, Play / Pause (the primary: a circle, four soft lobes while it plays), Next;
//   2 Stations — three rows a view: the dial's stations (the folder the swipes and Previous / Next go through), the
//     catalog's folders (the folder button) and a folder's stations; ‹ › turn the slice, the count under them;
//     a tap tunes, a long press is the heart (a favourite); Back (the middle button in a folder) goes one level up;
//   3 Volume — the speaker's level (volume.h, 0 … 10) on an arc, − and +;
//   4 Radio settings — Auto-retry (off: a dropped stream stops and the station is Ready again), Track info (the song's
//     title in place of the bitrate), the catalog (Radio Browser), which is information only.
// The stations, the favourites, the folders, the stream and the speaker are radio.h's, radio_net.h's and volume.h's —
// this file only shows them and calls them. The old main screen's parts (FL_RTIME … FL_RPLAY) stay in the editor, hidden
// on the board (radio.h). Included from PopkaWatch.ino after upager.h.
#if HAS_RAPP
extern const HaloFont gl_rpg_title, gl_rpg_tnar, gl_rpg_row, gl_rpg_meta, gl_rpg_mono, gl_rpg_vol, gl_rpg_ic44, gl_rpg_ic32,
    gl_rpg_ic31, gl_rpg_ic22, gl_rpg_ic20;  // (studio.py ROUND_APP_FONTS["radio"])
// The design's tokens
#define RPG_ACC 0xA8E9F2    // accent
#define RPG_INK 0x142D34    // dark ink on the accent
#define RPG_PANEL 0x11262D  // the poster, an inset badge, the source card
#define RPG_CARD 0x152E36   // a card, a secondary button
#define RPG_CUR 0x244752    // the selected station's card
#define RPG_RING 0x203940   // the volume's track, a switch off
#define RPG_WAVE 0x39707D   // the level bars, a disabled slice button
#define RPG_SOFT 0x86A9B2   // metadata, a switch's thumb off
#define RPG_IC_PREV "\xEF\x81\x88"
#define RPG_IC_NEXT "\xEF\x81\x91"
#define RPG_IC_LEFT "\xEF\x84\x84"
#define RPG_IC_RIGHT "\xEF\x84\x85"
#define RPG_IC_FOLDER "\xEF\x81\xBB"
#define RPG_IC_BACK "\xEF\x81\xA0"
#define RPG_IC_MINUS "\xEF\x81\xA8"
#define RPG_IC_PLUS "\xEF\x81\xA7"
#define RPG_IC_GLOBE "\xEF\x82\xAC"
#define RPG_SEP "  /  "  // (the design's metadata: two spaces either side of the slash)
// Stations' rows: full pills 76 px tall (a bold name over a grey line), 82 px apart; the pager's buttons under them
#define RPG_ROW_Y0 158.0f
#define RPG_ROW_DY 82.0f
#define RPG_ROW_H 76.0f
#define RPG_NAV_Y 402.0f

// ---------- Text by its ink ----------
// The ink's left and right edges of s from its pen's start (the design places ink, not advance boxes)
static void rpg_ink(const HaloFont &f, const char *s, float &l, float &r) {
  int pen = 0;
  l = 1e9f, r = -1e9f;
  for (const char *q = s; *q;) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(q));
    if (!g) continue;
    if (g->w > 1 || g->h > 1) l = fminf(l, pen + g->ox), r = fmaxf(r, pen + g->ox + g->w);
    pen += g->adv;
  }
  if (l > r) l = r = 0;
}
static inline float rpg_ink_w(const HaloFont &f, const char *s) {
  float l, r;
  rpg_ink(f, s, l, r);
  return r - l;
}
// A text whose ink is centred on y and, by align, starts at x (0) or is centred on x (1); sc — smaller (a long monogram)
static HaloItem &rpg_text(UpPage &P, const HaloFont &f, const char *s, float x, float y, int align, uint32_t c, float a,
                          const HaloView &v, const HaloItem *on = &halo_black, float sc = 1) {
  float l, r;
  rpg_ink(f, s, l, r);
  HaloItem &t = up_item(P);
  ui_text(t, f, s, halo_text_mid(f, s), align ? x - (l + r) / 2 * sc : x - l * sc, y, sc, 0, c, a, on);
  t.on_black = on == &halo_black;
  up_commit(t, v);
  return t;
}
// A round button: its disc, its glyph's ink in the middle
static void rpg_button(UpPage &P, float x, float y, float r, const char *icon, const HaloFont &f, uint32_t bg, uint32_t fg,
                       float a, const HaloView &v, float sc = 1) {
  HaloItem &d = up_disc(P, x, y, r, bg, a, v);
  rpg_text(P, f, icon, x, y, 1, fg, a, v, &d, sc);
}
static inline HaloItem &rpg_card(UpPage &P, float x0, float y0, float x1, float y1, float r, uint32_t c, float a,
                                 const HaloView &v) {
  up_rrect(P, x0, y0, x1, y1, r, c, a, v);
  return P.it[P.n - 1];
}

// ---------- Names and badges ----------
// The name shown: "Radio Swiss Classic" → "Swiss Classic" (a leading "Radio " goes when two words or more follow it;
// "Radio Paradise" stays)
static const char *rpg_disp(const char *name) {
  if (!strncasecmp(name, "radio ", 6) && strchr(name + 6, ' ')) return name + 6;
  return name;
}
// A letter's UTF-8 bytes in capitals (Latin and Cyrillic) into o; its length
static int rpg_upper(const char *p, char *o) {
  const int n = rd_char_len(p);
  memcpy(o, p, n);
  const uint8_t a = o[0], b = n > 1 ? o[1] : 0;
  if (n == 1 && a >= 'a' && a <= 'z') o[0] = a - 32;
  else if (n == 2 && a == 0xD0 && b >= 0xB0 && b <= 0xBF) o[1] = b - 0x20;                // а … п
  else if (n == 2 && a == 0xD1 && b >= 0x80 && b <= 0x8F) o[0] = 0xD0, o[1] = b + 0x20;   // р … я
  else if (n == 2 && a == 0xD1 && b == 0x91) o[0] = 0xD0, o[1] = 0x81;                     // ё
  return n;
}
// The badge's letters: two words' first ("Radio Paradise" → RP), a short word in capitals itself (FIP, KEXP), else its
// first two; a two-letter prefix in capitals before two more words is the brand, not the station ("RP Mellow Mix" → MM)
static void rpg_badge(const char *name, char *out, size_t cap) {
  const char *w[4];
  int n = 0;
  for (const char *p = rpg_disp(name); *p && n < 4;) {
    while (*p == ' ' || *p == '-' || *p == '_' || *p == '.' || *p == '(') p++;
    if (!*p) break;
    w[n++] = p;
    while (*p && *p != ' ' && *p != '-' && *p != '_' && *p != '.' && *p != '(') p++;
  }
  out[0] = 0;
  if (!n) return (void)strlcpy(out, "R", cap);
  if (n >= 3 && isupper((uint8_t)w[0][0]) && isupper((uint8_t)w[0][1]) && (w[0][2] == ' ' || w[0][2] == '-')) {
    for (int k = 0; k < n - 1; k++) w[k] = w[k + 1];
    n--;
  }
  char b[16];
  int o = 0;
  if (n == 1) {
    int len = 0;
    bool caps = true;
    for (const char *p = w[0]; *p && *p != ' ' && *p != '-'; p++) caps &= !(*p >= 'a' && *p <= 'z'), len++;
    const char *p = w[0];
    for (int k = 0; k < (caps && len <= 4 ? len : 2) && *p && *p != ' ' && o < 12; k++) {
      o += rpg_upper(p, b + o);
      p += rd_char_len(p);
    }
  } else {
    o += rpg_upper(w[0], b + o);
    o += rpg_upper(w[1], b + o);
  }
  b[o] = 0;
  strlcpy(out, b, cap);
}

// ---------- State ----------
enum { RPG_DIAL, RPG_FOLDERS, RPG_FOLDER };  // the Stations page's views
static int rpg_route, rpg_sl_dial, rpg_sl_folders, rpg_sl[RD_FOLDER_N];  // the view; the last slice in each
static bool rpg_retry = true, rpg_tinfo = true, rpg_prefs_in;
static float rpg_morph, rpg_m_from;
static uint32_t rpg_m_at;
static bool rpg_m_to;
static int rpg_lp = -1;     // a row held (its index in the view's list): a long press — the heart
static bool rpg_lp_done;    // … done: the release is not a tap
static char rpg_name_of[RD_NAME], rpg_name[RD_NAME + 8], rpg_mono[16], rpg_meta[RD_TITLE + 24], rpg_vol_s[4];
static const HaloFont *rpg_name_f = &gl_rpg_title;
static char rpg_rn[3][RD_NAME + 8], rpg_rm[3][72], rpg_rb[3][16], rpg_count[12], rpg_title_s[40];
static Preferences rpg_prefs;

static void rpg_prefs_load() {
  if (rpg_prefs_in) return;
  rpg_prefs_in = true;
  rpg_prefs.begin("radio", false);
  rpg_retry = rpg_prefs.getBool("retry", true), rpg_tinfo = rpg_prefs.getBool("tinfo", true);  // (a new one: both on)
  rn_retry = rpg_retry;
}
static void rpg_set(bool &pref, const char *key) {
  pref = !pref;
  rpg_prefs.putBool(key, pref);
  rn_retry = rpg_retry;
}

// The view's list: how many, and row i's station (nullptr — a folder: rpg_route RPG_FOLDERS)
static int rpg_n() {
  if (rpg_route == RPG_DIAL) return rd_list_n;
  if (rpg_route == RPG_FOLDERS) return RD_FOLDER_N;
  return ru->s_state == 1 ? ru->s_n : 0;
}
static const RdSt *rpg_st(int i) {
  if (rpg_route == RPG_DIAL) return i < rd_list_n ? &rd_list[i] : nullptr;
  if (rpg_route == RPG_FOLDER) return i < ru->s_n ? &ru->s_rows[i] : nullptr;
  return nullptr;
}
static int &rpg_slice() {
  return rpg_route == RPG_DIAL ? rpg_sl_dial : rpg_route == RPG_FOLDERS ? rpg_sl_folders : rpg_sl[ru->s_folder % RD_FOLDER_N];
}
static int rpg_slices() { return max(1, (rpg_n() + 2) / 3); }
static inline bool rpg_is_cur(const RdSt &s) { return rd_has_cur && !strcmp(rd_cur.url, s.url); }

// A folder opened (the folders' view): radio.h's rows — the favourites at once, else the catalog asked (rd_rows_take)
static void rpg_open_folder(int i) {
  rd_rows_open(i);
  rpg_route = RPG_FOLDER;
  rpg_sl[i] = constrain(rpg_sl[i], 0, rpg_slices() - 1);
}
// A station of a folder picked: the folder onto the dial, it plays (the view stays, the row highlighted)
static void rpg_pick_folder(int i) {
  if (i < ru->s_n) rd_rows_play(i);
}
static void rpg_heart(int i) {  // a long press on a station's row: in the favourites or out (radio.h)
  const RdSt *s = rpg_st(i);
  if (!s) return;
  const RdSt keep = *s;  // (the list may shift under it)
  rd_fav_toggle(keep);
  if (rpg_route == RPG_FOLDER && RD_FOLDER[ru->s_folder].kind == RF_FAV) rd_rows_favs();
  rpg_slice() = constrain(rpg_slice(), 0, rpg_slices() - 1);
}

// ---------- 1 Station ----------
static void rpg_station_strings() {
  const RdSt *s = rd_has_cur ? &rd_cur : rd_list_n ? &rd_list[0] : nullptr;
  const char *name = s ? s->name : "Radio";
  if (strcmp(rpg_name_of, name)) {  // the name: the title's cut, else narrower, else cut with "…" — 272 px of ink
    strlcpy(rpg_name_of, name, sizeof(rpg_name_of));
    const char *d = rpg_disp(name);
    rpg_name_f = &gl_rpg_title;
    strlcpy(rpg_name, d, sizeof(rpg_name));
    if (rpg_ink_w(gl_rpg_title, d) > 272) {
      rpg_name_f = &gl_rpg_tnar;
      if (rpg_ink_w(gl_rpg_tnar, d) > 272) rd_fit(gl_rpg_tnar, d, 268, rpg_name, sizeof(rpg_name));
    }
    rpg_badge(name, rpg_mono, sizeof(rpg_mono));
  }
  // LIVE / READY / how it goes, then the song (Track info) or the bitrate
  const bool on = rd_on && s;
  const char *st = !on ? "Ready" : rn_playing ? "Live" : rn_net == RN_NOWIFI ? "No Wi-Fi" : rn_net == RN_FAILED ? "Retrying"
                 : rn_net == RN_JOINING ? "Joining Wi-Fi" : rn_net == RN_STREAMING ? "Buffering" : "Connecting";
  char tail[RD_TITLE];
  tail[0] = 0;
  if (on && rn_playing && rpg_tinfo) rn_title_copy(tail, sizeof(tail));
  if (!tail[0] && s) snprintf(tail, sizeof(tail), "%d kbps", s->kbps);
  char m[RD_TITLE + 24];
  if (!s) strlcpy(m, "No stations", sizeof(m));
  else snprintf(m, sizeof(m), "%s \xC2\xB7 %s", st, tail);  // (the card language's second line: sentence case, grey)
  rd_fit(gl_row_sub, m, 300, rpg_meta, sizeof(rpg_meta));
}
static void rpg_lay_station(UpPage &P, float a, const HaloView &v) {
  up_title(P, "Radio", a, v);
  rpg_station_strings();
  // (Wear OS's media screen: no card round it — the monogram and its bars, the name, the status on black)
  const HaloItem &post = halo_black;
  // The paired level bars (x 233 ± 108, 92, 76; 5 px, round ends, on y 190): ready — 12 px each; live — the sound's
  // bands (radio.h's smoothed levels) up to 16, 28, 48 px
  static const float OFF[3] = {108, 92, 76}, MAX[3] = {16, 28, 48};
  static const uint8_t BAND[2][3] = {{15, 8, 1}, {17, 10, 3}};
  const bool live = rd_on && rn_playing;
  for (int k = 0; k < 3; k++)
    for (int sd = 0; sd < 2; sd++) {
      const float len = live ? fmaxf(5, MAX[k] * halo_clamp01(ru->lv[BAND[sd][k]] * 1.15f)) : 12;
      const float x = CX + (sd ? OFF[k] : -OFF[k]);
      up_rrect(P, x - 2.5f, 178 - len / 2, x + 2.5f, 178 + len / 2, 2.5f, RPG_WAVE, a, v);
    }
  // The monogram (a wide one smaller: 132 px of ink at most — RP's — clear of the inner bars)
  const float mw = rpg_ink_w(gl_rpg_mono, rpg_mono);
  rpg_text(P, gl_rpg_mono, rpg_mono, CX, 178, 1, RPG_ACC, a, v, &post, mw > 132 ? 132 / mw : 1);
  rpg_text(P, *rpg_name_f, rpg_name, CX, 240, 1, UP_WHITE, a, v, &post);
  rpg_text(P, gl_row_sub, rpg_meta, CX, 276, 1, 0x9E9E9E, a, v, &post);
  rpg_button(P, 108, 346, 34, RPG_IC_PREV, gl_rpg_ic32, RPG_CARD, RPG_ACC, a, v);
  rpg_button(P, 358, 346, 34, RPG_IC_NEXT, gl_rpg_ic32, RPG_CARD, RPG_ACC, a, v);
  up_primary(P, CX, 354, 55, rpg_morph, rd_on ? UP_IC_PAUSE : UP_IC_PLAY, RPG_ACC, RPG_INK, a, v, &gl_rpg_ic44);
}

// ---------- 2 Stations ----------
static void rpg_lay_stations(UpPage &P, float a, const HaloView &v) {
  const int n = rpg_n(), cnt = rpg_slices();
  int &sl = rpg_slice();
  sl = constrain(sl, 0, cnt - 1);
  if (rpg_route == RPG_DIAL) up_title(P, "Stations", a, v);
  else if (rpg_route == RPG_FOLDERS) up_title(P, "Folders", a, v);
  else strlcpy(rpg_title_s, RD_FOLDER[ru->s_folder].name, sizeof(rpg_title_s)), up_title(P, rpg_title_s, a, v);
  for (int r = 0; r < 3; r++) {
    const int i = sl * 3 + r;
    if (i >= n) continue;
    const float y = RPG_ROW_Y0 + RPG_ROW_DY * r;
    bool sel;
    const char *icon = nullptr;
    char meta[64];
    if (rpg_route == RPG_FOLDERS) {  // a folder: its icon, its name, what it is
      const RdFolder &fd = RD_FOLDER[i];
      sel = i == rd_folder;
      icon = RD_FIC[fd.icon % 5];
      rd_fit(gl_rpg_row, fd.name, 236, rpg_rn[r], sizeof(rpg_rn[r]));  // (the rows' type with the stations' letters)
      if (fd.kind == RF_FAV) snprintf(meta, sizeof(meta), "%d station%s", rd_fav_n, rd_fav_n == 1 ? "" : "s");
      else if (fd.kind == RF_TOP) strlcpy(meta, "Most listened", sizeof(meta));
      else if (fd.kind == RF_TAG) strlcpy(meta, "Genre", sizeof(meta));
      else snprintf(meta, sizeof(meta), "Country \xC2\xB7 %s", fd.arg);
      if (sel) {
        char t[64];
        snprintf(t, sizeof(t), "On the dial \xC2\xB7 %s", meta);
        strlcpy(meta, t, sizeof(meta));
      }
    } else {  // a station: its badge, its name, its country and bitrate (selected, or a favourite in a folder)
      const RdSt &s = *rpg_st(i);
      sel = rpg_is_cur(s);
      rpg_badge(s.name, rpg_rb[r], sizeof(rpg_rb[r]));
      rd_fit(gl_rpg_row, rpg_disp(s.name), 236, rpg_rn[r], sizeof(rpg_rn[r]));
      const bool fav = rpg_route == RPG_FOLDER && RD_FOLDER[ru->s_folder].kind != RF_FAV && rd_fav_find(s.url) >= 0;
      snprintf(meta, sizeof(meta), "%s%s%s%d kbps", sel ? "Selected \xC2\xB7 " : fav ? "Favorite \xC2\xB7 " : "", s.cc,
               s.cc[0] ? " \xC2\xB7 " : "", s.kbps);
    }
    rd_fit(gl_row_sub, meta, 236, rpg_rm[r], sizeof(rpg_rm[r]));
    // (the card language: a full pill, the name bold over a lighter grey line — upager.h up_row)
    const bool press = up_pressed(v, 58, y - RPG_ROW_H / 2, 408, y + RPG_ROW_H / 2);  // (the rows' press: lighter, squeezed)
    HaloItem &card = rpg_card(P, 58, y - RPG_ROW_H / 2, 408, y + RPG_ROW_H / 2, RPG_ROW_H * (press ? 0.36f : 0.5f),
                              ui_mix(sel ? RPG_CUR : RPG_CARD, 0xFFFFFF, press ? 0.1f : 0), a, v);
    HaloItem &badge = up_disc(P, 58 + RPG_ROW_H / 2, y, 24, sel ? RPG_ACC : RPG_PANEL, a, v);  // (concentric with the pill's end)
    if (icon) rpg_text(P, gl_rpg_ic22, icon, 58 + RPG_ROW_H / 2, y, 1, sel ? RPG_INK : RPG_ACC, a, v, &badge, 0.82f);
    else rpg_text(P, gl_rpg_meta, rpg_rb[r], 58 + RPG_ROW_H / 2, y, 1, sel ? RPG_INK : RPG_ACC, a, v, &badge);
    rpg_text(P, gl_rpg_row, rpg_rn[r], 132, y - 14, 0, UP_WHITE, a, v, &card);
    rpg_text(P, gl_row_sub, rpg_rm[r], 132, y + 16, 0, sel ? RPG_ACC : 0x9E9E9E, a, v, &card);
  }
  // Empty, loading, failed: a word in the middle of the rows
  const char *msg = nullptr, *sub = nullptr;
  if (rpg_route == RPG_FOLDER && ru->s_state == 0) msg = "Loading";
  else if (rpg_route == RPG_FOLDER && ru->s_state == 2) msg = rn_net == RN_NOWIFI ? "No Wi-Fi" : "No connection", sub = "Tap to retry";
  else if (!n) msg = rpg_route == RPG_DIAL || RD_FOLDER[ru->s_folder].kind == RF_FAV ? "No favorites yet" : "Nothing here";
  if (msg) rpg_text(P, gl_rpg_row, msg, CX, sub ? 220 : 231, 1, UP_WHITE, a, v);  // (an empty state: a bold line, a grey one)
  if (sub) rpg_text(P, gl_row_sub, sub, CX, 252, 1, 0x9E9E9E, a, v);
  // ‹ (folder or back) › and the slice count; an arrow at its end is shown, dimmed, and not a target
  const bool pv = sl > 0, nx = sl < cnt - 1;
  // (no "1 / 2" under them: the arrows dim at the ends, the rim's dots say the page)
  rpg_button(P, 165, RPG_NAV_Y, 24, RPG_IC_LEFT, gl_rpg_ic22, pv ? RPG_CARD : RPG_PANEL, pv ? RPG_ACC : RPG_WAVE, a, v);
  rpg_button(P, CX, RPG_NAV_Y, 24, rpg_route == RPG_DIAL ? RPG_IC_FOLDER : RPG_IC_BACK, gl_rpg_ic22, RPG_CARD, RPG_ACC, a, v);
  rpg_button(P, 301, RPG_NAV_Y, 24, RPG_IC_RIGHT, gl_rpg_ic22, nx ? RPG_CARD : RPG_PANEL, nx ? RPG_ACC : RPG_WAVE, a, v);
}
static int rpg_row_at(float x, float y) {  // the row under (x, y) in the view, or −1
  const int sl = rpg_slice(), n = rpg_n();
  for (int r = 0; r < 3; r++) {
    const float cy = RPG_ROW_Y0 + RPG_ROW_DY * r;
    if (up_in(x, y, 58, cy - RPG_ROW_H / 2, 408, cy + RPG_ROW_H / 2) && sl * 3 + r < n) return sl * 3 + r;
  }
  return -1;
}
static void rpg_tap_stations(float x, float y) {
  int &sl = rpg_slice();
  const int cnt = rpg_slices();
  if (up_hit(x, y, 165, RPG_NAV_Y, 24)) return void(sl > 0 ? sl-- : 0);
  if (up_hit(x, y, 301, RPG_NAV_Y, 24)) return void(sl < cnt - 1 ? sl++ : 0);
  if (up_hit(x, y, CX, RPG_NAV_Y, 24)) {  // the folders, or back one level (the last view of each kept)
    rpg_route = rpg_route == RPG_DIAL ? RPG_FOLDERS : rpg_route == RPG_FOLDER ? RPG_FOLDERS : RPG_DIAL;
    if (rpg_route == RPG_FOLDERS && rpg_sl_folders * 3 > rd_folder + 2) rpg_sl_folders = rd_folder / 3;
    return;
  }
  if (rpg_route == RPG_FOLDER && ru->s_state == 2 && up_in(x, y, 66, 193, 400, 269)) return rd_rows_retry();  // Retry
  const int i = rpg_row_at(x, y);
  if (i < 0) return;
  if (rpg_route == RPG_FOLDERS) rpg_open_folder(i);
  else if (rpg_route == RPG_DIAL) rd_tune(i);
  else if (!(rd_on && rpg_is_cur(ru->s_rows[i]))) rpg_pick_folder(i);
}

// ---------- 3 Volume: Wear OS's volume screen (upager.h up_volume) ----------
static void rpg_lay_volume(UpPage &P, float a, const HaloView &v) {
  up_volume(P, vol_level, RPG_ACC, RPG_RING, gl_rpg_ic31, RPG_IC_PLUS, RPG_IC_MINUS, a, v, &gl_ra_ic24);
}

// ---------- 4 Radio settings: Wear OS's settings list (upager.h list mode) ----------
static int rpg_items(int p) { return p == 3 ? 4 : 0; }
static float rpg_item_h(int, int k) { return up_set_h(k); }
static void rpg_lay_item(int, int k, UpPage &P, float, float ac, const HaloView &v) {
  if (k == 0) return up_set_title(P, "Settings", ac, v);
  if (k == 1) up_set_row(P, "Auto-retry", "Reconnect on drop", UP_ROW_SWITCH, rpg_retry, RPG_ACC, RPG_INK, ac, v);
  else if (k == 2) up_set_row(P, "Track info", "Show song titles", UP_ROW_SWITCH, rpg_tinfo, RPG_ACC, RPG_INK, ac, v);
  else up_set_row(P, "Radio Browser", "Station catalog", UP_ROW_BUTTON, 0, RPG_ACC, RPG_INK, ac, v);
}

// ---------- The app ----------
static int rpg_count_pages() { return 4; }
static void rpg_lay(int p, UpPage &P, float a, const HaloView &v) {
  if (!ru) return;
  if (p == 0) rpg_lay_station(P, a, v);
  else if (p == 1) rpg_lay_stations(P, a, v);
  else if (p == 2) rpg_lay_volume(P, a, v);  // (3, Settings: a list — rpg_lay_item)
}
static void rpg_tap(int p, float x, float y) {
  if (!ru) return;
  if (p == 0) {
    const int at = rd_at >= 0 ? rd_at : 0;
    if (up_in(x, y, 74, 312, 142, 380) && rd_list_n) rd_tune(rd_mod(at - 1, rd_list_n));
    else if (up_in(x, y, 324, 312, 392, 380) && rd_list_n) rd_tune(rd_mod(rd_at >= 0 ? at + 1 : 0, rd_list_n));
    else if (up_in(x, y, 178, 299, 288, 409)) rd_toggle();
  } else if (p == 1) {
    if (!rpg_lp_done) rpg_tap_stations(x, y);
  } else if (p == 2) {
    if (const int d = up_volume_tap(x, y)) vol_set(constrain(vol_level + d, 0, 10), false);
  } else {
    const int k = up_list_hit(3, y);
    if (k == 1) rpg_set(rpg_retry, "retry");
    else if (k == 2) rpg_set(rpg_tinfo, "tinfo");
  }
}
static void rpg_update(int p, float dt) {
  if (!ru) return;
  rd_rows_take();
  // Play ↔ Pause: the primary's shape over 180 ms, ease-out cubic
  if (rpg_m_to != rd_on) rpg_m_to = rd_on, rpg_m_from = rpg_morph, rpg_m_at = halo_ms();
  const float u = halo_clamp01((halo_ms() - rpg_m_at) / 180.0f), e = 1 - (1 - u) * (1 - u) * (1 - u);
  rpg_morph = rpg_m_from + ((rpg_m_to ? 1 : 0) - rpg_m_from) * e;
  // A long press on a station's row: the heart (the release then is not a tap)
  const HaloFinger &f = halo_finger;
  if (f.pressed) rpg_lp_done = false, rpg_lp = p == 1 && rpg_route != RPG_FOLDERS ? rpg_row_at(f.x0, f.y0) : -1;
  if (f.dragging || !f.down) rpg_lp = -1;
  if (rpg_lp >= 0 && halo_ms() - f.t0 > 550) rpg_heart(rpg_lp), rpg_lp = -1, rpg_lp_done = true;
}
static const UpApp RPG_APP = {rpg_count_pages, rpg_lay, rpg_tap, rpg_update, nullptr, RPG_ACC, nullptr, false, rpg_items, rpg_item_h, rpg_lay_item};

// The screen (settings.h calls these for FL_RADIO)
static void rapp_enter() {
  rpg_prefs_load();
  if (ru && !rd_has_cur && rd_list_n) rd_cur = rd_list[0], rd_has_cur = true, rd_at = 0;  // (Ready: the dial's first)
  rpg_m_to = rd_on, rpg_morph = rpg_m_from = rd_on ? 1 : 0;
  up_open(&RPG_APP);
}
static void rapp_update(float dt) { up_update(dt); }
static void rapp_layout(const HaloView &v) { up_layout(v); }
static void rapp_draw() { up_draw(); }
#endif
