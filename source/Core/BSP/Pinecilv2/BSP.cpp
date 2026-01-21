void setStatusLED(const enum StatusLED state) {
#if defined(WS2812B_ENABLE)
  static enum StatusLED lastState = LED_UNKNOWN;

  // We must refresh while heating (animation) and while cooling (pulse animation)
  if (lastState != state || state == LED_HEATING || state == LED_COOLING_STILL_HOT) {
    switch (state) {
    default:
    case LED_UNKNOWN:
    case LED_OFF:
      ws2812b.led_set_color(0, 0, 0, 0);
      break;

    case LED_STANDBY:
      ws2812b.led_set_color(0, 0, 0xFF, 0); // green
      break;

    case LED_HEATING: {
      // Red fade
      ws2812b.led_set_color(0, ((xTaskGetTickCount() / 4) % 192) + 64, 0, 0);
    } break;

    case LED_HOT:
      ws2812b.led_set_color(0, 0xFF, 0, 0); // red
      break;

    case LED_COOLING_STILL_HOT: {
      // Yellow + slow "breathing" pulse (2 seconds triangle wave)
      // Make a 0..255..0 ramp with 2s period
      uint32_t ms = (uint32_t)xTaskGetTickCount() * (uint32_t)portTICK_PERIOD_MS;
      uint32_t p  = ms % 2000;                    // 0..1999
      uint32_t tri = (p < 1000) ? p : (2000 - p); // 0..1000..0
      uint8_t  k   = (uint8_t)((tri * 255) / 1000); // 0..255..0

      uint8_t r = (uint8_t)((uint16_t)0xFF * k / 255);
      uint8_t g = (uint8_t)((uint16_t)0xFF * k / 255);
      uint8_t b = 0;

      ws2812b.led_set_color(0, r, g, b);
    } break;
    }

    ws2812b.led_update();
    lastState = state;
  }
#endif
}
