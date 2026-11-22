// confirm.h — one confirm for the whole watch (Material 3 for Wear OS, the study of Google's images, 23 Nov 2026): an
// icon badge in the accent's deep tone, a short title, a line under it cut to the screen's width, and at the bottom a
// dark rounded square with ✕ beside a larger pill in the accent with ✓ (a dark ✓ of its hue). ✕ or the swipe back —
// no; ✓ — the action (the screen stays: it may say how it went — ui_confirm_title). It opens over the screen it was
// asked from (lib/Halo's stack), as every screen. Power off is the first to use it.
// Included from PopkaWatch.ino after face.h (the gallery's fonts, the stack's levels), before power.h.
#ifdef HALO_ROUND
extern const HaloFont gl_cf_ic;  // ✕ ✓ and the badges' icons (studio.py: GALLERY_FONTS)
#define UI_IC_X "\xEF\x80\x8D"      // xmark
#define UI_IC_CHECK "\xEF\x80\x8C"  // check
#define UI_IC_POWER "\xEF\x80\x91"  // power-off
#define UI_IC_TRASH "\xEF\x87\xB8"  // trash
#define UI_IC_RESET "\xEF\x8B\xB1"  // rotate (reset)

struct UiConfirm {
  const char *icon;   // the badge's glyph (UI_IC_*)
  const char *title;  // one short line ("Power off?")
  const char *body;   // a line under it (nullptr — none), cut with an ellipsis if long
  uint32_t accent;    // the ✓ pill (a pastel) — the badge and the ✓ are its deep tone
  void (*yes)();      // ✓
};

static UiConfirm cf;
static const char *cf_title, *cf_title_was;
static HaloItem cf_it[7];
static float cf_mid_title, cf_mid_body, cf_mid_ic;
static constexpr float CF_BADGE_Y = 116, CF_TITLE_Y = 190, CF_BODY_Y = 232, CF_BTN_Y = 330, CF_BTN_H = 84;
static constexpr float CF_SQ_W = 84, CF_PILL_W = 152, CF_GAP = 16;  // (the square and the pill, centered as a pair)

static uint32_t cf_mix(uint32_t a, uint32_t b, float t) {  // a toward b by t
  uint32_t out = 0;
  for (int s = 0; s <= 16; s += 8) {
    const int ca = a >> s & 255, cb = b >> s & 255;
    out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << s;
  }
  return out;
}
static inline float cf_x0() {
  return CX - (CF_SQ_W + CF_GAP + CF_PILL_W) / 2;
}

static void ui_confirm_update(float) {
  const HaloFinger &f = halo_finger;
  if (!f.tap || f.owner || fabsf(f.y - CF_BTN_Y) > CF_BTN_H / 2 + 8) return;
  const float x0 = cf_x0(), xp = x0 + CF_SQ_W + CF_GAP;
  if (f.x >= x0 - 8 && f.x <= x0 + CF_SQ_W + 8) halo_nav_back();
  else if (f.x >= xp - 8 && f.x <= xp + CF_PILL_W + 8 && cf.yes) cf.yes();
}

