// power.h — PopkaStudio's Functions: Always On when idle, and what the BOOT and PWR buttons do. Always On and sleep
// are projects/HaloWatch's power.h one to one (lib/Halo: halo_nav_ambient, halo_sleep; docs/halo.md, section 12); the
// lock is new: on HaloWatch it dimmed the screen and turned the touch off where it was, here it is Always On itself.
//
// Always On (FN_AOD): FN_AOD_IDLE s without touches and motion — the interface glides into Always On in one move, one
// spring (pw_amb: 0 — awake, 1 — Always On). Along it the panel dims from the awake level (pw_awake_level: Display's
// slider, settings.h; else PW_AWAKE) down to FN_AOD_BRIGHT (evenly for
// the eye) and the whole interface shrinks by 5 % around the screen center; on the face the face itself goes there,
// from any other screen it is fade through (halo_nav_ambient). Then the board sleeps and wakes once a minute for one
// frame: new digits, set without rolling, and the face a few pixels elsewhere (AMOLED burn-in: 8 places on a 7 px
// circle; FN_AOD_SHIFT — the editor's Burn-in shift, on by default; off — it stays put). A touch, a button or picking the board up — the same move back, quicker; the waking touch presses nothing.
// Off — the screen stays on. The face's animations (the stars, the pixel art: fc_calm) stop the moment the move into
// Always On starts and go on the moment the move back starts — the same both ways. Back to watch face (FN_AOD_HOME):
// once there (only the face is seen), everything over the face is closed (halo_nav_home_now) — the watch wakes on the
// face, the face itself coming out of Always On; off — on the screen where it was.
//
// The buttons (FN_BOOT, FN_PWR): FN_SCREEN_OFF — the same move to fully dark, the panel off; a touch, a button or
// picking it up — back. FN_AMBIENT — into Always On at once, as when idle (also with Always On off): the same button
// again, a touch or picking it up — back. FN_LOCK — dim and shrink toward the center with touch off. On round boards,
// a short BOOT or PWR press toggles the display while locked, a hold unlocks, and FN_PALM_LOCK enters locked Always On. FN_HOME
// returns to the home screen. PWR can't wake the round board from light sleep (its
// chip's interrupt is not wired) — on battery PWR is read when the board runs again (for example, when picked up).
// On USB the board does not really sleep (Halo) — the same screens, the frames just go on.
//
// Waking up (FN_WAKE_TAP, FN_WAKE_RAISE, FN_WAKE_BTN — the editor's Always On → Wake up): what brings the screen back
// from Always On or dark — a touch, picking the watch up (the motion sensor; off — the sensor is not even started,
// unless the Steps layer needs it — then the pedometer has it and motion never wakes the board), a button (BOOT or PWR:
// in Always On and dark any press only wakes, its action is for an awake watch). A way that is off is not a way: the
// board, woken by it, goes straight back to sleep; motion with Raise off is not activity either. At least one is on
// (the editor and studio.py see to it). This setting controls unlocked wake-ups.
//
// The editor writes the settings into layers.h; its page changes them on the fly (engine/studio_api.h: studio_fn).
// Commands: aod on|off, aod N (the panel in Always On, of 255), aod home on|off (Back to watch face), aod shift on|off
// (Burn-in shift), idle N (s), sleep
// (Always On now), wake (as a touch);
// lib/Halo's boot / boot long, pwr / pwr long — the buttons.
// Included from PopkaWatch.ino after face.h.
#include <sys/time.h>

#ifndef FN_AOD  // layers.h has them when the editor wrote it
#define FN_AOD 1
#define FN_AOD_IDLE 15
#define FN_AOD_BRIGHT 24
#define FN_BOOT 1
#define FN_PWR 2
#endif
#ifndef FN_AOD_HOME
#define FN_AOD_HOME 0  // out of Always On on the face (1) or where it was (0)
#endif
#ifndef FN_AOD_SHIFT
#define FN_AOD_SHIFT 1  // Always On: a few pixels elsewhere every minute (AMOLED burn-in)
#endif
#ifndef FN_PALM_LOCK
#define FN_PALM_LOCK 0
#endif
#ifndef FN_WAKE_TAP
#define FN_WAKE_TAP 1
#define FN_WAKE_RAISE 1
#define FN_WAKE_BTN 1
#endif
enum { FN_NOTHING, FN_SCREEN_OFF, FN_LOCK, FN_AMBIENT, FN_HOME, FN_BACK };  // what a button does (Back — a level back,
// then home; Home — straight to the clock; on the face both open Apps)
#define PW_AWAKE 180        // the panel awake: HaloWatch's default brightness
#define PW_AOD_SCALE 0.95f  // Always On and dark: everything 5 % smaller
#define PW_LISTEN_S 3       // locked: awake this long after a wake-up, for the button's long press

