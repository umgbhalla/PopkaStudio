// The TE signal's count and its waiters, the same on the boards that wait for a TE edge (the round watch, WT32-SC01 Plus;
// on the S3 LCD-4B — the start of a pass over the frame buffer). The board's interrupt keeps the state: under te_mux it
// counts te_count, sets te_rise (and te_period between neighbouring edges), then wakes te_waiter; te_blank — the pause
// before the rows. The P4 wakes its waiter only at the count it asked for: its own.
//
// Included by the board's display file only (it defines hd_wait_te and the hd_te_* readers); HALO_TE_BLANK_US first —
// te_blank before the first measure.
#pragma once

static DRAM_ATTR volatile uint32_t te_rise, te_period, te_count, te_blank = HALO_TE_BLANK_US;
static DRAM_ATTR volatile TaskHandle_t te_waiter;
static portMUX_TYPE te_mux = portMUX_INITIALIZER_UNLOCKED;  // the signal's number and its time — as a pair (hd_te_snapshot)

// The waiter is written before the counter is checked: a signal that comes between the check and the sleep is not lost
bool hd_wait_te(uint32_t count, uint32_t timeout_ms) {
  te_waiter = xTaskGetCurrentTaskHandle();
  ulTaskNotifyTake(pdTRUE, 0);  // old notifications do not count — the counter does
  bool ok = true;
  while (ok && (int32_t)(te_count - count) < 0) ok = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms)) > 0;
  te_waiter = nullptr;
  return ok;
}

void hd_te_snapshot(uint32_t *count, uint32_t *rise) {
  portENTER_CRITICAL(&te_mux);
  *count = te_count, *rise = te_rise;
  portEXIT_CRITICAL(&te_mux);
}

uint32_t hd_te_count() {
  return te_count;
}
uint32_t hd_te_period() {
  return (uint32_t)esp_timer_get_time() - te_rise < 100000 ? te_period : 0;
}
uint32_t hd_te_rise() {
  return te_rise;
}
uint32_t hd_te_blank() {
  return te_blank;
}
