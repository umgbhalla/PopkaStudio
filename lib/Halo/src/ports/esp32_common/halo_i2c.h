// I2C for the boards whose bus driver takes turns between tasks by itself (P4 7″, S3 LCD-4B, WT32-SC01 Plus): one more
// device on a bus, and the sketch's halo_i2c_write / halo_i2c_read (Halo.h, halo_i2c.cpp). The round watch locks its bus
// around each transfer and keeps its own (ports/s3_amoled175/halo_touch.cpp).
#pragma once

#include <driver/i2c_master.h>

i2c_master_dev_handle_t halo_i2c_device(i2c_master_bus_handle_t bus, uint8_t addr);  // 400 kHz; nullptr — no answer