enum { PW_ACTIVE, PW_FADING, PW_AOD, PW_DARK };
static int pw_mode = PW_ACTIVE;
static bool pw_to_aod;        // where the fade goes: Always On or dark
static bool pw_aod_way;       // the way the spring left zero: to Always On (the brightness's curve, the face at the end)
static HaloSpring pw_amb;     // 0 — awake, 1 — Always On or dark
static HaloSpringK K_PW_OUT, K_PW_IN, K_PW_SHIFT;
static int64_t pw_active_us;  // the last touch or motion
static int64_t pw_listen_us;  // locked: stay up until then
static uint32_t pw_motion;
static int pw_sent = -1;      // the brightness last sent to the panel
static bool pw_locked;
static int pw_lock_btn;       // what locked: 0 — palm, 1 — BOOT or shade, 2 — PWR
#ifndef HALO_ROUND
static uint32_t pw_lock_ms;   // locked then: a long press of that same press does not unlock
#endif
static HaloSpring pw_shift_amount = {1, 0, 1};  // 0 during lock and its wake-up, 1 for regular Always On
static volatile bool pw_cmd_sleep, pw_cmd_wake;
static volatile bool pw_debug;  // debug on: never idle into Always On or dark, no palm lock (testing; not kept)
static volatile int pw_aod_on = FN_AOD, pw_idle_s = FN_AOD_IDLE, pw_aod_level = FN_AOD_BRIGHT, pw_aod_home = FN_AOD_HOME;
static volatile int pw_aod_shift = FN_AOD_SHIFT;
static volatile int pw_boot = FN_BOOT, pw_pwr = FN_PWR;
#ifdef HALO_ROUND
static volatile bool pw_palm_lock = FN_PALM_LOCK;
static constexpr int64_t PW_LOCK_DIM_US = 120000000;
static constexpr int64_t PW_TAP_US = 100000, PW_UNLOCK_US = 500000, PW_UNLOCK_FADE_US = 150000;
static constexpr int64_t PW_RING_US = PW_TAP_US + (PW_UNLOCK_US - PW_TAP_US) / 4;
static int64_t pw_dim_until;
static uint8_t pw_unlock_keys, pw_unlock_btn;  // captured through release, even after unlocking
static int64_t pw_unlock_down_us, pw_unlock_exit_us;
static bool pw_unlock_was_dark;
static bool pw_pwr_release_captured;  // a PMIC click may arrive separately from the release edge
static float pw_unlock_progress, pw_unlock_fade;
static HaloSpring pw_unlock_in;
static HaloSpringK K_PW_UNLOCK;
static int pw_shutdown;
#endif
static int pw_boot_fn() {  // (settings.h: Display's hint)
  return pw_boot;
}
static volatile int pw_wake_tap = FN_WAKE_TAP, pw_wake_raise = FN_WAKE_RAISE, pw_wake_btn = FN_WAKE_BTN;
#define PW_WAKE_CMD 5  // woken by the command wake (the editor entering Try): always

// Does this wake-up (HALO_WAKE_*) bring the screen back — is its way on
static bool pw_wakes(uint8_t w) {
  return w == HALO_WAKE_TOUCH ? pw_wake_tap : w == HALO_WAKE_MOTION ? pw_wake_raise : w == HALO_WAKE_BOOT ? pw_wake_btn : true;
}

// Asleep until something wants the board (settings_wake_ms: Pomodoro's interval ending) — or until ms, if sooner (0 — no
// timer of its own)
static void pw_sleep(uint32_t ms = 0) {
  const uint32_t want = settings_wake_ms();
  halo_sleep(want && (!ms || want < ms) ? want : ms);
}

static uint32_t pw_ms_to_minute() {  // until the next minute, +30 ms to be sure it has turned
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return 60000 - (uint32_t)((tv.tv_sec % 60) * 1000 + tv.tv_usec / 1000) + 30;
}

// The panel's brightness: awake — PW_AWAKE; to Always On — down to its level, every frame the same share less (the eye
// sees brightness about as a logarithm); to dark — to zero along a square (the fade's end is the slow part)
static int pw_level(float awake) {
  const float k = pw_amb.x;
  if (k <= 0 || awake < 1) return (int)lroundf(awake);
  if (!pw_aod_way) return (int)lroundf(awake * (1 - k) * (1 - k));
  const float lo = fminf(awake, (float)pw_aod_level);
  return (int)lroundf(awake * powf(lo / awake, k));
}

static void pw_fade(bool to_aod) {
  if (pw_amb.x == 0) pw_aod_way = to_aod;
  pw_mode = PW_FADING, pw_to_aod = to_aod, pw_amb.target = 1;
}

