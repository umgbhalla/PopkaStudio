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
#endif
