// Halo's frame engine on the ESP32-S3-Touch-LCD-4B: on the start of each pass over the frame buffer it draws the bands
// of rows that changed and writes them into it. It is the round watch's engine (ports/s3_amoled175/halo_frame.cpp — the
// long story of its timing is there) by way of the WT32-SC01 Plus's, with the output this board needs
// (halo_lcd4b_display.cpp): a band goes into a frame buffer in PSRAM that the LCD's bounce fills read 60 times a second.
//
//   the frame task (core 1)                          the helper (core 0)
//     waits for a pass to start ("TE")
//     the last update's changes → bands with changes
//     for each: a buffer (waits on a semaphore) ───→   the right part (a wide band): black + draw
//               the left part: black + draw
//               waits for the helper ←───              done
//               waits until the pass has read these rows, then into the copy queue (does not wait)
//     waits until all is in
//     update(dt) — the next frame; nothing changed — the next frame is not drawn at all
//
// No tearing with one buffer: a frame's bands all go in behind the same pass — each once that pass has read it — and the
// next pass shows them together. Always behind, never ahead: the pass reads its first 32 rows before the LCD's blank, so
// no frame could get ahead of it there. A frame is on screen a pass after it is drawn, as with the P4's two buffers.
// Spans are whole HD_SPAN-pixel blocks (the DMA writes PSRAM in HD_BURST-byte blocks).
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"
#include "halo_spans.h"
#include <driver/gpio.h>
#include <esp_cache.h>
#include <esp_timer.h>

#pragma GCC optimize("O3", "fast-math")

#define HF_FRAME_BUFFER  // the screen: a frame buffer in PSRAM, read by the LCD's passes (the top of this file)
#include "../esp32_common/halo_s3_frame.h"  // the frame engine: one for the S3 boards

// The BOOT button — GPIO0 (down — 0): a press counts when it is down two frames in a row after two up (contact bounce is
// shorter than a frame). The boot command is the same press from the port. Held longer than HALO_BOOT_LONG_MS — a long
// press too (halo_boot_long), the press that woke the board included
static bool boot_swallow;  // BOOT woke the board (halo_lcd4b_system.cpp): this press is already "counted"
static bool boot_down, boot_long_done = true;
static uint32_t boot_down_ms;

void hf_swallow_boot() {
  boot_swallow = true;
}

bool halo_boot_click() {
  static uint8_t hist = 0xFF;
  hist = hist << 1 | (gpio_get_level((gpio_num_t)HALO_PIN_BOOT) ? 1 : 0);
  if (boot_cmd) return boot_cmd = false, true;
  if (boot_swallow) boot_swallow = false, boot_down = true, boot_down_ms = millis(), boot_long_done = false;
  if (!boot_down && (hist & 3) == 0) return boot_down_ms = millis(), boot_long_done = false, boot_down = true;
  if (boot_down && (hist & 3) == 3) boot_down = false, boot_long_done = true;
  return false;
}

bool halo_boot_long() {
  if (boot_long_cmd) return boot_long_cmd = false, true;
  if (boot_long_done || !boot_down || millis() - boot_down_ms < HALO_BOOT_LONG_MS) return false;
  return boot_long_done = true;
}

bool halo_begin(HaloUpdate update, HaloDraw draw, const HaloConfig &c) {
  if (!hf_begin_display(update, draw, c)) return false;
  pinMode(HALO_PIN_BOOT, INPUT_PULLUP);
  return hf_begin_tasks();
}

void halo_loop() {
  hc_loop();
  hf_report();
  delay(5);
}

#endif  // HALO_BOARD_LCD4B