static void pw_wake(uint8_t w) {  // the screen where it was: back along the same spring
  static const char *const NAMES[] = {"button", "touch", "BOOT", "motion", "timer", "command"};
  LOG("power: woke by %s\n", NAMES[w < 6 ? w : 0]);
  pw_mode = PW_ACTIVE, pw_amb.target = 0, pw_active_us = esp_timer_get_time();
  halo_wake();  // woken by a button the chip reads or a command — lib/Halo would still take the next touch for a wake-up
  face_still(false);
  if (!halo_display_is_on()) halo_display_on(true);  // dark: the panel comes on at zero brightness
}

static void pw_lock_set(bool on, int btn) {
  pw_locked = on, pw_lock_btn = btn;
#ifdef HALO_ROUND
  pw_dim_until = on ? esp_timer_get_time() + PW_LOCK_DIM_US : 0;
  pw_unlock_btn = 0;
  if (on) {
    pw_unlock_exit_us = 0;
    pw_unlock_was_dark = false;
    pw_unlock_progress = pw_unlock_fade = 0;
    halo_spring_set(pw_unlock_in, 0);
  } else if (pw_unlock_in.target > 0 && !pw_unlock_exit_us) pw_unlock_exit_us = esp_timer_get_time();
#else
  pw_lock_ms = halo_ms();
#endif
  if (on) {
    if (pw_amb.x == 0 || !pw_aod_way || !halo_display_is_on()) halo_spring_set(pw_shift_amount, 0);
    else pw_shift_amount.target = 0;  // recenter smoothly if idle Always On had already shifted the face
  }
  halo_touch_on(!on);
  LOG("%s\n", on ? "locked" : "unlocked");
}

#ifdef HALO_ROUND
static void pw_locked_display(bool on) {
  pw_dim_until = on ? esp_timer_get_time() + PW_LOCK_DIM_US : 0;
  pw_mode = on ? PW_AOD : PW_DARK;
  pw_to_aod = pw_aod_way = on;
  halo_spring_set(pw_amb, 1);
  halo_spring_set(pw_shift_amount, 0);
  if (on) {
    halo_wake();
    face_still(true);
    fc_check = true;
    if (pw_aod_home) halo_nav_home_now();
  }
  halo_display_on(on);
  LOG("locked display: %s\n", on ? "on" : "off");
}
#endif

// A button pressed (1 — BOOT, 2 — PWR; held — its long press) with what it does
static void pw_button(int btn, int fn, bool held, int64_t now) {
  if ((fn == FN_HOME || fn == FN_BACK) && !held && !pw_locked && pw_mode != PW_AOD && pw_mode != PW_DARK) {  // (asleep or locked: below)
    const bool animate = !pw_locked && pw_mode == PW_ACTIVE && pw_amb.x == 0;
#ifdef HALO_ROUND
    bool open_apps = btn == 2 && animate && halo_nav_on_face();
#if HAS_SHADE
    open_apps = open_apps && sh_px <= 0;
#endif
#endif
#if HAS_SHADE && defined(HALO_ROUND)
    const bool shade_was = sh_px > 0;
    if (animate) shade_dismiss();
    else shade_close();
#else
    const bool shade_was = false;
#endif
#ifdef HALO_ROUND
    // A pseudo back (the user's word, 23 Nov 2026): inside an app or a page opened over one — back a level; on the
    // Apps list itself, over the face, or with quick settings out — home to the clock (the face: Apps, as before)
    if (fn == FN_BACK && animate && !shade_was && !open_apps && (halo_nav_depth() > 0 || halo_nav_over())) {
      halo_nav_back();
      if (!pw_locked) pw_wake(0);
      return;
    }
    if (open_apps) halo_nav_open_apps();
    else
#endif
    if (animate) halo_nav_home(btn == 2);
    else halo_nav_home_now();
    if (!pw_locked) pw_wake(0);
    return;
  }
  // Locked, either button (the user's rule, 23 Nov 2026): a press — the locked screen on, the next one off (a cycle);
  // held — unlocked and awake
  if (pw_locked) {
    if (held) {
#ifdef HALO_ROUND
      // Serial/preview commands can request a complete hold directly.
      pw_lock_set(false, 0), pw_wake(0);
#else
      if (halo_ms() - pw_lock_ms > HALO_BOOT_LONG_MS + 300) pw_lock_set(false, 0), pw_wake(0);  // (not the hold that locked)
#endif
      return;
    }
    pw_listen_us = now + PW_LISTEN_S * 1000000ll;
#ifdef HALO_ROUND
    pw_locked_display(!halo_display_is_on());
#endif
    return;
  }
  // Asleep (Always On or dark), either button, pressed or held: awake — nothing else
  if (pw_mode == PW_AOD || pw_mode == PW_DARK) {
    pw_wake(0);
    return;
  }
  if (held) return;
  if (fn == FN_LOCK) {  // into Always On, locked — from anywhere (from dark too: the panel on, straight to its level)
    pw_lock_set(true, btn);
#ifdef HALO_ROUND
    if (pw_mode == PW_DARK) {
      pw_listen_us = now + PW_LISTEN_S * 1000000ll;
      pw_locked_display(true);
      return;
    }
#endif
    if (!halo_display_is_on()) halo_display_on(true);
    if (pw_mode != PW_AOD) pw_aod_way = true, pw_fade(true);
  } else if (fn == FN_SCREEN_OFF) {
    if (pw_mode == PW_ACTIVE || (pw_mode == PW_FADING && pw_to_aod)) pw_fade(false);  // to dark (from anywhere on the way)
    else pw_wake(0);  // dark, Always On, or fading to dark — back
  } else if (fn == FN_AMBIENT) {
    if (pw_mode == PW_ACTIVE && (pw_amb.x == 0 || pw_aod_way)) pw_fade(true);  // into Always On, as when idle
    else pw_wake(0);  // Always On, dark, or on the way there — back
  }
}

