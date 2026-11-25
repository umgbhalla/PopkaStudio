// The P4 board's touch: GT911 on the shared I2C bus (GPIO7/8, also the sound chips).
//
// The chip's INT line does not reach the processor (boards/p4lcd7/hardware.md), so reports are polled. They come exactly every
// 10 ms while a finger is down (measured: 98.7 a second, 10-11 ms apart, none lost), which is what the polling is built
// around: after a report there is nothing to ask for 8 ms, then the task asks without pausing until the next one appears.
// Each question is 175 us on the bus and the task sleeps through it, so a report's time is known to about 0.2 ms; the time
// matters more than the position here, since the finger code interpolates between reports by these times.
//
// Status at 0x814E: bit 7 — a new report, bits 0-3 — how many fingers; 8 bytes a finger from 0x814F (track id, x, y, size);
// writing 0 to 0x814E takes the report. Both axes of the touch layer are mirrored against the panel.
// The chip's address is 0x5D or 0x14: which one depends on a line we do not drive, and it can change after a reset.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include "../esp32_common/halo_i2c.h"
#include "../esp32_common/halo_touch_ring.h"
#include "../esp32_common/halo_gt911.h"

#define PIN_SDA GPIO_NUM_7
#define PIN_SCL GPIO_NUM_8

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t tp;
static uint8_t tp_addr;

static void touch_task(void *) {
  gt911_task(tp, [] {}, [](int *x, int *y) {
    *x = HALO_FB_W - 1 - *x, *y = HALO_FB_H - 1 - *y;  // (both axes mirrored against the panel)
    halo_p4_to_screen(x, y);  // (the panel's point -> the interface's: turned when the board is held tall)
  });
}

HaloI2c halo_i2c(uint8_t addr) {  // (halo_i2c.cpp: its reads and writes)
  return halo_i2c_device(bus, addr);
}

bool ht_begin() {
  i2c_master_bus_config_t bc = {};
  bc.i2c_port = I2C_NUM_0;
  bc.sda_io_num = PIN_SDA;
  bc.scl_io_num = PIN_SCL;
  bc.clk_source = I2C_CLK_SRC_DEFAULT;
  bc.glitch_ignore_cnt = 7;
  bc.flags.enable_internal_pullup = 1;
  if (i2c_new_master_bus(&bc, &bus) != ESP_OK) return false;
  for (uint8_t a : {(uint8_t)0x5D, (uint8_t)0x14}) {
    if (i2c_master_probe(bus, a, 50) != ESP_OK) continue;
    i2c_device_config_t dc = {};
    dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = a, dc.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &dc, &tp) == ESP_OK) tp_addr = a;
    break;
  }
  if (!tp) {
    LOG("touch: GT911 NOT FOUND\n");
    return false;
  }
  xTaskCreatePinnedToCore(touch_task, "halo_touch", 4096, nullptr, 12, nullptr, 0);  // (LOG takes most of it)
  LOG("touch: GT911 at 0x%02X\n", tp_addr);
  return true;
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
