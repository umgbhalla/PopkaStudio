// Движок кадров Halo: по сигналу TE рисует и отправляет только пачки строк с изменениями.
//
//   задача кадров (ядро 1)                         помощник (ядро 0)
//     ждёт TE
//     изменения прошлого update → пачки с изменениями
//     для каждой: буфер (ждёт на семафоре) ───→     правая часть (если пачка широкая): чёрный фон + draw
//                 левая часть: чёрный фон + draw
//                 ждёт помощника ←───              готово
//                 в очередь DMA (не ждёт)
//     ждёт, пока всё уйдёт
//     update(dt) — следующий кадр; изменений нет — следующий кадр не рисуется вовсе
//
// В пачке перерисовывается и уходит не весь круг, а только столбцы с изменениями (объединение по пачке).
//
// Без разрыва (дисплей сам 60 раз в секунду проходит свою память сверху вниз, ~34 мкс на строку, проход начинается
// с концом паузы TE). Строки одного кадра должны попасть в один и тот же проход, есть два способа:
//   * впереди прохода — лёгкий кадр: пишем сразу после TE, каждая пачка успевает раньше, чем до неё дойдёт проход
//     (у верхней строки — 0,6 мс, у нижней — 16 мс). Задержка от расчёта до экрана — меньше;
//   * вслед за проходом — тяжёлый кадр: пачку отправляем, только когда текущий проход её миновал, а следующий
//     покажет кадр целиком. У каждой пачки — почти весь период 16,6 мс, но кадр на экране на проход позже.
// Какой выбрать — по оценке: рисование пачек (по прошлым кадрам) и очередь на шине против срока каждой «впереди».
//
// Ровный шаг движения (как Choreographer в Android: время кадра — от сигнала дисплея, а не от того, когда считали):
//   * время анимации — момент, когда кадр покажет экран: перед update решаем, с какого TE кадр начнётся и пойдёт ли
//     вслед за проходом, и берём начало этого прохода. Раньше брали время самого update — оно гуляет с ценой кадра
//     (лёгкий кадр кончился через 2 мс после TE, тяжёлый — через 10), и шаг движения на экране гулял так же;
//   * способ не меняется посреди движения: переход «впереди → вслед» показывает прошлый кадр дважды, обратный
//     (два кадра в один проход) — один не показывает вовсе. Раз пошли вслед — так и идём, пока движение не встанет
//     (0,1 с без изменений) или кадры долго лёгкие с запасом; тогда обратно «впереди», а чтобы два кадра не попали в
//     один проход, новый ждёт следующего TE (проход тот же, что был бы вслед, — ничего не теряется).
// Замер HaloWatch (лента, бросок): было 10 смен способа на 80 кадров — 6 кадров дважды, 6 ни разу, шаг движения
// гулял до 14 мс (tools/trace.py: «кадры на экране»).
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32S3 && !defined(HALO_BOARD_WT32SC01P) && !defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_s3.h"
#include "halo_spans.h"
#include <esp_timer.h>

#pragma GCC optimize("O3", "fast-math")

#include "../esp32_common/halo_s3_frame.h"  // движок кадров — один на платы S3

static float hf_us_per_px() {  // передача пикселя по шине: QSPI — 4 бита за такт, 2 байта на пиксель
  return 4e6f / cfg.bus_hz;
}

// Кнопка BOOT — GPIO0 (нажата — 0): нажатие засчитывается, когда она нажата два кадра подряд после двух отпущенных
// (дребезг контактов — короче кадра). Команда boot — то же нажатие из порта. Держат дольше HALO_BOOT_LONG_MS — ещё и
// долгое (halo_boot_long), и у нажатия, которым её разбудили, тоже
static bool boot_swallow;  // BOOT разбудила плату (halo_power.cpp): это нажатие уже «засчитано»
static bool boot_down, boot_long_done = true, boot_report_release, boot_release_long;
static uint32_t boot_down_ms;
static uint8_t boot_edges;

void hf_swallow_boot() {
  boot_swallow = true;
}

static bool boot_poll(bool on_release, bool count_wake) {
  static uint8_t hist = 0xFF;
  boot_edges = 0;
  hist = hist << 1 | (gpio_get_level(GPIO_NUM_0) ? 1 : 0);
  bool click = boot_cmd;
  boot_cmd = false;
  if (boot_swallow || (!boot_down && (hist & 3) == 0)) {
    boot_report_release = on_release && (!boot_swallow || count_wake);
    click |= !on_release && !boot_swallow;
    boot_swallow = false;
    if (!boot_down) boot_edges |= HALO_KEY_DOWN;
    boot_down = true;
    boot_down_ms = millis();
    boot_long_done = false;
  }
  if (boot_down && (hist & 3) == 3) {
    boot_edges |= HALO_KEY_UP;
    if (!boot_long_done && on_release) {
      boot_release_long = millis() - boot_down_ms >= HALO_BOOT_LONG_MS;
      click |= boot_report_release && !boot_release_long;
    }
    boot_down = false;
    boot_long_done = true;
  }
  return click;
}

bool halo_boot_click() {
  return boot_poll(false, false);
}

bool halo_boot_short(bool count_wake) {
  return boot_poll(true, count_wake);
}

uint8_t halo_boot_edges() { return boot_edges; }

bool halo_boot_long(bool physical) {
  if (boot_long_cmd) return boot_long_cmd = false, true;
  if (boot_release_long) return boot_release_long = false, physical;
  if (boot_long_done || !boot_down || millis() - boot_down_ms < HALO_BOOT_LONG_MS) return false;
  boot_long_done = true;
  return physical;
}

bool halo_begin(HaloUpdate update, HaloDraw draw, const HaloConfig &c) {
  if (!hf_begin_display(update, draw, c)) return false;
#ifdef HALO_BOARD_AMOLED175G
  hr_begin();  // (the 1.75's clock chip: on the touch's I2C bus — after ht_begin)
#endif
  pinMode(0, INPUT_PULLUP);  // кнопка BOOT
  return hf_begin_tasks();
}

void halo_loop() {
  hc_loop();
#ifdef HALO_BOARD_AMOLED175G
  hr_tick();
#endif
  hf_report();
  delay(5);
}

#endif  // CONFIG_IDF_TARGET_ESP32S3