#ifdef HALO_ROUND
static void pw_unlock_hide() {
  if (pw_locked && pw_unlock_was_dark) pw_locked_display(false);
  pw_unlock_was_dark = false;
  pw_unlock_exit_us = 0;
  pw_unlock_progress = pw_unlock_fade = 0;
  halo_spring_set(pw_unlock_in, 0);
}

// The first key down owns the gesture. Absorb the release and any legacy click/hold for captured keys.
static bool pw_unlock_key(int btn, uint8_t events, int64_t now) {
  const uint8_t edges = events & (HALO_KEY_DOWN | HALO_KEY_UP);
  bool captured = pw_unlock_keys & btn;
  if (!captured && edges == (HALO_KEY_DOWN | HALO_KEY_UP)) return false;
  if (pw_locked && (edges & HALO_KEY_DOWN) && !captured) {
    pw_unlock_keys |= btn;
    captured = true;
    if (!pw_unlock_btn) {
      pw_unlock_btn = btn;
      pw_unlock_down_us = now;
      pw_listen_us = now + PW_LISTEN_S * 1000000ll;
      halo_wake();
    }
  }
  if (!(edges & HALO_KEY_UP) || !captured) return captured;
  pw_unlock_keys &= ~btn;
  if (pw_unlock_btn != btn) return true;
  const int64_t held = now - pw_unlock_down_us;
  if (held >= PW_UNLOCK_US) return true;  // complete in tick even if release and deadline share this frame
  pw_unlock_btn = 0;
  if (held < PW_RING_US) {
    if (pw_unlock_exit_us) pw_unlock_hide();
    pw_button(btn, FN_NOTHING, false, now);
  } else if (pw_unlock_in.target > 0 && !pw_unlock_exit_us) {
    pw_unlock_exit_us = now;
  }
  return true;
}

static void pw_unlock_tick(float dt, int64_t now) {
  if (pw_unlock_exit_us) {
    const float t = halo_clamp01((now - pw_unlock_exit_us) / (float)PW_UNLOCK_FADE_US);
    pw_unlock_fade = t * t * (3 - 2 * t);
    if (t < 1) return;
    pw_unlock_hide();
  }
  if (!pw_unlock_btn) return;
  const int64_t held = now - pw_unlock_down_us;
  if (held < PW_RING_US) return;
  if (pw_unlock_in.target == 0) {
    pw_unlock_was_dark = !halo_display_is_on();
    if (pw_unlock_was_dark) pw_locked_display(true);
    pw_unlock_in.target = 1;
  }
  pw_unlock_progress = halo_clamp01((held - PW_RING_US) / (float)(PW_UNLOCK_US - PW_RING_US));
  halo_spring_run(pw_unlock_in, K_PW_UNLOCK, dt);
  if (held < PW_UNLOCK_US) return;
  halo_spring_set(pw_unlock_in, 1);
  pw_unlock_exit_us = now;
  pw_lock_set(false, 0);
  pw_wake(0);
}


