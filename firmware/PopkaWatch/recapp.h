// recapp.h — the Recorder's round screen (FL_RECORDER) as three non-scrolling pages swiped left and right (upager.h;
// Codex's measured study, design/SPEC_recorder.md — 23 Nov 2026):
//   1 Capture — the elapsed time big, a live level meter (23 bars, the last 23 slices of 75 ms) in a card, Recordings
//     and the microphone; recording — the big button morphs to four lobes with Stop, the rim fills to the clip limit,
//     the pager holds still (stop first to swipe);
//   2 Recordings — the clips as cards, a tap opens one and plays it (the playback child: the played time, the clip's
//     overview with the played bars in coral, Back and Pause / Play, the rim to the end of the clip);
//   3 Recorder settings — the clip limit (15, 30, 60 s), Level audio (the playback's automatic gain and quiet gate),
//     what is kept.
// The sound — capture, cleaning, playback, levelling — is recorder.h's. This firmware keeps ONE clip, in PSRAM: the
// list shows it (numbered from the watch's start) and says it is in memory only (lost when the power goes), and
// recording again asks first
// (the spec's fallback for a build without a persistent clip store). Round board only; included from PopkaWatch.ino
// after upager.h.
#ifdef HALO_ROUND
#include <Preferences.h>
extern const HaloFont gl_ra_dur, gl_ra_ic43, gl_ra_ic38, gl_ra_ic31, gl_ra_ic26, gl_ra_ic19;  // (studio.py GALLERY_FONTS)
#define RA_IC_MIC "\xEF\x84\xB0"
#define RA_IC_STOP "\xEF\x81\x8D"
#define RA_IC_LIST "\xEF\x80\xBA"
#define RA_IC_BACK "\xEF\x81\xA0"
#define RA_IC_MINUS "\xEF\x81\xA8"
#define RA_IC_PLUS "\xEF\x81\xA7"
// The spec's tokens
#define RA_ACC 0xFFB7AD    // coral
#define RA_INK 0x401910    // on coral: the primary's glyph, the switch's thumb
#define RA_CTRL 0x382324   // secondary controls
#define RA_PANEL 0x22171B  // cards, the idle wave's card
#define RA_HOT 0x321B1E    // the wave's card while recording
#define RA_QUIET 0x795655  // the quiet wave, disabled glyphs
// The action row (apps/SPEC.md): the secondary r 38 at (161, 352), the primary r 55 at (287, 352); square hit boxes
#define RA_SEC_X 161.0f
#define RA_PRI_X 287.0f
#define RA_ROW_Y 352.0f
#define RA_BARS 23
static const int RA_LIMITS[3] = {15, 30, 60};

static bool ra_child;        // the playback child is open (over Recordings)
static int ra_back_page = 1;  // the page Back returns to
static HaloSpring ra_morph, ra_pmorph, ra_sw;  // the capture button's lobes; the playback's; Level audio's thumb
static HaloSpringK K_RA;
static float ra_ov[RA_BARS];  // the clip's overview (bar heights), made when the child opens
static char ra_time[8], ra_cap[16], ra_dur[8], ra_stamp[24], ra_sum[40], ra_lim[4];
static float ra_mid_mic, ra_mid_stop, ra_mid_list, ra_mid_back, ra_mid_pm, ra_mid_play19, ra_mid_dur;
static Preferences ra_prefs;
static int ra_owner;  // (the horizontal drag held while recording)

