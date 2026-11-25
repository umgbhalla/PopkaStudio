// Sound for Halo on the ESP32-S3-Touch-LCD-4B: the shared driver (esp32_common/halo_es_audio.cpp — the long story is
// there), since the chips are the round watch's: the speaker through the ES8311 codec (I2C 0x18), two microphones through
// the ES7210 (0x40), one I2S bus with the ESP32 as master. What differs here: the pins (MCLK 5, BCLK 16, LRCK 7, DOUT 6,
// DIN 15), the speaker's amplifier is pin 3 of the TCA9554 expander (an I2C write, not a GPIO), and the chips sit on the
// board's one I2C bus (halo_lcd4b_display.cpp starts it).
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"
#include "../esp32_common/halo_es_audio.h"

const HaloEsAudio halo_es_audio = {
    HALO_PIN_MCLK, HALO_PIN_BCLK, HALO_PIN_LRCK, HALO_PIN_DOUT, HALO_PIN_DIN,
    hx_device,
    [] { hx_set(HX_AMP, false); },
    [](bool on) { hx_set(HX_AMP, on); },
    3072,
};

#endif  // HALO_BOARD_LCD4B