// Hold to unlock: the whole rim lit from the edge inwards — the overscroll glow's look (ui.h ui_glow: light fading in
// from the edge) all round, and the hold deepens it: at the start a thin bright edge, then the light reaches further in
// and grows stronger until, held through, the rim glows ~48 px deep. (The user, 28 Nov 2026: not a sweep round the
// circle — "the full circle, from a low edge to a deeper edge effect"; the sweep also cost an angle per pixel every
// frame, a slow frame.) Cheap: per row only the lit ring's two spans; per pixel one table look-up (depth by r², then
// alpha by depth — that table is made once a frame, in tick).
#define PW_GLOW_DMAX 48                      // the deepest the light reaches, px
static constexpr uint32_t PW_GLOW_C = 0xFFF8EE;  // house text white
static uint8_t pw_glow_d4[1280];             // depth, quarter px (0 … 192), by (4r² − 4r_in²) >> 6
static int pw_glow_r2in;                     // 4r² of the deepest edge
static uint8_t pw_glow_a[PW_GLOW_DMAX * 4 + 1];  // this frame's alpha (0 … 255) by depth in quarter px
static float pw_glow_reach;                  // this frame's reach, px (where alpha falls under 1/255)
static void pw_glow_make() {
  if (pw_glow_r2in) return;
  const float rin = CX - PW_GLOW_DMAX;
  pw_glow_r2in = (int)(4 * rin * rin);
  for (int k = 0; k < 1280; k++) {
    const float d = CX - sqrtf((pw_glow_r2in + (k << 6) + 32) / 4.0f);
    pw_glow_d4[k] = (uint8_t)fmaxf(0, fminf(PW_GLOW_DMAX * 4, d * 4 + 0.5f));
  }
}
// The light for a hold's progress p (0 … 1) at strength `alpha`: its fall-off length grows 2.5 → 13 px, its edge 45 → 95 %
static void pw_glow_frame(float p, float alpha) {
  pw_glow_make();
  const float tau = 2.5f + 10.5f * p, edge = (0.45f + 0.5f * p) * alpha;
  pw_glow_reach = fminf(PW_GLOW_DMAX, tau * logf(fmaxf(1.0f, edge * 255)));
  for (int q = 0; q <= PW_GLOW_DMAX * 4; q++) pw_glow_a[q] = (uint8_t)(255 * edge * expf(-(q / 4.0f) / tau) + 0.5f);
}

static float pw_unlock_alpha() { return halo_clamp01(pw_unlock_in.x) * (1 - pw_unlock_fade); }

// The lit ring's rows are redrawn while it shows (as deep as it reaches now or reached last frame — the retained
// framebuffer clears what it leaves); the frame's light is set here, before the bands draw
static void pw_unlock_damage() {
  static float was;
  const float alpha = pw_unlock_alpha();
  if (alpha > 0) pw_glow_frame(pw_unlock_exit_us && pw_unlock_progress >= 1 ? 1.0f : pw_unlock_progress, alpha);
  const float reach = alpha > 0 ? pw_glow_reach : 0;
  if (!was && !reach) return;
  const float outer = CX + 1, inner = CX - fmaxf(was, reach) - 2;
  was = reach;
  for (int y = 0; y < HALO_H; y += HALO_ROWS) {
    const int bottom = min(y + HALO_ROWS - 1, HALO_H - 1);
    const float near = fmaxf(0, fmaxf(y - CY, CY - bottom - 1));
    const float far = fmaxf(fabsf(y - CY), fabsf(bottom + 1 - CY));
    const float outside = sqrtf(fmaxf(0, outer * outer - near * near));
    const float inside = sqrtf(fmaxf(0, inner * inner - far * far));
    halo_damage((int)(CX - outside), y, (int)ceilf(CX - inside), bottom);
    halo_damage((int)(CX + inside), y, (int)ceilf(CX + outside), bottom);
  }
}

static inline void pw_glow_px(uint16_t *px, int A, int cr, int cg, int cb, int d) {
  const uint32_t v = HALO_PX(*px);
  const int R = (v >> 11) << 4, G = (v >> 5 & 63) << 4, B = (v & 31) << 4;
  const int r5 = min(31, (R + ((cr - R) * A >> 8) + d) >> 4), g6 = min(63, (G + ((cg - G) * A >> 8) + d) >> 4),
            b5 = min(31, (B + ((cb - B) * A >> 8) + d) >> 4);
  *px = HALO_PX((r5 << 11) | (g6 << 5) | b5);
}
static void power_draw() {
  if (pw_unlock_alpha() <= 0 || pw_glow_reach <= 0) return;
  const int cr = (PW_GLOW_C >> 16 & 255) * 31 * 16 / 255, cg = (PW_GLOW_C >> 8 & 255) * 63 * 16 / 255,
            cb = (PW_GLOW_C & 255) * 31 * 16 / 255;
  static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
  const float rin = CX - pw_glow_reach;
  for (int y = hb.y; y < hb.y + hb.rows; y++) {
    const float dy = y + 0.5f - CY, ho2 = CX * CX - dy * dy;
    if (ho2 <= 0) continue;
    const float hi2 = rin * rin - dy * dy, ho = sqrtf(ho2), hi = hi2 > 0 ? sqrtf(hi2) : 0;
    uint16_t *row = halo_row(y);
    const uint8_t *by = BY + (y & 3) * 4;
    const int Y2 = 2 * y + 1 - HALO_H, yy = Y2 * Y2;
    // the lit ring's two spans on this row: left [CX − ho, CX − hi], right [CX + hi, CX + ho]
    const int sp[2][2] = {{(int)(CX - ho), (int)ceilf(CX - hi)}, {(int)(CX + hi), (int)ceilf(CX + ho)}};
    for (int k = 0; k < 2; k++) {
      if (hi <= 0 && k) break;  // (no hole on this row: one span, the whole chord)
      const int x0 = max(hb.x0, sp[k][0]), x1 = min(hb.x1, hi <= 0 ? sp[1][1] : sp[k][1]);
      for (int x = x0; x <= x1; x++) {
        const int X2 = 2 * x + 1 - W, i = (X2 * X2 + yy - pw_glow_r2in) >> 6;
        if (i < 0) continue;
        const int A = pw_glow_a[pw_glow_d4[min(1279, i)]];
        if (A) pw_glow_px(row + x, A, cr, cg, cb, by[x & 3]);
      }
    }
  }
}
#endif

