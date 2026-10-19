#pragma once

#include <Arduino.h>

struct NavStep {
  uint32_t delayMs;
  uint8_t keycode;
  char name[16];
};

static const size_t kMaxNavSteps = 64;

bool navSequenceLoad(const char *path);
bool navSequencePlay();
