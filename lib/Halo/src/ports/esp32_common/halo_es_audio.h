// Звук на ES8311 (динамик) + ES7210 (микрофоны) по одной шине I2S — общий драйвер (halo_es_audio.cpp) для плат с этой
// парой чипов: s3_amoled175, s3_lcd4b, p4_lcd7. Плата даёт только своё: пины I2S, как найти чип на своей шине I2C, как
// включить усилитель и стек задачи звука. Публичное (halo_audio_*, halo_mic_*) — из драйвера.
#pragma once

#include <stdint.h>

struct i2c_master_dev_t;

struct HaloEsAudio {
  int mclk, bclk, lrck, dout, din;          // пины I2S: MCLK, BCLK, LRCK, данные в ES8311, данные из ES7210
  i2c_master_dev_t *(*device)(uint8_t addr);  // чип на шине платы (0x18 — ES8311, 0x40 — ES7210), nullptr — не ответил
  void (*amp_init)();                       // до запуска I2S: усилитель — выход, выключен
  void (*amp)(bool on);                     // усилитель динамика (задача звука)
  int stack;                                // стек задачи звука, байт
};

extern const HaloEsAudio halo_es_audio;  // у каждой платы свой (её *_audio.cpp)