// Every frame, first (before the face: Always On wants its digits without rolling)
static void power_tick(float dt) {
  const HaloFinger &f = halo_finger;
  const int64_t now = esp_timer_get_time();
  const bool moved = pw_wake_raise && halo_motion_count() != pw_motion;  // (Raise off: motion is nothing)
  if (f.down || moved || settings_busy()) pw_active_us = now;  // touched, moved, recording or playing — not idle
  pw_motion = halo_motion_count();
  uint8_t w = halo_woke();
#ifdef HALO_ROUND
  const bool boot_locks = pw_boot == FN_LOCK;
  const bool boot = halo_boot_short(pw_locked || boot_locks);
  const bool boot_held = halo_boot_long(!pw_locked);
  if (w == HALO_WAKE_BOOT && (pw_locked || boot_locks)) {
    pw_listen_us = now + PW_LISTEN_S * 1000000ll;
    w = 0;  // wait for the short or long press before changing display state
  }
#else
  const bool boot = halo_boot_click();
  const bool boot_held = halo_boot_long();
#endif
#ifdef HALO_ROUND
  if (halo_take_palm() && pw_palm_lock && !pw_locked && !pw_debug) {
    halo_finger = {};
    pw_lock_set(true, 0);
    pw_locked_display(true);  // (the locked Always On screen, not black: the user's call, 23 Nov 2026)
    // Drain a release for a previously captured button before the palm consumes this frame.
    if (halo_boot_edges() & HALO_KEY_UP) pw_unlock_keys &= ~1;
    if (halo_pwr_key_event(true) & HALO_KEY_UP) pw_unlock_keys &= ~2;
    return;
  }
#endif
  if (pw_cmd_wake) {  // as a touch: asleep — wake, fading out — back, awake — idle from now
    pw_cmd_wake = false, pw_active_us = now;
    if (!w && (pw_mode == PW_AOD || pw_mode == PW_DARK)) w = PW_WAKE_CMD;
    else if (pw_mode == PW_FADING && !pw_locked) pw_mode = PW_ACTIVE, pw_amb.target = 0;
  }
  if (settings_due() && (pw_mode == PW_AOD || pw_mode == PW_DARK || (pw_mode == PW_FADING && !pw_locked))) {
    // Pomodoro's interval ran out: the screen comes back for the alert (locked — the lock stays, the board stays up a
    // while for the sound); this frame's tick plays it
    if (pw_locked) pw_listen_us = now + PW_LISTEN_S * 1000000ll;
    else pw_wake(4), w = 0;
  }
  if (halo_finger_blocked() && pw_locked) pw_listen_us = now + PW_LISTEN_S * 1000000ll;
#ifdef HALO_ROUND
  const uint8_t pwr = halo_pwr_key_event(pw_locked || (pw_unlock_keys & 2));
  const uint8_t boot_edges = halo_boot_edges();
  const bool boot_captured = pw_unlock_key(1, boot_edges, now);
  const bool pwr_captured = pw_unlock_key(2, pwr, now);
  if (pwr & HALO_KEY_DOWN) pw_pwr_release_captured = false;
  if (pwr & HALO_KEY_UP) pw_pwr_release_captured = pwr_captured;
  const bool boot_click = boot && (!pw_locked || !(boot_edges & HALO_KEY_UP));
  if (!boot_captured && (boot_click || boot_held)) pw_button(1, pw_boot, boot_held, now);
  const bool pwr_command = pwr & HALO_KEY_COMMAND;
  const bool pwr_held = (pwr & HALO_PWR_LONG) && (pwr_command || (!pw_locked && !pw_pwr_release_captured));
  // If both PMIC edges arrived between two polls, its short flag is the tap classification.
  const bool pwr_tap = (pwr & (HALO_KEY_DOWN | HALO_KEY_UP)) == (HALO_KEY_DOWN | HALO_KEY_UP);
  const bool pwr_click = (pwr & HALO_PWR_CLICK) && (pwr_command || (pw_locked ? pwr_tap : !pw_pwr_release_captured));
  if (!pwr_captured && (pwr_click || pwr_held)) pw_button(2, pw_pwr, pwr_held, now);
  pw_unlock_tick(dt, now);
  if (pw_locked && !pw_unlock_btn && !pw_unlock_exit_us && pw_dim_until && now >= pw_dim_until) pw_locked_display(false);
#else
  const uint8_t pwr = halo_pwr_event();
  if (boot || boot_held) pw_button(1, pw_boot, boot_held, now);
  if (pwr) pw_button(2, pw_pwr, pwr & HALO_PWR_LONG, now);
#endif
  switch (pw_mode) {
    case PW_ACTIVE:
      if (!pw_locked && (pw_cmd_sleep || (!pw_debug && pw_aod_on && pw_idle_s && now - pw_active_us > pw_idle_s * 1000000ll)) &&
          (pw_amb.x == 0 || pw_aod_way))
        pw_fade(true);
      break;
    case PW_FADING:
      // Touched or picked up while fading to Always On — the screen comes back (the touch is off when locked; a button
      // pressed in a hand moves the board too — the dark fade is not cancelled by that)
      if (!pw_locked && ((f.down && pw_wake_tap) || (pw_to_aod && moved))) {
        pw_mode = PW_ACTIVE, pw_amb.target = 0, pw_active_us = now;
      } else if (pw_amb.x > 0.995f || (!pw_aod_way && pw_level(pw_awake_level) == 0)) {  // there: the rest is not seen
        halo_spring_set(pw_amb, 1);
        if (pw_to_aod) {
          pw_mode = PW_AOD;
          face_still(true);
          if (pw_aod_home) halo_nav_home_now();  // Back to watch face: only the face is seen now — close the rest
        } else {
          pw_mode = PW_DARK;
          halo_display_on(false), LOG("screen off\n");
          pw_sleep();
        }
      }
      break;
    case PW_AOD:
      if (w && w != HALO_WAKE_TIMER && !pw_locked && pw_wakes(w)) {
        pw_wake(w);
      } else {
        if (w == HALO_WAKE_TIMER) fc_check = true;  // a new minute: face_tick sets the digits
        if (w && pw_locked) pw_listen_us = now + PW_LISTEN_S * 1000000ll;  // locked: up a while, for the button
        if (!halo_asleep() && now >= pw_listen_us) {
          uint32_t sleep_ms = pw_ms_to_minute();
#ifdef HALO_ROUND
          if (pw_locked && pw_dim_until > now) sleep_ms = min(sleep_ms, (uint32_t)((pw_dim_until - now + 999) / 1000));
#endif
          pw_sleep(sleep_ms);
        }
      }
      break;
    case PW_DARK:
      if (w && !pw_locked && pw_wakes(w)) pw_wake(w);
      else if ((w || pw_locked) && !halo_asleep() && now >= pw_listen_us) pw_sleep();
      break;
  }
  pw_cmd_sleep = false;  // sent while already asleep — it must not put the board back to sleep right after it wakes
  fc_calm = pw_mode == PW_AOD || (pw_mode == PW_FADING && pw_to_aod);  // the animations: still from the move's start
  halo_spring_run(pw_amb, pw_amb.target > 0.5f ? K_PW_IN : K_PW_OUT, dt);
  if (!pw_locked && pw_amb.x == 0) halo_spring_set(pw_shift_amount, 1);
  else halo_spring_run(pw_shift_amount, K_PW_SHIFT, dt);
  const int level = pw_level(pw_awake_level);
  if (level != pw_sent) halo_brightness(level), pw_sent = level;
}