static void ra_mmss(char *o, int n, float sec) {
  const int s = max(0, (int)sec);
  snprintf(o, n, "%02d:%02d", s / 60, s % 60);
}
static inline float ra_bar_h(int lv) { return 2 + 22 * constrain(lv, 0, 255) / 255.0f; }  // level 0..255 → 2..24 px
static void ra_bar(UpPage &P, int i, float h, uint32_t c, float a, const HaloView &v) {
  const float x = 123 + 10 * i;  // (a round 4 px stroke, caps r 2: the box is h + 4 tall)
  up_rrect(P, x - 2, 260 - h / 2 - 2, x + 2, 260 + h / 2 + 2, 2, c, a, v);
}
// The capture's and the playback's shared card: the wave and its caption
static HaloItem &ra_wave_card(UpPage &P, uint32_t fill, float a, const HaloView &v) {
  up_rrect(P, 86, 243, 380, 293, 24, fill, a, v);
  return P.it[P.n - 1];
}
// The big button: the shape (up_primary: circle → four lobes by t), then a glyph of the spec's size on it
static void ra_primary(UpPage &P, float t, const HaloFont &f, const char *ic, float mid, float a, const HaloView &v) {
  up_primary(P, RA_PRI_X, RA_ROW_Y, 55, t, "", RA_ACC, RA_INK, a, v);
  HaloItem &none = P.it[P.n - 1];  // (up_primary's own glyph: none here — hidden)
  none.alpha = 0, up_commit(none, v);
  up_text(P, f, ic, mid, RA_PRI_X, RA_ROW_Y, 1, RA_INK, a, v, nullptr);
}
static void ra_secondary(UpPage &P, uint32_t bg, const char *ic, float mid, uint32_t fg, float a, const HaloView &v) {
  HaloItem &d = up_disc(P, RA_SEC_X, RA_ROW_Y, 38, bg, a, v);
  up_text(P, gl_ra_ic31, ic, mid, RA_SEC_X, RA_ROW_Y, 1, fg, a, v, &d);
}
static bool ra_in_pri(float x, float y) { return up_in(x, y, 232, 297, 342, 407); }
static bool ra_in_sec(float x, float y) { return up_in(x, y, 123, 314, 199, 390); }

// ---------- The kept settings ----------
static void ra_keep() { ra_prefs.putUChar("limit", rc_limit_s), ra_prefs.putBool("level", rc_level_on); }
static void ra_step_limit(int d) {
  int k = 0;
  while (k < 2 && RA_LIMITS[k] < rc_limit_s) k++;
  k = constrain(k + d, 0, 2);
  if (RA_LIMITS[k] != rc_limit_s) rc_limit_s = RA_LIMITS[k], ra_keep();
}

