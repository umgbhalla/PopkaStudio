// What every ESP32 port's frame engine has the same (the S3's halo_s3_frame.h, the P4's halo_p4_frame.cpp): the band
// being drawn, the damage lists, animation time, the display's requests, the report's tail and the commands every board
// takes. Not a header to share declarations: it holds the definitions themselves, so only a port's frame engine includes
// it — once, inside its board's #if, after its board header and its optimize pragma.
#pragma once
#include "halo_spans.h"

HALO_TLS HaloBand hb;
uint32_t hf_finger_t;

static HaloConfig cfg;
static HaloUpdate m_update;
static HaloDraw m_draw;

// ---------- Что изменилось: по пачке — до HALO_SPANS отрезков столбцов (halo_spans.h) ----------
static HaloSpans dmg[HALO_BANDS];  // n = 0 — пачка чистая
static portMUX_TYPE dmg_mux = portMUX_INITIALIZER_UNLOCKED;

void halo_damage(int x0, int y0, int x1, int y1) {
  x0 = max(x0, 0), y0 = max(y0, 0), x1 = min(x1, HALO_W - 1), y1 = min(y1, HALO_H - 1);
  if (x0 > x1 || y0 > y1) return;
  portENTER_CRITICAL(&dmg_mux);
  for (int b = y0 / HALO_ROWS; b <= y1 / HALO_ROWS; b++) halo_spans_add(dmg[b], x0, x1);
  portEXIT_CRITICAL(&dmg_mux);
}

void halo_damage_f(float x0, float y0, float x1, float y1) {
  halo_damage(halo_floor(x0) - 1, halo_floor(y0) - 1, halo_floor(x1) + 2, halo_floor(y1) + 2);
}

void halo_damage_all() {
  halo_damage(0, 0, HALO_W - 1, HALO_H - 1);
}

// ---------- Время анимаций ----------
static uint64_t m_clock;
static volatile bool vt_paused, vt_fixed;
static volatile int vt_steps;
static volatile float slowmo = 1;

uint32_t halo_us() {
  return (uint32_t)m_clock;
}
uint32_t halo_ms() {
  return (uint32_t)(m_clock / 1000);
}

// ---------- Requests from other tasks: the frame task carries them out between frames ----------
static volatile int bright_req = -1, power_req = -1;
static volatile bool disp_on = true, boot_cmd, boot_long_cmd;

void halo_brightness(uint8_t level) {
  bright_req = level;
}

void halo_display_on(bool on) {
  disp_on = on, power_req = on;
}

bool halo_display_is_on() {
  return disp_on;
}

// ---------- The report (halo_loop): the frame task leaves the last period's numbers here ----------
static uint32_t hp_report[2][HP_PARTS];
static volatile bool report_ready;

static void hf_log_idle(uint32_t ticks, uint32_t update_us) {
  const float sec = cfg.report_ms / 1000.0f;
  LOG("%s: idle (%.0f ticks/s), update %.2f ms\n", cfg.name, ticks / sec, update_us / 1000.0f / max((uint32_t)1, ticks));
}

static void hf_log_prof(uint32_t drawn) {  // мс на нарисованный кадр: ядро 1 / ядро 0
  static const char *const N[HP_PARTS] = {"text", "mask", "image", "rrect", "circle", "ring", "arc", "edge", "pixels", "clear", "-"};
  char line[280];
  for (int half = 0; half < 2; half++) {  // (двумя строками: длинную LOG обрезал — после «pixels» было не видно)
    int n = snprintf(line, sizeof(line), "%s prof ms/frame (core1/core0):", cfg.name);
    for (int i = half ? 6 : 0; i < (half ? HP_PARTS - 1 : 6); i++)
      n += snprintf(line + n, sizeof(line) - n, " %s %.2f/%.2f", N[i], hp_report[1][i] / (HALO_CPU_MHZ * 1000.0f) / drawn,
                    hp_report[0][i] / (HALO_CPU_MHZ * 1000.0f) / drawn);
    LOG("%s\n", line);
  }
}

// ---------- The commands every board takes (halo_begin: after its own) ----------
static void hf_commands() {
  halo_command("pause", [](const char *) { vt_steps = 0, vt_paused = true; });
  halo_command("resume", [](const char *) { vt_steps = 0, vt_paused = false; });
  halo_command("step", [](const char *a) { vt_steps += max(1, atoi(a)), vt_paused = true; });
  halo_command("clock", [](const char *a) { vt_fixed = !strcmp(a, "fixed"); });
  halo_command("slow", [](const char *a) { slowmo = 1 / constrain(atof(a), 1.0f, 100.0f); });
  halo_command("bright", [](const char *a) { halo_brightness(atoi(a)); });
  halo_command("boot", [](const char *a) { (strstr(a, "long") ? boot_long_cmd : boot_cmd) = true; });
  halo_command("redraw", [](const char *) { halo_damage_all(); });
  halo_command("prof", [](const char *) { hp_on = !hp_on; });
}
