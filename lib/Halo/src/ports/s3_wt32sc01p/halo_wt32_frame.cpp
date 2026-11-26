// Halo's frame engine on the WT32-SC01 Plus: on the panel's TE signal it draws and sends only the bands of rows that
// changed. It is the round watch's engine (ports/s3_amoled175/halo_frame.cpp — the long story of its timing is there):
// the ST7796 keeps the picture in its own memory just as that panel does, so a frame is bands in small DMA buffers.
// What differs here: the screen is a rectangle (a band is the whole width, no circle to cut by), the bus is 8-bit
// parallel (one byte a tick), there is no BOOT button, and the diagnostics that live in PSRAM fit its 2 MB.
//
//   the frame task (core 1)                          the helper (core 0)
//     waits for TE
//     the last update's changes → bands with changes
//     for each: a buffer (waits on a semaphore) ───→   the right part (a wide band): black + draw
//               the left part: black + draw
//               waits for the helper ←───              done
//               into the bus queue (does not wait)
//     waits until all is out
//     update(dt) — the next frame; nothing changed — the next frame is not drawn at all
//
// No tearing: the panel passes over its memory 60 times a second, the pass starts when TE's pause ends. A frame's rows
// must all land in one pass: ahead of it (a light frame: written right after TE, each band before the pass reaches it)
// or behind it (a heavy frame: a band goes once the pass has gone by; the next pass shows the frame whole).
// On the wide screen (HALO_LANDSCAPE) the panel's pass runs across our rows, not along them: the timing still paces
// the frames, but a moving edge can show a tear.
#include <sdkconfig.h>
#if defined(HALO_BOARD_WT32SC01P)  // this board's port: other boards build their own
#include "halo_wt32.h"
#include "halo_spans.h"
#include <esp_timer.h>

#pragma GCC optimize("O3", "fast-math")

// rec: 2 MB of PSRAM in all — 1 MB when the command does not say, half a megabyte stays free
#define HF_REC_BYTES (1u << 20)
#define HF_REC_SPARE (1u << 19)
#include "../esp32_common/halo_s3_frame.h"  // the frame engine: one for the S3 boards

static float hf_us_per_px() {  // a pixel on the bus: 8 bits a tick, 2 bytes a pixel
  return 2e6f / HALO_WT32_PCLK_HZ;
}

// No BOOT button here: GPIO0 is the panel's command line. The boot command stays — a press from the port, for checks
bool halo_boot_click() {
  if (boot_cmd) return boot_cmd = false, true;
  return false;
}

bool halo_boot_long() {
  if (boot_long_cmd) return boot_long_cmd = false, true;
  return false;
}

bool halo_begin(HaloUpdate update, HaloDraw draw, const HaloConfig &c) {
  if (!hf_begin_display(update, draw, c)) return false;
  return hf_begin_tasks();
}

void halo_loop() {
  hc_loop();
  hf_report();
  delay(5);
}

#endif  // HALO_BOARD_WT32SC01P
