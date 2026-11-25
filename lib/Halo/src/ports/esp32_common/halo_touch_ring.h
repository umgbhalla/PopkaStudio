// The touch's last samples and its log, the same on every board: the reading task puts each report here with
// touch_put(), a lifted finger empties it with release(), halo_touch_samples() hands the sketch the newest ones first.
// Included by the board's touch file only (it defines halo_touch_samples), after the board's own header.
#pragma once

#define HIST 8
static HaloTouchSample hist[HIST];
static int hist_n, hist_head;
static portMUX_TYPE hist_mux = portMUX_INITIALIZER_UNLOCKED;

int halo_touch_samples(HaloTouchSample *out, int max) {
  portENTER_CRITICAL(&hist_mux);
  const int n = min(max, hist_n);
  for (int i = 0; i < n; i++) out[i] = hist[(hist_head - i + HIST) % HIST];
  portEXIT_CRITICAL(&hist_mux);
  return n;
}

// For the log: where the finger came down and went up, how many reports
static int16_t down_x, down_y, last_x, last_y;
static uint16_t reports;

static void release(const char *why) {
  portENTER_CRITICAL(&hist_mux);
  hist_n = 0;
  portEXIT_CRITICAL(&hist_mux);
  if (reports) LOG("touch up %d,%d: moved %+d,%+d, %u reports (%s)\n", last_x, last_y, last_x - down_x, last_y - down_y,
                   reports, why);
  reports = 0;
}

static void touch_put(const HaloTouchSample &s) {
  portENTER_CRITICAL(&hist_mux);
  hist_head = (hist_head + 1) % HIST;
  hist[hist_head] = s;
  if (hist_n < HIST) hist_n++;
  portEXIT_CRITICAL(&hist_mux);
  trace_mark(TR_TOUCH_READ, s.x, s.y);
  if (!reports++) down_x = s.x, down_y = s.y, LOG("touch down %d,%d\n", s.x, s.y);
  last_x = s.x, last_y = s.y;
}
