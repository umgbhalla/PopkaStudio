// The GT911 touch chip, polled: the boards whose chip's INT line does not reach the processor (P4 7″, S3 LCD-4B).
//
// Reports come every 10 ms while a finger is down: after a report there is nothing to ask for 8 ms, then the task asks
// without pausing until the next one appears. Status at 0x814E: bit 7 — a new report, bits 0-3 — how many fingers;
// 8 bytes a finger from 0x814F (track id, x, y, size); writing 0 to 0x814E takes the report.
//
// Included by the board's touch file only, after halo_touch_ring.h. The board gives its task gt911_task(): the chip, what
// to do between questions (idle) and how the chip's point lies on the screen (point).
#pragma once

#include <driver/i2c_master.h>

#define REPORT_AFTER_US 8000  // nothing to ask for this long after a report
#define RELEASE_US 100000     // no reports for this long: the finger is gone even without a "no fingers" report

static bool gt_read(i2c_master_dev_handle_t tp, uint16_t reg, uint8_t *out, size_t n) {
  const uint8_t r[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
  return i2c_master_transmit_receive(tp, r, 2, out, n, 20) == ESP_OK;
}

static bool gt_take(i2c_master_dev_handle_t tp) {  // the report is read: the chip may write the next one
  const uint8_t w[3] = {0x81, 0x4E, 0};
  return i2c_master_transmit(tp, w, 3, 20) == ESP_OK;
}

// idle() — before each question; point(&x, &y) — the chip's point to the screen's (clamped to it afterwards).
// No chip (tp == nullptr): only idle, every 50 ms
template <class Idle, class Point>
static void gt911_task(i2c_master_dev_handle_t tp, Idle idle, Point point) {
  int track = -1;      // the finger we follow: with two on the glass their order in the report changes
  uint32_t last = 0;   // when the latest report came
  uint32_t asked = 0;  // questions in a row without an answer
  for (;;) {
    if (hist_n) {
      const int32_t left = (int32_t)(REPORT_AFTER_US - (trace_now() - last));
      if (left >= 1000) vTaskDelay(left / 1000);
      if (asked >= 200) vTaskDelay(1);  // (40 ms of asking: a finger that stopped reporting, stay polite on the bus)
    } else {
      vTaskDelay(3);
    }
    idle();
    if (!tp) {
      vTaskDelay(50);
      continue;
    }
    uint8_t st = 0;
    const uint32_t t = trace_now();
    if (!gt_read(tp, 0x814E, &st, 1)) {
      vTaskDelay(5);
      continue;
    }
    if (!(st & 0x80)) {
      asked++;
      if (hist_n && t - last > RELEASE_US) track = -1, release("timeout");
      continue;
    }
    asked = 0;
    const int n = st & 0x0F;
    if (n < 1 || n > 5) {
      gt_take(tp);
      track = -1;
      release("lift");
      continue;
    }
    uint8_t p[40];
    const bool ok = gt_read(tp, 0x814F, p, n * 8);
    gt_take(tp);
    if (!ok) continue;
    int i = -1;
    if (track < 0) i = 0, track = p[0];
    else
      for (int k = 0; k < n; k++)
        if (p[k * 8] == track) i = k;
    if (i < 0) {  // the finger we followed is gone and another one stays: a new touch, not a jump to where that one is
      track = -1;
      release("finger changed");
      continue;
    }
    int x = p[i * 8 + 1] | (p[i * 8 + 2] << 8), y = p[i * 8 + 3] | (p[i * 8 + 4] << 8);
    point(&x, &y);
    last = t;
    touch_put({(int16_t)constrain(x, 0, HALO_W - 1), (int16_t)constrain(y, 0, HALO_H - 1), t});
  }
}
