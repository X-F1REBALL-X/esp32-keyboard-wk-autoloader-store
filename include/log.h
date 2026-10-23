#pragma once
#include <Arduino.h>
// Mirror app logs to USB CDC (Serial) and UART0 (COM port, 115200).
// ESP32-S2 without USB CDC: Serial is already UART0.
#if !ARDUINO_USB_CDC_ON_BOOT
#define Serial0 Serial
#endif
class DualLog : public Print {
public:
  void begin(unsigned long baud) {
    Serial.begin(baud);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial0.begin(baud);
#endif
  }
  size_t write(uint8_t c) override {
    Serial0.write(c);
#if ARDUINO_USB_CDC_ON_BOOT
    if (Serial) Serial.write(c);
#endif
    return 1;
  }
  size_t write(const uint8_t *b, size_t n) override {
    Serial0.write(b, n);
#if ARDUINO_USB_CDC_ON_BOOT
    if (Serial) Serial.write(b, n);
#endif
    return n;
  }
};
extern DualLog LOG;
