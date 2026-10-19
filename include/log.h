#pragma once
#include <Arduino.h>
// Mirror app logs to USB CDC (Serial) and UART0 (COM port, 115200).
class DualLog : public Print {
public:
  void begin(unsigned long baud) {
    Serial.begin(baud);
    Serial0.begin(baud);
  }
  size_t write(uint8_t c) override {
    Serial0.write(c);
    if (Serial) Serial.write(c);
    return 1;
  }
  size_t write(const uint8_t *b, size_t n) override {
    Serial0.write(b, n);
    if (Serial) Serial.write(b, n);
    return n;
  }
};
extern DualLog LOG;
