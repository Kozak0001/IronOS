/*
 * WS2812B.h
 *
 *  Created on: 9 July 2023
 *      Author: Doegox
 *      Currently for RISC-V architecture only
 *      Based on WS2812.h by Ralim for STM32
 */
#include "Pins.h"
#include "Setup.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef CORE_DRIVERS_WS2812B_H_
#define CORE_DRIVERS_WS2812B_H_

#ifndef WS2812B_LED_CHANNEL_COUNT
#define WS2812B_LED_CHANNEL_COUNT 3
#endif

#define WS2812B_RAW_BYTES_PER_LED (WS2812B_LED_CHANNEL_COUNT * 8)

template <uint16_t LED_PIN, int LED_COUNT> class WS2812B {
private:
  uint8_t leds_colors[WS2812B_LED_CHANNEL_COUNT * LED_COUNT];

  // Global brightness limiter (0..255)
  // 40 ~= 15% brightness: much less glare + much less internal heating.
  static constexpr uint8_t BRIGHTNESS = 85; // ~33% від максимуму

  static inline uint8_t scale(uint8_t v) {
    return (uint16_t(v) * BRIGHTNESS) / 255;
  }

public:
  void led_update() {
    __disable_irq();
    // Bitbang it out as our cpu irq latency is too high
    for (unsigned int i = 0; i < sizeof(leds_colors); i++) {
      // Shove out MSB first
      for (int x = 0; x < 8; x++) {
        if ((leds_colors[i] & (1 << (7 - x))) == (1 << (7 - x))) {
          gpio_write(LED_PIN, 1);
          for (int k = 0; k < 27; k++) {
            __ASM volatile("nop");
          }
          gpio_write(LED_PIN, 0);
          for (int k = 0; k < 10; k++) {
            __ASM volatile("nop");
          }
        } else {
          gpio_write(LED_PIN, 1);
          for (int k = 0; k < 10; k++) {
            __ASM volatile("nop");
          }
          gpio_write(LED_PIN, 0);
          for (int k = 0; k < 27; k++) {
            __ASM volatile("nop");
          }
        }
      }
    }
    __enable_irq();
  }

  void init(void) {
    memset(leds_colors, 0, sizeof(leds_colors));
    gpio_set_mode(LED_PIN, GPIO_OUTPUT_MODE);

    // WS2812 idle should be low, not high
    gpio_write(LED_PIN, 0);

    // Default indicator (green), scaled by BRIGHTNESS
    led_set_color(0, 0, 0xFF, 0);
    led_update();
  }

  // Optional helper: force refresh without changing colors
  void refresh() { led_update(); }

  void led_set_color(size_t index, uint8_t r, uint8_t g, uint8_t b) {
    r = scale(r);
    g = scale(g);
    b = scale(b);

    leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 0] = g;
    leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 1] = r;
    leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 2] = b;
  }

  void led_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
    r = scale(r);
    g = scale(g);
    b = scale(b);

    for (int index = 0; index < LED_COUNT; index++) {
      leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 0] = g;
      leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 1] = r;
      leds_colors[index * WS2812B_LED_CHANNEL_COUNT + 2] = b;
    }
  }
};

#endif /* CORE_DRIVERS_WS2812B_H_ */