static void ui_confirm_layout(const HaloView &v) {
  if (cf_title != cf_title_was) cf_mid_title = halo_text_mid(gl_title, cf_title), cf_title_was = cf_title;
  const uint32_t deep = cf_mix(cf.accent, 0x000000, 0.72f), on_acc = cf_mix(cf.accent, 0x000000, 0.8f);
  HaloItem &badge = cf_it[0], &ic = cf_it[1], &title = cf_it[2], &body = cf_it[3], &sq = cf_it[4], &pill = cf_it[5],
           &glyphs = cf_it[6];
  badge.kind = HI_CIRCLE, badge.x = CX, badge.y = CF_BADGE_Y, badge.r = 34, badge.scale = 1, badge.color = deep;
  badge.alpha = cf.icon ? 1 : 0, badge.on_black = true, badge.on = nullptr;
  gl_text_mid(ic, gl_cf_ic, gl_str(cf.icon), cf_mid_ic, CX, CF_BADGE_Y, 1, cf.accent, cf.icon ? 1 : 0, &badge);
  gl_text_mid(title, gl_title, gl_str(cf_title), cf_mid_title, CX, CF_TITLE_Y, 1, GL_TEXT, 1, &halo_black);
  title.on_black = true, title.fit = 340;
  gl_text_mid(body, gl_t20, gl_str(cf.body), cf_mid_body, CX, CF_BODY_Y, 1, GL_DIM, cf.body ? 1 : 0, &halo_black);
  body.on_black = true, body.fit = 320;
  const float x0 = cf_x0();
  sq.kind = HI_RRECT, sq.x = x0 + CF_SQ_W / 2, sq.y = CF_BTN_Y, sq.w = CF_SQ_W, sq.h = CF_BTN_H, sq.r = 26;
  sq.color = 0x2E3034, sq.alpha = 1, sq.scale = 1, sq.on_black = true, sq.on = nullptr;
  pill.kind = HI_RRECT, pill.x = x0 + CF_SQ_W + CF_GAP + CF_PILL_W / 2, pill.y = CF_BTN_Y, pill.w = CF_PILL_W;
  pill.h = CF_BTN_H, pill.r = CF_BTN_H / 2, pill.color = cf.accent, pill.alpha = 1, pill.scale = 1, pill.on_black = true;
  pill.on = nullptr;
  gl_text_mid(glyphs, gl_cf_ic, UI_IC_CHECK, cf_mid_ic, pill.x, CF_BTN_Y, 1, on_acc, 1, &pill);
  for (HaloItem *it : {&badge, &ic, &title, &body, &sq, &pill, &glyphs}) halo_view_commit(*it, v);
}

static HaloItem cf_x_it;  // (✕ on the square: its own item, after the square)
static void ui_confirm_draw() {
  for (int i = 0; i < 5; i++) halo_item_draw(cf_it[i]);
  halo_item_draw(cf_x_it);
  halo_item_draw(cf_it[5]), halo_item_draw(cf_it[6]);
}
static void ui_confirm_layout_all(const HaloView &v) {
  ui_confirm_layout(v);
  gl_text_mid(cf_x_it, gl_cf_ic, UI_IC_X, cf_mid_ic, cf_x0() + CF_SQ_W / 2, CF_BTN_Y, 1, GL_TEXT, 1, &cf_it[4]);
  halo_view_commit(cf_x_it, v);
}

// Open it over the screen it is asked from
static void ui_confirm_open(const UiConfirm &c) {
  cf = c, cf_title = c.title, cf_title_was = nullptr;
  cf_mid_body = halo_text_mid(gl_t20, gl_str(c.body)), cf_mid_ic = halo_text_mid(gl_cf_ic, UI_IC_CHECK);
  const int d = min(halo_nav_depth() + 1, NAV_LEVELS - 1);
  nav_scr[d] = -1, nav_stub[d] = c.title;
  halo_nav_open({nullptr, ui_confirm_update, ui_confirm_layout_all, ui_confirm_draw});
}
// What it says now (the action's outcome: "Couldn't power off")
static void ui_confirm_title(const char *t) {
  cf_title = t;
}

// ---------- The result: a moment over the screen (M3 for Wear OS: a cookie badge, a label along the bottom edge) ------
// ui_result(true, "Focus done", accent): over the screen for 1.6 s (a tap — at once), then back. The badge — a cookie:
// a disc with eight bumps round it, in the accent — pops in on a spring with a little overshoot, a dark ✓ (or ✕) of the
// accent's hue in it; the label runs along the bottom edge, as Wear OS's confirmations
static bool ur_ok, ur_open;
static const char *ur_label, *ur_arc_of;
static uint32_t ur_accent, ur_until;
static HaloSpring ur_pop;
static HaloSpringK K_UR;
static HaloItem ur_it[11];
static HaloMask ur_arc;
static float ur_ax, ur_ay, ur_mid;

