// Звук для Halo: динамик через кодек ES8311 (I2C 0x18 — на шине тача) и усилитель (GPIO46), два микрофона через
// АЦП ES7210 (I2C 0x40). Драйвер — общий (esp32_common/halo_es_audio.cpp, там же вся история); здесь — только своё:
// пины I2S (MCLK = 256·fs на GPIO16, на 1.75 -B/-G — 42; BCLK 9, LRCK 45, данные 8 (→ ES8311) и 10 (← ES7210)), чипы —
// на шине тача, усилитель — обычный GPIO.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32S3 && !defined(HALO_BOARD_WT32SC01P) && !defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_s3.h"
#include "../esp32_common/halo_es_audio.h"

#include <driver/gpio.h>

const HaloEsAudio halo_es_audio = {
    HALO_PIN_MCLK, HALO_PIN_BCLK, HALO_PIN_LRCK, HALO_PIN_DOUT, HALO_PIN_DIN,
    ht_i2c_device,
    [] {
      gpio_set_direction((gpio_num_t)HALO_PIN_PA, GPIO_MODE_OUTPUT);
      gpio_set_level((gpio_num_t)HALO_PIN_PA, 0);
    },
    [](bool on) { gpio_set_level((gpio_num_t)HALO_PIN_PA, on); },
    3072,
};

#endif  // CONFIG_IDF_TARGET_ESP32S3
