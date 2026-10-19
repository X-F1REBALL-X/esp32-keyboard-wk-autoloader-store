#include "log.h"
#include <lwip/def.h>
#include "console_detect.h"
#include "config.h"

#include <WiFi.h>
#include <Preferences.h>

static uint32_t sHint = 0;  // last console IP (host order), tried first

static uint32_t loadHint() {
  Preferences p;
  if (!p.begin("autostore", true)) return 0;
  uint32_t v = p.getUInt("last_ip", 0);
  p.end();
  return v;
}

static void saveHint(uint32_t v) {
  Preferences p;
  if (!p.begin("autostore", false)) return;
  if (p.getUInt("last_ip", 0) != v) p.putUInt("last_ip", v);
  p.end();
}

bool consolePortOpen(IPAddress ip, uint16_t port, uint32_t timeoutMs) {
  WiFiClient c;
  c.setTimeout(timeoutMs);
  bool ok = c.connect(ip, port, timeoutMs);
  if (ok) c.stop();
  return ok;
}

bool consoleIsReady(IPAddress ip) {
  return consolePortOpen(ip, kPs5PortA, 200) ||
         consolePortOpen(ip, kPs5PortB, 200);
}

static uint32_t sScanHost = 0;
static uint32_t sScanBcast = 0;
static uint32_t sScanSelf = 0;
static uint32_t sScanGw = 0;
static bool sScanActive = false;

void consoleScanBegin() {
  sScanActive = false;
  if (WiFi.status() != WL_CONNECTED) return;
  IPAddress self = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  IPAddress gw = WiFi.gatewayIP();
  // IPAddress casts to network byte order; scan in host order so ++ walks
  // the last octet (192.168.1.1, .2, ...) instead of the first.
  uint32_t ip = ntohl((uint32_t)self);
  uint32_t m = ntohl((uint32_t)mask);
  uint32_t net = ip & m;
  sScanBcast = net | ~m;
  sScanSelf = ip;
  sScanGw = ntohl((uint32_t)gw);
  sScanHost = net + 1;
  sHint = loadHint();
  if ((sHint & m) != net || sHint == ip) sHint = 0;
  sScanActive = true;
  LOG.printf("[detect] scanning LAN %s mask %s\n", self.toString().c_str(),
                mask.toString().c_str());
}

bool consoleScanStep(IPAddress &found, bool &hasFound) {
  hasFound = false;
  if (!sScanActive) return true;
  if (sHint) {
    // Try the console address from the last boot before walking the LAN.
    uint32_t h = sHint;
    sHint = 0;
    IPAddress cand(htonl(h));
    if (consolePortOpen(cand, kPs5PortA, kScanHostTimeoutMs * 2) ||
        consolePortOpen(cand, kPs5PortB, kScanHostTimeoutMs)) {
      found = cand;
      hasFound = true;
      sScanActive = false;
      LOG.printf("[detect] found console at last IP %s\n", found.toString().c_str());
      return true;
    }
    return false;
  }
  if (sScanHost >= sScanBcast) {
    sScanActive = false;
    LOG.println("[detect] LAN scan found no console");
    return true;
  }

  uint32_t host = sScanHost++;
  if (host == sScanSelf || host == sScanGw) return false;

  IPAddress cand(htonl(host));
  if ((host & 0x1f) == 1)
    LOG.printf("[detect] scan at %s\n", cand.toString().c_str());
  // First connect also has to resolve ARP over Wi-Fi, 40 ms was too short.
  if (consolePortOpen(cand, kPs5PortA, kScanHostTimeoutMs) ||
      consolePortOpen(cand, kPs5PortB, kScanHostTimeoutMs / 2)) {
    found = cand;
    hasFound = true;
    sScanActive = false;
    LOG.printf("[detect] found console at %s\n", found.toString().c_str());
    saveHint(host);
    return true;
  }
  return false;
}