// After face_tick (its minute is fresh), before halo_nav_update: the whole interface's view along the spring — 5 %
// smaller, and in Always On a few pixels elsewhere every minute (where — kept while the spring moves: a minute that
// turns on the way would make the face jump)
static void power_view() {
#ifdef HALO_ROUND
  pw_unlock_damage();
#endif
  static const int8_t SX[8] = {0, 5, 7, 5, 0, -5, -7, -5}, SY[8] = {-7, -5, 0, 5, 7, 5, 0, -5};
  static int spot;
  if (fc_min >= 0 && (pw_amb.x == 0 || (pw_mode == PW_AOD && !pw_locked))) spot = fc_min & 7;
  const float k = pw_amb.x, m = pw_aod_way && pw_aod_shift ? k * pw_shift_amount.x : 0;
  if (m > 0 && pw_shift_amount.x != pw_shift_amount.target) halo_subpixel = true;
  const HaloView view = {SX[spot] * m, SY[spot] * m, 1 + (PW_AOD_SCALE - 1) * k, 1, false};
  halo_nav_ambient(k, view, pw_aod_way);
}

#ifdef HALO_ROUND
// Power off: the shared confirm (confirm.h) — ✓ asks the power chip (power_shutdown_poll, from loop()); if it says no,
// the confirm says so
static void power_confirm_open() {
  __atomic_store_n(&pw_shutdown, 0, __ATOMIC_RELAXED);
  ui_confirm_open({UI_IC_POWER, "Power off?", "Press PWR to turn it back on", 0xFFB4AB, [] {
                     if (__atomic_load_n(&pw_shutdown, __ATOMIC_RELAXED) != 1) __atomic_store_n(&pw_shutdown, 1, __ATOMIC_RELAXED);
                   }});
}