static void ur_update(float dt) {
  halo_spring_run(ur_pop, K_UR, dt);
  const HaloFinger &f = halo_finger;
  if (f.tap && !f.owner) ur_until = halo_ms();
  if (ur_open && (int32_t)(halo_ms() - ur_until) >= 0) ur_open = false, halo_nav_back();
}
static void ur_layout(const HaloView &v) {
  const float s = ur_pop.x, cy = CY - 18;
  const uint32_t on_acc = cf_mix(ur_accent, 0x000000, 0.8f);
  for (int i = 0; i < 9; i++) {  // the cookie: the core, then its eight bumps
    HaloItem &c = ur_it[i];
    const float an = i * (HALO_PI / 4), r = i == 8 ? 58 : 24, d = i == 8 ? 0 : 52;
    c.kind = HI_CIRCLE, c.x = CX + d * s * sinf(an), c.y = cy - d * s * cosf(an), c.r = r * s, c.scale = 1;
    c.color = ur_accent, c.alpha = s > 0.05f ? 1 : 0, c.on_black = true, c.on = nullptr;
  }
  gl_text_mid(ur_it[9], gl_cf_ic, ur_ok ? UI_IC_CHECK : UI_IC_X, cf_mid_ic, CX, cy, 1.7f * s, on_acc, 1, &ur_it[8]);
  if (ur_label != ur_arc_of) {  // (the label along the bottom edge: made into a mask when it changes)
    ur_mid = halo_text_mid(gl_t24, ur_label);
    halo_text_arc(ur_arc, gl_t24, ur_label, ur_mid, 186, HALO_PI, true, 1, &ur_ax, &ur_ay);
    ur_arc_of = ur_label;
  }
  HaloItem &lb = ur_it[10];
  lb.kind = HI_MASK, lb.mask = &ur_arc, lb.color = GL_TEXT, lb.on_black = true, lb.on = &halo_black, lb.scale = 1;
  lb.x = ur_ax + ur_arc.w / 2.0f, lb.y = ur_ay + ur_arc.h / 2.0f, lb.alpha = ur_arc.bits ? halo_clamp01(s * 1.4f - 0.4f) : 0;
  for (HaloItem &it : ur_it) halo_view_commit(it, v);
}
static void ur_draw() {
  for (HaloItem &it : ur_it) halo_item_draw(it);
}
static void ui_result(bool ok, const char *label, uint32_t accent) {
  static bool k;
  if (!k) K_UR = halo_spring(HALO_M3X_DEFAULT), k = true;  // (expressive: a little overshoot)
  ur_ok = ok, ur_label = label, ur_accent = accent, ur_until = halo_ms() + 1600, ur_open = true;
  cf_mid_ic = halo_text_mid(gl_cf_ic, UI_IC_CHECK);
  halo_spring_set(ur_pop, 0.6f), ur_pop.target = 1;
  const int d = min(halo_nav_depth() + 1, NAV_LEVELS - 1);
  nav_scr[d] = -1, nav_stub[d] = label;
  halo_nav_open({nullptr, ur_update, ur_layout, ur_draw});
}

// Every frame (PopkaWatch.ino): what the apps asked to show
static void ui_tick() {
#if HAS_POMODORO
  if (pm_result >= 0) {  // an interval ran out — say so, if Pomodoro is what is on screen
    const int was = pm_result;
    pm_result = -1;
    if (halo_nav_depth() >= 1 && halo_nav_at_home() == false)
      ui_result(true, was == PM_FOCUS ? "Focus done" : "Break over", was == PM_FOCUS ? 0xA4F9B5 : 0xFFB68A);
  }
#endif
}
#else
static void ui_tick() {}
#endif
