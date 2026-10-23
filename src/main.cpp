#include <Arduino.h>
#include <LittleFS.h>

#include "config.h"
#include "wifi_portal.h"
#include "log.h"
#include "hid_keyboard.h"
#include "nav_sequence.h"
#include "console_detect.h"
#include "setup_http.h"

enum class Phase : uint8_t {
  Boot,
  SetupOnly,    // SoftAP, no home Wi-Fi yet - serve setup page only
  WaitMinUsb,   // home mode: pick scan or fixed IP (no fixed wait here)
  ScanLan,      // home mode: optional incremental console scan
  Probe,        // home mode: poll console ports
  Backoff,      // home mode: wait before retry
  WaitBoot,     // console found: hold keys until kMinWaitAfterUsbMs since boot
  Navigate,
  Done,
  Failed,
};

static Phase sPhase = Phase::Boot;
static int sAttempt = 0;
static uint32_t sPhaseSince = 0;
static uint32_t sLastProbe = 0;
static IPAddress sConsoleIp;
static bool sAutoIp = false;  // console IP came from auto-scan, not wifi.json

DualLog LOG;

static void setPhase(Phase p) {
  sPhase = p;
  sPhaseSince = millis();
}

void setup() {
  LOG.begin(115200);
  delay(500);
  LOG.println();
  LOG.println("=== AutoStore " AUTOSTORE_VERSION " ===");
  LOG.println("USB HID keyboard only - opens Store (LEFT, ENTER)");

  if (!LittleFS.begin(true)) {
    LOG.println("[fs] LittleFS mount failed");
  } else {
    LOG.println("[fs] LittleFS mounted");
  }

  hidKeyboardBegin();
  wifiPortalBegin();
  setupHttpBegin();
  navSequenceLoad(kSequencePath);

  if (wifiPortalHomeMode()) {
    setPhase(Phase::WaitMinUsb);
    if (wifiPortalConsoleIpKnown()) {
      LOG.printf("[main] home LAN - will probe console %s now\n",
                    wifiPortalConsoleIp().toString().c_str());
    } else {
      LOG.println("[main] home LAN - will auto-scan console now");
    }
  } else {
    setPhase(Phase::SetupOnly);
    LOG.println("[main] SoftAP setup only - open http://10.1.1.1/");
  }
}

void loop() {
  wifiPortalLoop();
  setupHttpLoop();
  hidKeyboardLoop();

  // Fixed timer: send LEFT + ENTER once, kFixedKeyDelayMs after power-on,
  // whether or not the console was seen on the network.
  static bool sKeysSent = false;
  if (!sKeysSent && millis() >= kFixedKeyDelayMs) {
    sKeysSent = true;
    LOG.printf("[main] t=%lus - sending keys\n", (unsigned long)(millis() / 1000));
    if (navSequencePlay()) LOG.println("[main] keys sent - done");
    setPhase(Phase::Done);
  }
  if (sKeysSent) return;

  switch (sPhase) {
  case Phase::Boot:
    setPhase(wifiPortalHomeMode() ? Phase::WaitMinUsb : Phase::SetupOnly);
    break;

  case Phase::SetupOnly:
    // Fresh board / SoftAP-only: never probe. Setup page + captive portal only.
    if (wifiPortalHomeMode()) {
      LOG.println("[main] home Wi-Fi came up - starting console search");
      setPhase(Phase::WaitMinUsb);
    }
    break;

  case Phase::WaitMinUsb:
    // Search runs in parallel with the USB wait; the wait is enforced later.
    sAutoIp = !wifiPortalConsoleIpKnown();
    if (sAutoIp) {
      consoleScanBegin();
      setPhase(Phase::ScanLan);
    } else {
      sConsoleIp = wifiPortalConsoleIp();
      sLastProbe = 0;
      setPhase(Phase::Probe);
      LOG.printf("[main] probing %s\n", sConsoleIp.toString().c_str());
    }
    break;

  case Phase::ScanLan: {
    IPAddress found;
    bool hasFound = false;
    bool done = consoleScanStep(found, hasFound);
    if (!done) break;
    if (hasFound) {
      wifiPortalSetConsoleIp(found);
      sConsoleIp = found;
      sLastProbe = 0;
      setPhase(Phase::Probe);
      LOG.printf("[main] probing %s\n", sConsoleIp.toString().c_str());
    } else {
      // Console may still be off or booting: keep rescanning, never give up.
      LOG.println("[main] auto-scan found nothing - rescan after backoff");
      setPhase(Phase::Backoff);
    }
    break;
  }

  case Phase::Probe:
    if (sLastProbe != 0 && millis() - sLastProbe < kDetectPollMs) break;
    sLastProbe = millis();
    if (consoleIsReady(sConsoleIp)) {
      LOG.printf("[main] console ready at t=%lus\n",
                 (unsigned long)(millis() / 1000));
      setPhase(Phase::WaitBoot);
      break;
    }
    if (sAutoIp && millis() - sPhaseSince >= 30000) {
      LOG.println("[main] auto console IP lost - rescanning");
      wifiPortalClearConsoleIp();
      setPhase(Phase::Backoff);
      break;
    }
    if (millis() - sPhaseSince >= kDetectTimeoutMs) {
      sAttempt++;
      if (sAttempt >= kMaxRetries) {
        setPhase(Phase::Failed);
      } else {
        LOG.printf("[main] detect fail - retry %d/%d after backoff\n", sAttempt,
                      kMaxRetries);
        setPhase(Phase::Backoff);
      }
    }
    break;

  case Phase::Backoff:
    if (millis() - sPhaseSince < kRetryBackoffMs) break;
    if (!wifiPortalConsoleIpKnown()) {
      sAutoIp = true;
      consoleScanBegin();
      setPhase(Phase::ScanLan);
    } else {
      sConsoleIp = wifiPortalConsoleIp();
      sLastProbe = 0;
      setPhase(Phase::Probe);
    }
    break;

  case Phase::WaitBoot:
    // Keys go out at max(console found, kMinWaitAfterUsbMs since power-on),
    // so USB HID is enumerated on the console before LEFT + ENTER.
    if (millis() < kMinWaitAfterUsbMs) break;
    setPhase(Phase::Navigate);
    break;

  case Phase::Navigate: {
    LOG.printf("[main] attempt %d/%d - HID navigate\n", sAttempt + 1,
                  kMaxRetries);
    if (!navSequencePlay()) {
      LOG.println("[main] empty sequence");
      setPhase(Phase::Failed);
      break;
    }
    LOG.println("[main] keys sent - done");
    setPhase(Phase::Done);
    break;
  }

  case Phase::Done:
    break;

  case Phase::Failed:
    if (millis() - sPhaseSince < 2000) {
      LOG.println("[main] FAILED after limited retries - setup page still up");
      sPhaseSince = millis();
    }
    break;
  }
}
