// Waveshare ESP32-S3-Touch-AMOLED-1.75C: the numbers every part of Halo needs to know about this board.
#pragma once

#define HALO_W 466
#define HALO_H 466
#define HALO_PX(c) __builtin_bswap16(c)  // a pixel in memory: the panel takes the high byte first
#define HALO_ROUND 1  // the screen is a circle: nothing outside it is drawn

#if defined(HALO_BOARD_AMOLED175G)
// The 1.75 (-B, -G) has a microSD slot, wired for SPI (its own bus, SPI3: SPI2 is the display's). The firmware's
// Music player reads its tracks from it (firmware/PopkaWatch/music_sd.h); a board without these has no card
#define HALO_SD_MOSI 1
#define HALO_SD_SCK 2
#define HALO_SD_MISO 3
#define HALO_SD_CS 41
#endif
