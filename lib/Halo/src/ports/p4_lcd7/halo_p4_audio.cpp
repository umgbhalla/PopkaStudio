// The P4 board's sound: a speaker through the ES8311 codec and the NS4150B amplifier (GPIO53, high is on), two microphones
// through the ES7210 ADC. Both codecs sit on the I2C bus shared with the touch chip (0x18, 0x40; the bus driver takes turns).
// The chips are the round board's, so the driver is the shared one (esp32_common/halo_es_audio.cpp); only the pins, the bus,
// the amplifier and the task's stack are this board's own.
//
// I2S: the ESP32 drives the bus — MCLK = 256 fs on GPIO13, BCLK 12, LRCK 10, data out 9, data in 11.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"
#include "../esp32_common/halo_es_audio.h"

#include <driver/gpio.h>

#define PIN_PA GPIO_NUM_53

const HaloEsAudio halo_es_audio = {
    GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_10, GPIO_NUM_9, GPIO_NUM_11,
    halo_i2c,
    [] {  // (the whole setup, not just the direction: after reset this pad's mux is not on the GPIO function)
      const gpio_config_t pa_cfg = {.pin_bit_mask = 1ULL << PIN_PA, .mode = GPIO_MODE_OUTPUT};
      gpio_config(&pa_cfg);
      gpio_set_level(PIN_PA, 0);
    },
    [](bool on) { gpio_set_level(PIN_PA, on); },
    4096,
};

#endif  // CONFIG_IDF_TARGET_ESP32P4
