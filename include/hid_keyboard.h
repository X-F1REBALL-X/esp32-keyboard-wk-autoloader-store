#pragma once

#include <Arduino.h>
#include <stdint.h>

bool hidKeyboardBegin();
void hidKeyboardLoop();
void hidSendKey(uint8_t keycode, uint8_t modifier = 0);
void hidReleaseAll();
uint8_t hidKeyFromName(const char *name);