// ---------- What the clip is called, when it was made ----------
static int ra_taken;  // clips captured since the watch started: the clip's number
static char ra_name_s[12];
static const char *ra_name() {
  snprintf(ra_name_s, sizeof(ra_name_s), "Clip %02d", constrain(ra_taken, 1, 99));
  return ra_name_s;
}
static void ra_when(char *o, int n) {
  static const char *const MON[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  const time_t now = time(nullptr);
  struct tm w, t;
  localtime_r(&rc_when, &w), localtime_r(&now, &t);
  if (w.tm_year < 120) return (void)strlcpy(o, "IN MEMORY", n);  // (the clock was not set yet)
  if (w.tm_yday == t.tm_yday && w.tm_year == t.tm_year) snprintf(o, n, "TODAY %02d:%02d", w.tm_hour, w.tm_min);
  else snprintf(o, n, "%d %s %02d:%02d", w.tm_mday, MON[w.tm_mon], w.tm_hour, w.tm_min);
}
static void ra_overview() {  // the whole clip in 23 bars: each the mean loudness of its slices
  for (int s = 0; s < RA_BARS; s++) {
    if (!rc_k) {
      ra_ov[s] = 2;
      continue;
    }
    const int a = min(s * rc_k / RA_BARS, rc_k - 1), b = min(max((s + 1) * rc_k / RA_BARS, a + 1), rc_k);
    int sum = 0;
    for (int i = a; i < b; i++) sum += (rc_lv[i][0] + rc_lv[i][1]) / 2;
    ra_ov[s] = ra_bar_h(sum / (b - a));
  }
}

// ---------- The actions ----------
static void ra_record() {
  rc_record();
  if (rc_state == RC_REC) ra_taken++;
}
static void ra_replace_yes() {
  halo_nav_back();
  ra_record();
}
static void ra_capture_tap() {
  // Stop: the end cut back to the finger's touch (its click), as before
  if (rc_state == RC_REC) return rc_stop(fminf(0.5f, (halo_ms() - halo_finger.t0) / 1000.0f + 0.04f));
  if (rc_len > 0)  // (one clip is kept: replacing it is asked first)
    return ui_confirm_open({UI_IC_RESET, "Replace clip?", "Only the last clip is kept", RA_ACC, ra_replace_yes});
  ra_record();
}
static void ra_open_clip();  // (below the two apps)
static void ra_close_clip();

// ---------- The pages ----------
static int ra_count() { return 3; }
static void ra_lay(int p, UpPage &P, float a, const HaloView &v) {
  const bool rec = rc_state == RC_REC;
  if (p == 0) {  // Capture: ready or recording
    up_title(P, "Recorder", a, v);
    ra_mmss(ra_time, sizeof(ra_time), rec ? rc_t : 0);
    up_text(P, gl_up_hero, ra_time, up_mid_hero, CX, 181, 1, UP_WHITE, a, v);
    HaloItem &card = ra_wave_card(P, rec ? RA_HOT : RA_PANEL, a, v);
    for (int i = 0; i < RA_BARS; i++) {  // the last 23 slices, the newest (in progress) at the right
      float h = 2;
      if (rec) {
        const int s = rc_k - (RA_BARS - 1) + i;
        if (s >= 0 && s < rc_k) h = ra_bar_h((rc_lv[s][0] + rc_lv[s][1]) / 2);
        else if (s == rc_k) h = ra_bar_h((rc_cur[0] + rc_cur[1]) / 2);
      }
      ra_bar(P, i, h, rec ? RA_ACC : RA_QUIET, a, v);
    }
    char lim[8];
    ra_mmss(lim, sizeof(lim), rc_limit_s);
    snprintf(ra_cap, sizeof(ra_cap), rec ? "RECORDING" : "UP TO %s", lim);
    up_text(P, gl_up_lab, ra_cap, up_mid_lab, CX, 282, 1, rec ? RA_ACC : UP_MUTED, a, v, &card);
    ra_secondary(P, rec ? RA_PANEL : RA_CTRL, RA_IC_LIST, ra_mid_list, rec ? RA_QUIET : RA_ACC, a, v);
    ra_primary(P, ra_morph.x, rec ? gl_ra_ic38 : gl_ra_ic43, rec ? RA_IC_STOP : RA_IC_MIC, rec ? ra_mid_stop : ra_mid_mic, a, v);
  } else if (p == 1) {  // Recordings
    up_title(P, "Recordings", a, v);
    if (!rc_has() && rc_state != RC_PLAY) {
      up_text(P, gl_up_title, "No clips yet", up_mid_title, CX, 222, 1, UP_MUTED, a, v);
      up_text(P, gl_up_lab, "TAP THE MICROPHONE TO RECORD", up_mid_lab, CX, 256, 1, UP_MUTED, a, v);
      return;
    }
    const float y = 164;
    up_rrect(P, 80, y - 36, 386, y + 36, 24, RA_PANEL, a, v);
    HaloItem &row = P.it[P.n - 1];
    ra_when(ra_stamp, sizeof(ra_stamp));
    const int s = (int)((float)rc_len / rc_rate + 0.5f);
    snprintf(ra_dur, sizeof(ra_dur), "%d:%02d", s / 60, s % 60);
    up_text(P, gl_up_title, ra_name(), up_mid_title, 107, y - 9, 0, UP_WHITE, a, v, &row);
    up_text(P, gl_up_lab, ra_stamp, up_mid_lab, 107, y + 17, 0, UP_MUTED, a, v, &row);
    up_text(P, gl_ra_dur, ra_dur, ra_mid_dur, 306, y, 2, UP_WHITE, a, v, &row);
    HaloItem &d = up_disc(P, 348, y, 25, RA_CTRL, a, v);
    up_text(P, gl_ra_ic19, UP_IC_PLAY, ra_mid_play19, 350, y, 1, RA_ACC, a, v, &d);
    // (a build without a clip store: say what happens to it)
    up_text(P, gl_up_lab, "KEPT IN MEMORY ONLY", up_mid_lab, CX, 236, 1, UP_MUTED, a, v);
    up_text(P, gl_up_lab, "RECORDING AGAIN REPLACES IT", up_mid_lab, CX, 258, 1, UP_MUTED, a, v);
  } else {  // Recorder settings
    up_title(P, "Recorder settings", a, v);
    up_rrect(P, 74, 126, 392, 267, 28, RA_PANEL, a, v);
    HaloItem &card = P.it[P.n - 1];
    up_text(P, gl_up_title, "Clip limit", up_mid_title, CX, 146, 1, UP_WHITE, a, v, &card);
    snprintf(ra_lim, sizeof(ra_lim), "%d", rc_limit_s);
    up_text(P, gl_up_disp, ra_lim, up_mid_disp, CX, 206, 1, UP_WHITE, a, v, &card);
    up_text(P, gl_up_lab, "SECONDS", up_mid_lab, CX, 254, 1, UP_MUTED, a, v, &card);
    for (int k = 0; k < 2; k++) {  // − and +: quiet at the ends (no hit there)
      const float x = k ? 347 : 119;
      const bool on = k ? rc_limit_s < RA_LIMITS[2] : rc_limit_s > RA_LIMITS[0];
      HaloItem &d = up_disc(P, x, 203, 27, RA_CTRL, a, v);
      const char *ic = k ? RA_IC_PLUS : RA_IC_MINUS;
      up_text(P, gl_ra_ic26, ic, halo_text_mid(gl_ra_ic26, ic), x, 203, 1, on ? RA_ACC : RA_QUIET, a, v, &d);
    }
    up_rrect(P, 80, 280, 386, 350, 24, RA_PANEL, a, v);
    HaloItem &lv = P.it[P.n - 1];
    up_text(P, gl_up_title, "Level audio", up_mid_title, 104, 305, 0, UP_WHITE, a, v, &lv);
    up_text(P, gl_up_lab, "EVEN OUT PLAYBACK", up_mid_lab, 104, 331, 0, UP_MUTED, a, v, &lv);
    const float s = halo_clamp01(ra_sw.x);  // the switch: off — the secondary fill, a muted thumb left; on — coral
    up_rrect(P, 303, 298, 366, 332, 17, ui_mix(RA_CTRL, RA_ACC, s), a, v);
    up_disc(P, 321 + 27 * s, 315, 12, ui_mix(UP_MUTED, RA_INK, s), a, v);
    if (!rc_len) strlcpy(ra_sum, "NO CLIPS", sizeof(ra_sum));
    else snprintf(ra_sum, sizeof(ra_sum), "1 CLIP  /  %.1f MB IN MEMORY", rc_len * 2 / 1e6f);
    up_text(P, gl_up_lab, ra_sum, up_mid_lab, CX, 381, 1, UP_MUTED, a, v);
  }
}
static void ra_tap(int p, float x, float y) {
  if (p == 0) {
    if (ra_in_pri(x, y)) ra_capture_tap();
    else if (ra_in_sec(x, y) && rc_state != RC_REC) up_go(1);
  } else if (p == 1) {
    if (up_in(x, y, 80, 128, 386, 200)) ra_open_clip();
  } else {
    if (up_in(x, y, 92, 176, 146, 230)) ra_step_limit(-1);
    else if (up_in(x, y, 320, 176, 374, 230)) ra_step_limit(1);
    else if (up_in(x, y, 80, 280, 386, 350)) rc_level_on = !rc_level_on, ra_keep();
  }
}
static void ra_springs(float dt) {
  ra_morph.target = rc_state == RC_REC, ra_pmorph.target = rc_state == RC_PLAY, ra_sw.target = rc_level_on;
  halo_spring_run(ra_morph, K_RA, dt), halo_spring_run(ra_pmorph, K_RA, dt), halo_spring_run(ra_sw, K_RA, dt);
}
static void ra_upd(int, float dt) { ra_springs(dt); }
static float ra_ring() {  // capture: elapsed of the limit (the page in front; the other pages leave the rim black)
  if (up_pos.x > 0.5f) return -2;
  return rc_state == RC_REC ? rc_t / rc_limit_s : 0;
}
static const UpApp RA_APP = {ra_count, ra_lay, ra_tap, ra_upd, ra_ring, RA_ACC};

// The playback child (over Recordings): one page, Back in place of the dots
static int rp_count() { return 1; }
static void rp_lay(int, UpPage &P, float a, const HaloView &v) {
  const bool play = rc_state == RC_PLAY;
  up_title(P, ra_name(), a, v);
  ra_mmss(ra_time, sizeof(ra_time), (float)rc_pos / rc_rate);
  up_text(P, gl_up_hero, ra_time, up_mid_hero, CX, 181, 1, UP_WHITE, a, v);
  HaloItem &card = ra_wave_card(P, RA_PANEL, a, v);
  const int played = rc_len ? (int)lroundf(RA_BARS * (float)rc_pos / rc_len) : 0;
  for (int i = 0; i < RA_BARS; i++) ra_bar(P, i, ra_ov[i], i < played ? RA_ACC : RA_QUIET, a, v);
  char tot[8];
  ra_mmss(tot, sizeof(tot), (float)rc_len / rc_rate);
  snprintf(ra_cap, sizeof(ra_cap), "OF %s", tot);
  up_text(P, gl_up_lab, ra_cap, up_mid_lab, CX, 282, 1, UP_MUTED, a, v, &card);
  ra_secondary(P, RA_CTRL, RA_IC_BACK, ra_mid_back, RA_ACC, a, v);
  up_primary(P, RA_PRI_X, RA_ROW_Y, 55, ra_pmorph.x, play ? UP_IC_PAUSE : UP_IC_PLAY, RA_ACC, RA_INK, a, v);
}
static void rp_tap(int, float x, float y) {
  if (ra_in_pri(x, y)) rc_state == RC_PLAY ? (void)(rc_playing = false, rc_state = RC_DONE) : rc_play();
  else if (ra_in_sec(x, y)) ra_close_clip();
}
static float rp_ring() { return rc_len ? (float)rc_pos / rc_len : 0; }
static const UpApp RA_PLAY = {rp_count, rp_lay, rp_tap, ra_upd, rp_ring, RA_ACC};

// Into the playback child (the row tapped: it plays from the start) and back to the page it came from
static void ra_open_clip() {
  if (!rc_has()) return;
  ra_back_page = up_page, ra_child = true;
  rc_playing = false, rc_pos = 0, rc_state = RC_DONE;
  ra_overview();
  halo_spring_set(ra_pmorph, 0);
  up_open(&RA_PLAY);
  rc_play();
}
static void ra_close_clip() {
  if (rc_state == RC_PLAY) rc_playing = false, rc_state = RC_DONE;
  ra_child = false;
  up_open(&RA_APP, ra_back_page);
}

// ---------- The screen (settings.h calls these) ----------
static void ra_begin() {
  static bool done;
  if (done) return;
  done = true;
  K_RA = halo_spring(HALO_M3X_FAST);
  ra_prefs.begin("rec", false);
  rc_limit_s = ra_prefs.getUChar("limit", RC_MAX_S), rc_level_on = ra_prefs.getBool("level", true);
  bool ok = false;
  for (int l : RA_LIMITS) ok |= l == rc_limit_s;
  if (!ok) rc_limit_s = RC_MAX_S;
  halo_spring_set(ra_sw, rc_level_on);
  ra_mid_mic = halo_text_mid(gl_ra_ic43, RA_IC_MIC), ra_mid_stop = halo_text_mid(gl_ra_ic38, RA_IC_STOP);
  ra_mid_list = halo_text_mid(gl_ra_ic31, RA_IC_LIST), ra_mid_back = halo_text_mid(gl_ra_ic31, RA_IC_BACK);
  ra_mid_play19 = halo_text_mid(gl_ra_ic19, UP_IC_PLAY), ra_mid_dur = halo_text_mid(gl_ra_dur, "0123456789");
}
static void recorder_enter() {
  ra_begin();
  rc_armed = -1, rc_mic_want = true, rc_seen = halo_ms();  // (the microphones warm before the microphone is tapped)
  if (up_app == &RA_PLAY && ra_child) return;              // (back from over the playback: as it was)
  if (up_app != &RA_APP) ra_child = false, up_open(&RA_APP);
}
static void recorder_update(float dt) {
  rc_seen = halo_ms(), rc_active = true, rc_mic_want = true;
  const HaloFinger &f = halo_finger;
  // Recording: capture keeps its page (the pager does not get the swipe — stop first)
  if (rc_state == RC_REC && f.drag_start && f.horiz && f.edge != HALO_EDGE_LEFT) halo_finger_claim(&ra_owner);
  if (ra_child && !rc_has() && rc_state != RC_PLAY) ra_close_clip();  // (the clip went: a new capture began)
  up_update(dt);
}
static void recorder_layout(const HaloView &v) { up_layout(v); }
static void recorder_draw() { up_draw(); }
#endif