static void power_shutdown_poll() {
  if (__atomic_load_n(&pw_shutdown, __ATOMIC_RELAXED) != 1) return;
  LOG("power: shutdown confirmed\n");
  const bool accepted = halo_power_off();
  __atomic_store_n(&pw_shutdown, accepted ? 2 : 3, __ATOMIC_RELAXED);
  if (!accepted) ui_confirm_title("Couldn't power off");
#ifdef __EMSCRIPTEN__
  if (accepted) halo_display_on(false);
#endif
}
#endif

static void power_build() {  // after halo_begin: the motion sensor is on its I2C bus
  K_PW_IN = halo_spring(0.8f, 1);   // to Always On — calm: ~0.5 s most of the way
  K_PW_OUT = halo_spring(0.5f, 1);  // awake — quicker: ~0.3 s
  K_PW_SHIFT = halo_spring(0.3f, 1);
#ifdef HALO_ROUND
  K_PW_UNLOCK = halo_spring(0.20f, 0.8f);
#endif
#if FN_WAKE_RAISE || FN_STEPS || defined(__EMSCRIPTEN__)
  halo_motion_begin(150);  // (Raise off and no Steps — the motion sensor stays off)
#endif
  pw_active_us = esp_timer_get_time();
  halo_command("aod", [](const char *a) {
    if (isdigit((unsigned char)*a)) pw_aod_level = constrain(atoi(a), 1, 255), LOG("aod: brightness %d\n", pw_aod_level);
    else if (!strncmp(a, "home", 4)) pw_aod_home = !strstr(a, "off"), LOG("aod: back to face %s\n", pw_aod_home ? "on" : "off");
    else if (!strncmp(a, "shift", 5)) pw_aod_shift = !strstr(a, "off"), LOG("aod: burn-in shift %s\n", pw_aod_shift ? "on" : "off");
    else pw_aod_on = strcmp(a, "off") != 0, LOG("aod: %s\n", pw_aod_on ? "on" : "off");
  });
  // debug on | off — for testing over USB: the screen stays awake (no idle fade, no palm lock) until debug off or a
  // restart; the settings are untouched
  halo_command("debug", [](const char *a) {
    pw_debug = strcmp(a, "off") != 0, pw_active_us = esp_timer_get_time();
    LOG("debug: %s\n", pw_debug ? "on — the screen stays awake" : "off");
  });
  halo_command("idle", [](const char *a) { pw_idle_s = max(0, atoi(a)), LOG("idle: %d s\n", pw_idle_s); });
  halo_command("sleep", [](const char *) { pw_cmd_sleep = true; });
  halo_command("wake", [](const char *) { pw_cmd_wake = true; });
  // unlockpeek 0..1 | off — the hold-to-unlock ring at that progress, without unlocking (to see it over serial)
#ifdef HALO_ROUND
  halo_command("unlockpeek", [](const char *a) {
    if (!strcmp(a, "off")) pw_unlock_hide();
    else pw_unlock_progress = halo_clamp01(atof(a)), halo_spring_set(pw_unlock_in, 1), pw_unlock_in.target = 1;
  });
#endif
  halo_command("power", [](const char *) {
    LOG("power: %s, display %s\n", pw_locked ? "locked" : "unlocked", halo_display_is_on() ? "on" : "off");
  });
}

#if HAS_SHADE
// The quick settings panel (shade_rect.h): Always On now, the Buttons' Lock and Screen off, Wi-Fi's "sync now"
static bool shade_aod_is_on() {
  return pw_aod_on;
}
static void shade_power(int what) {
  pw_active_us = esp_timer_get_time();
  if (what == SI_AOD) {
    pw_aod_on = !pw_aod_on, LOG("aod: %s (panel)\n", pw_aod_on ? "on" : "off");
  } else if (what == SI_LOCK) {  // as the button's Lock: Always On, the touch off; a long BOOT unlocks
    pw_lock_set(true, 1);
    if (pw_mode != PW_AOD) pw_aod_way = true, pw_fade(true);
  } else if (what == SI_OFF) {
    pw_fade(false);
  } else if (what == SI_WIFI) {
#if FN_WIFI && !defined(__EMSCRIPTEN__)
    if (hclk_handle) xTaskNotifyGive(hclk_handle);  // (as the command ntp: the time and the weather now)
#endif
  }
}
#endif
