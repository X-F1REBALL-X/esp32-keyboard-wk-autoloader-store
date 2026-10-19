#include "log.h"
#include "hid_keyboard.h"

#include "USB.h"
#include "USBHIDKeyboard.h"

static USBHIDKeyboard sKeyboard;
static bool sReady = false;

bool hidKeyboardBegin() {
  // USB HID keyboard only - never spoof a gamepad / controller.
  USB.begin();
  sKeyboard.begin();
  sReady = true;
  LOG.println("[hid] USB HID keyboard ready");
  return true;
}

void hidKeyboardLoop() {
  // Serviced by the USB stack.
}

void hidSendKey(uint8_t keycode, uint8_t modifier) {
  if (!sReady) return;
#if defined(ARDUINO_USB_MODE)
  sKeyboard.pressRaw(keycode);
  delay(50);
  sKeyboard.releaseRaw(keycode);
#else
  (void)modifier;
  sKeyboard.write(keycode);
  delay(50);
#endif
  delay(30);
}

void hidReleaseAll() {
  if (!sReady) return;
  sKeyboard.releaseAll();
}

uint8_t hidKeyFromName(const char *name) {
  if (!name) return 0;
  // USB HID usage IDs (keyboard page)
  if (!strcasecmp(name, "ENTER") || !strcasecmp(name, "RETURN")) return 0x28;
  if (!strcasecmp(name, "ESC") || !strcasecmp(name, "ESCAPE")) return 0x29;
  if (!strcasecmp(name, "BACKSPACE")) return 0x2a;
  if (!strcasecmp(name, "TAB")) return 0x2b;
  if (!strcasecmp(name, "SPACE")) return 0x2c;
  if (!strcasecmp(name, "UP")) return 0x52;
  if (!strcasecmp(name, "DOWN")) return 0x51;
  if (!strcasecmp(name, "RIGHT")) return 0x4f;
  if (!strcasecmp(name, "LEFT")) return 0x50;
  if (!strcasecmp(name, "PAGE_UP")) return 0x4b;
  if (!strcasecmp(name, "PAGE_DOWN")) return 0x4e;
  if (strlen(name) == 1) {
    char c = name[0];
    if (c >= 'a' && c <= 'z') return (uint8_t)(0x04 + (c - 'a'));
    if (c >= 'A' && c <= 'Z') return (uint8_t)(0x04 + (c - 'A'));
  }
  if (!strcasecmp(name, "A")) return 0x04;
  if (!strcasecmp(name, "B")) return 0x05;
  if (!strcasecmp(name, "C")) return 0x06;
  if (!strcasecmp(name, "D")) return 0x07;
  if (!strcasecmp(name, "E")) return 0x08;
  if (!strcasecmp(name, "F")) return 0x09;
  if (!strcasecmp(name, "G")) return 0x0a;
  if (!strcasecmp(name, "H")) return 0x0b;
  if (!strcasecmp(name, "I")) return 0x0c;
  if (!strcasecmp(name, "J")) return 0x0d;
  if (!strcasecmp(name, "K")) return 0x0e;
  if (!strcasecmp(name, "L")) return 0x0f;
  if (!strcasecmp(name, "M")) return 0x10;
  if (!strcasecmp(name, "N")) return 0x11;
  if (!strcasecmp(name, "O")) return 0x12;
  if (!strcasecmp(name, "P")) return 0x13;
  if (!strcasecmp(name, "Q")) return 0x14;
  if (!strcasecmp(name, "R")) return 0x15;
  if (!strcasecmp(name, "S")) return 0x16;
  if (!strcasecmp(name, "T")) return 0x17;
  if (!strcasecmp(name, "U")) return 0x18;
  if (!strcasecmp(name, "V")) return 0x19;
  if (!strcasecmp(name, "W")) return 0x1a;
  if (!strcasecmp(name, "X")) return 0x1b;
  if (!strcasecmp(name, "Y")) return 0x1c;
  if (!strcasecmp(name, "Z")) return 0x1d;
  LOG.printf("[hid] unknown key name: %s\n", name);
  return 0;
}
