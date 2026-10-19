#include "log.h"
#include "nav_sequence.h"
#include "hid_keyboard.h"
#include "config.h"

#include <LittleFS.h>

static NavStep sSteps[kMaxNavSteps];
static size_t sCount = 0;

// Built-in default: open PS Store tile (fixed slot).
// Focus starts one slot right of Store: one LEFT, then Enter.
// Tune on the target console - timings depend on language and UI.
// Line format: <delay_seconds> <KEY>
static const char *kDefaultSequence =
    "0.3 LEFT\n"
    "0.3 ENTER\n";

bool navSequenceLoad(const char *path) {
  sCount = 0;
  File f = LittleFS.open(path, "r");
  String body;
  if (f) {
    body = f.readString();
    f.close();
    LOG.printf("[nav] loaded %s (%u bytes)\n", path, (unsigned)body.length());
  } else {
    LOG.printf("[nav] %s missing - using built-in default\n", path);
    body = kDefaultSequence;
  }

  int start = 0;
  while (start < (int)body.length() && sCount < kMaxNavSteps) {
    int nl = body.indexOf('\n', start);
    if (nl < 0) nl = body.length();
    String line = body.substring(start, nl);
    start = nl + 1;
    line.trim();
    if (line.isEmpty() || line.startsWith("#")) continue;

    int sp = line.indexOf(' ');
    if (sp < 0) continue;
    String dStr = line.substring(0, sp);
    String kStr = line.substring(sp + 1);
    kStr.trim();
    float d = dStr.toFloat();
    uint32_t delayMs;
    if (dStr.indexOf('.') >= 0 || d < 50.0f) {
      delayMs = (uint32_t)(d * 1000.0f);
    } else {
      delayMs = (uint32_t)d;
    }
    uint8_t code = hidKeyFromName(kStr.c_str());
    if (!code) continue;
    sSteps[sCount].delayMs = delayMs;
    sSteps[sCount].keycode = code;
    strncpy(sSteps[sCount].name, kStr.c_str(), sizeof(sSteps[sCount].name) - 1);
    sSteps[sCount].name[sizeof(sSteps[sCount].name) - 1] = 0;
    sCount++;
  }
  LOG.printf("[nav] %u steps ready\n", (unsigned)sCount);
  return sCount > 0;
}

bool navSequencePlay() {
  if (sCount == 0) return false;
  LOG.println("[nav] playing sequence...");
  for (size_t i = 0; i < sCount; i++) {
    delay(sSteps[i].delayMs);
    LOG.printf("[nav] key %s (0x%02x)\n", sSteps[i].name, sSteps[i].keycode);
    hidSendKey(sSteps[i].keycode);
  }
  hidReleaseAll();
  LOG.println("[nav] sequence done");
  return true;
}
