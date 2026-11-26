// I2C devices and the sketch's register reads and writes (halo_i2c.h).
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B) || defined(HALO_BOARD_WT32SC01P) || CONFIG_IDF_TARGET_ESP32P4  // the round watch: its own
#include "../../Halo.h"
#include "halo_i2c.h"

i2c_master_dev_handle_t halo_i2c_device(i2c_master_bus_handle_t bus, uint8_t addr) {
  if (!bus || i2c_master_probe(bus, addr, 50) != ESP_OK) return nullptr;
  i2c_device_config_t dc = {};
  dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = addr, dc.scl_speed_hz = 400000;
  i2c_master_dev_handle_t h = nullptr;
  return i2c_master_bus_add_device(bus, &dc, &h) == ESP_OK ? h : nullptr;
}

bool halo_i2c_write(HaloI2c d, uint8_t reg, uint8_t val) {  // (the bus driver takes turns between tasks by itself)
  const uint8_t w[2] = {reg, val};
  return d && i2c_master_transmit(d, w, 2, 20) == ESP_OK;
}

bool halo_i2c_read(HaloI2c d, uint8_t reg, uint8_t *buf, int n) {
  return d && i2c_master_transmit_receive(d, &reg, 1, buf, n, 20) == ESP_OK;
}

#endif  // HALO_BOARD_LCD4B || HALO_BOARD_WT32SC01P || CONFIG_IDF_TARGET_ESP32P4
