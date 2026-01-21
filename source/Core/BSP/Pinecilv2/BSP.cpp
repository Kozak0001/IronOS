// BSP mapping functions

#include "BSP.h"
#include "BootLogo.h"
#include "I2C_Wrapper.hpp"
#include "IRQ.h"
#include "Pins.h"
#include "Settings.h"
#include "Setup.h"
#if defined(WS2812B_ENABLE)
#include "WS2812B.h"
#endif
#include "TipThermoModel.h"
#include "USBPD.h"
#include "Utils.hpp"
#include "bflb_platform.h"
#include "bl702_adc.h"
#include "configuration.h"
#include "crc32.h"
#include "hal_flash.h"
#include "history.hpp"
#include "main.hpp"

#include <cstdint>

extern ADC_Gain_Coeff_Type adcGainCoeffCal;

// PWM control
const uint16_t powerPWM         = 255;
uint8_t        holdoffTicks     = 25;
uint8_t        tempMeasureTicks = 25;
uint16_t       totalPWM         = 255;

#if defined(WS2812B_ENABLE)
WS2812B<WS2812B_Pin, 1> ws2812b;
#endif

void resetWatchdog() {
  // Not implemented
}

#ifdef TEMP_NTC
static const int32_t NTCHandleLookup[] = {
    3405, -400, 4380, -350, 5572, -300, 6999, -250, 8688, -200, 10650, -150,
    12885, -100, 15384, -50, 18129, 0, 21074, 50, 24172, 100, 27362, 150,
    30595, 200, 33792, 250, 36907, 300, 39891, 350, 42704, 400, 45325, 450,
    47736, 500, 49929, 550, 51912, 600, 53689, 650, 55274, 700, 56679, 750,
    57923, 800, 59020, 850, 59984, 900, 60832, 950, 61580, 1000, 62232, 1050,
    62810, 1100, 63316, 1150, 63765, 1200, 64158, 1250,
};
#endif

uint16_t getHandleTemperature(uint8_t sample) {
  int32_t result = getADCHandleTemp(sample);
  return Utils::InterpolateLookupTable(
      NTCHandleLookup,
      sizeof(NTCHandleLookup) / (2 * sizeof(int32_t)),
      result);
}

uint16_t getInputVoltageX10(uint16_t divisor, uint8_t sample) {
  uint32_t res = getADCVin(sample);
  res *= 4;
  res /= divisor;
  return res;
}

uint8_t getButtonA() { return gpio_read(KEY_A_Pin); }
uint8_t getButtonB() { return gpio_read(KEY_B_Pin); }

void BSPInit(void) {
#if defined(WS2812B_ENABLE)
  ws2812b.init();
#endif
}

void reboot() { hal_system_reset(); }

void delay_ms(uint16_t count) { BL702_Delay_MS(count); }

uint32_t __get_IPSR(void) { return 0; }

bool isTipDisconnected() {
  uint16_t th = TipThermoModel::getTipMaxInC() - 5;
  return TipThermoModel::getTipInC() > th;
}

//
// ================= LED LOGIC =================
//

void setStatusLED(const enum StatusLED state) {
#if defined(WS2812B_ENABLE)
  static enum StatusLED lastState = LED_UNKNOWN;

  constexpr uint8_t MAX_BRIGHT = 85; // ~1/3 від 255

  // Для HEATING і COOLING оновлюємо постійно, бо там анімація
  bool forceUpdate = (state == LED_HEATING || state == LED_COOLING_STILL_HOT);

  if (lastState != state || forceUpdate) {
    switch (state) {
    default:
    case LED_UNKNOWN:
    case LED_OFF:
      ws2812b.led_set_color(0, 0, 0, 0);
      break;

    case LED_STANDBY:
      ws2812b.led_set_color(0, 0, MAX_BRIGHT, 0); // зелений
      break;

    case LED_HEATING: {
      uint8_t r = (uint8_t)((xTaskGetTickCount() / 4) % MAX_BRIGHT);
      ws2812b.led_set_color(0, r, 0, 0); // червоний fade
    } break;

    case LED_HOT:
      ws2812b.led_set_color(0, MAX_BRIGHT, 0, 0); // червоний
      break;

    case LED_COOLING_STILL_HOT: {
      // Жовтий повільний "пульс" 2 секунди
      uint32_t ms  = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
      uint32_t p   = ms % 2000;
      uint32_t tri = (p < 1000) ? p : (2000 - p);
      uint8_t  k   = (uint8_t)((tri * MAX_BRIGHT) / 1000);

      ws2812b.led_set_color(0, k, k, 0); // жовтий
    } break;
    }

    ws2812b.led_update();
    lastState = state;
  }
#endif
}
