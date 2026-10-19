#include "log.h"
#include "wifi_portal.h"
#include "config.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "setup_http.h"

static bool sApUp = false;
static DNSServer sDns;
static bool sStaUp = false;
static bool sHomeMode = false;
static bool sConsoleKnown = false;
static IPAddress sConsoleIp(0, 0, 0, 0);

static String jsonGetString(const String &body, const char *key) {
  String needle = String("\"") + key + "\"";
  int k = body.indexOf(needle);
  if (k < 0) return String();
  int colon = body.indexOf(':', k + needle.length());
  if (colon < 0) return String();
  int q1 = body.indexOf('"', colon + 1);
  if (q1 < 0) return String();
  // Unescape \" and \\ written by the setup page.
  String out;
  for (int i = q1 + 1; i < (int)body.length(); i++) {
    char c = body[i];
    if (c == '\\' && i + 1 < (int)body.length()) {
      out += body[++i];
      continue;
    }
    if (c == '"') return out;
    out += c;
  }
  return String();
}

static bool sStaWanted = false;
static uint8_t sLastReason = 0;

static void onWifiEvent(WiFiEvent_t ev, WiFiEventInfo_t info) {
  switch (ev) {
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    LOG.println("[wifi] STA associated");
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    sLastReason = info.wifi_sta_disconnected.reason;
    LOG.printf("[wifi] STA disconnected reason=%u\n", sLastReason);
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    LOG.printf("[wifi] STA got IP %s\n", WiFi.localIP().toString().c_str());
    break;
  default:
    break;
  }
}

static bool parseIp(const String &s, IPAddress &out) {
  return out.fromString(s);
}

static bool startSoftAp() {
  WiFi.softAPConfig(kApIp, kApGateway, kApSubnet);
  if (!WiFi.softAP(kApSsid, kApPass)) {
    LOG.println("[wifi] softAP failed");
    return false;
  }
  sDns.setTTL(30);
  sDns.setErrorReplyCode(DNSReplyCode::NoError);
  // Captive portal: every DNS name resolves to the SoftAP IP.
  sDns.start(53, "*", kApIp);
  sApUp = true;
  LOG.printf("[wifi] SoftAP %s IP %s (captive DNS)\n", kApSsid,
                kApIp.toString().c_str());
  return true;
}

static bool joinHomeSta(const String &ssid, const String &pass, bool useStatic,
                        IPAddress ip, IPAddress gw, IPAddress mask) {
  if (ssid.isEmpty()) {
    LOG.println("[wifi] home mode but sta_ssid empty");
    return false;
  }

  if (useStatic) {
    if (!WiFi.config(ip, gw, mask)) {
      LOG.println("[wifi] STA static IP config failed");
    } else {
      LOG.printf("[wifi] STA static IP %s gw %s\n", ip.toString().c_str(),
                    gw.toString().c_str());
    }
  } else {
    // DHCP (default)
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    LOG.println("[wifi] STA using DHCP");
  }

  LOG.printf("[wifi] STA joining \"%s\"...\n", ssid.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  // Non-blocking: the join runs in the background while the USB wait counts
  // down. wifiPortalLoop() flips to home mode once the STA has an IP.
  return false;
}

bool wifiPortalBegin() {
  sApUp = false;
  sStaUp = false;
  sHomeMode = false;
  sConsoleKnown = false;
  sConsoleIp = IPAddress(0, 0, 0, 0);
  WiFi.onEvent(onWifiEvent);

  String body;
  if (LittleFS.exists(kWifiConfigPath)) {
    File f = LittleFS.open(kWifiConfigPath, "r");
    if (f) {
      body = f.readString();
      f.close();
    }
  }

  LOG.printf("[wifi] wifi.json %s (%u bytes)\n",
             LittleFS.exists(kWifiConfigPath) ? "found" : "missing",
             (unsigned)body.length());
  String mode = jsonGetString(body, "mode");
  mode.toLowerCase();

  if (mode == "home" && body.length() > 0) {
    sHomeMode = true;
    String ssid = jsonGetString(body, "sta_ssid");
    String pass = jsonGetString(body, "sta_pass");
    IPAddress staIp, gw, mask, consoleIp;
    String sip = jsonGetString(body, "sta_ip");
    String sgw = jsonGetString(body, "sta_gateway");
    String sms = jsonGetString(body, "sta_subnet");
    String cip = jsonGetString(body, "console_ip");
    LOG.printf("[wifi] loaded ssid=\"%s\" pass_len=%u console_ip=%s\n",
               ssid.c_str(), (unsigned)pass.length(),
               cip.isEmpty() ? "(auto)" : cip.c_str());

    bool useStatic = false;
    if (!sip.isEmpty() && parseIp(sip, staIp)) {
      useStatic = true;
      if (!sgw.isEmpty())
        parseIp(sgw, gw);
      else
        gw = IPAddress(0, 0, 0, 0);
      if (!sms.isEmpty())
        parseIp(sms, mask);
      else
        mask = IPAddress(255, 255, 255, 0);
    }

    if (!cip.isEmpty() && parseIp(cip, consoleIp)) {
      sConsoleIp = consoleIp;
      sConsoleKnown = true;
    }

    WiFi.mode(WIFI_AP_STA);
    bool joined = joinHomeSta(ssid, pass, useStatic, staIp, gw, mask);
    if (!joined) {
      // Old code switched to WIFI_AP here and never tried again until reboot.
      // Stay in AP+STA so a late router (or a slow DHCP) still gets joined.
      LOG.println("[wifi] home STA joining in background - SoftAP stays up");
      sStaWanted = !ssid.isEmpty();
    }
    if (!startSoftAp()) return false;
    if (sStaUp) {
      // Modem sleep delays replies by a beacon interval and breaks scan timing.
      WiFi.setSleep(false);
      LOG.printf("[wifi] mode=home board=%s console=%s\n",
                    WiFi.localIP().toString().c_str(),
                    sConsoleKnown ? sConsoleIp.toString().c_str() : "(auto)");
    } else {
      LOG.println("[wifi] mode=softap-fallback");
    }
    return true;
  }

  WiFi.mode(WIFI_AP);
  if (!startSoftAp()) return false;
  LOG.println("[wifi] mode=softap (open http://10.1.1.1 to configure)");
  return true;
}

void wifiPortalLoop() {
  if (sApUp) sDns.processNextRequest();
  if (sStaWanted && !sStaUp && WiFi.status() == WL_CONNECTED) {
    sStaUp = true;
    sStaWanted = false;
    WiFi.setSleep(false);
    LOG.printf("[wifi] STA joined late IP %s - home mode\n",
               WiFi.localIP().toString().c_str());
  }
}

uint8_t wifiPortalLastStaReason() { return sLastReason; }

bool wifiPortalHasApClient() { return WiFi.softAPgetStationNum() > 0; }

IPAddress wifiPortalConsoleIp() { return sConsoleIp; }

bool wifiPortalConsoleIpKnown() { return sConsoleKnown; }

bool wifiPortalHomeMode() { return sHomeMode && sStaUp; }

void wifiPortalClearConsoleIp() {
  sConsoleIp = IPAddress(0, 0, 0, 0);
  sConsoleKnown = false;
}

void wifiPortalSetConsoleIp(IPAddress ip) {
  sConsoleIp = ip;
  sConsoleKnown = true;
}
