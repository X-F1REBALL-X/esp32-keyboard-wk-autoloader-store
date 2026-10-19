#include "log.h"
#include "setup_http.h"
#include "config.h"

#include <WebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

static WebServer sServer(80);
static bool sUp = false;

static String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

static String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n' || c == '\r') {
      continue;
    } else {
      out += c;
    }
  }
  return out;
}

static String readWifiField(const char *key) {
  if (!LittleFS.exists(kWifiConfigPath)) return String();
  File f = LittleFS.open(kWifiConfigPath, "r");
  if (!f) return String();
  String body = f.readString();
  f.close();
  String needle = String("\"") + key + "\"";
  int k = body.indexOf(needle);
  if (k < 0) return String();
  int colon = body.indexOf(':', k + needle.length());
  if (colon < 0) return String();
  int q1 = body.indexOf('"', colon + 1);
  if (q1 < 0) return String();
  int q2 = body.indexOf('"', q1 + 1);
  if (q2 < 0) return String();
  return body.substring(q1 + 1, q2);
}

static const char *kPageHead =
    "<!doctype html><html><head><meta charset=utf-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>AutoStore setup</title>"
    "<style>"
    "body{font-family:system-ui,sans-serif;background:#111;color:#eee;"
    "max-width:28rem;margin:2rem auto;padding:0 1rem}"
    "label{display:block;margin:.8rem 0 .2rem}"
    "input{width:100%;box-sizing:border-box;padding:.5rem;border-radius:.4rem;"
    "border:1px solid #444;background:#1a1a1a;color:#eee}"
    "button{margin-top:1.2rem;width:100%;padding:.7rem;border:0;border-radius:.4rem;"
    "background:#3b82f6;color:#fff;font-weight:600}"
    "p,.hint{color:#aaa;font-size:.9rem;line-height:1.4}"
    ".ok{color:#4ade80}"
    "code{color:#ddd}"
    "</style></head><body>";

static void handleRoot() {
  String ssid = readWifiField("sta_ssid");
  String consoleIp = readWifiField("console_ip");
  String staIp = readWifiField("sta_ip");
  String gw = readWifiField("sta_gateway");

  String html = kPageHead;
  html += "<h1>AutoStore</h1>";
  html += "<p>Enter your home Wi-Fi (2.4 GHz). The board saves settings and "
          "reboots. The console can use Wi-Fi or Ethernet on the same LAN.</p>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p class=ok>Board is on home Wi-Fi. IP: <code>";
    html += WiFi.localIP().toString();
    html += "</code></p>";
  }
  html += "<form method=POST action=/save>";
  html += "<label>Home Wi-Fi name (SSID)</label>";
  html += "<input name=ssid required value=\"" + htmlEscape(ssid) + "\">";
  html += "<label>Home Wi-Fi password</label>";
  html += "<input name=pass type=password placeholder=\"leave blank to keep current\">";
  html += "<label>Console IP (optional)</label>";
  html += "<input name=console_ip placeholder=\"blank = auto-detect on LAN\" value=\"" +
          htmlEscape(consoleIp) + "\">";
  html += "<p class=hint>Find it on the PS5: Settings &gt; Network &gt; "
          "Connection Status. Leave blank to let the board scan the LAN.</p>";
  html += "<label>Board static IP (optional)</label>";
  html += "<input name=sta_ip placeholder=\"blank = DHCP\" value=\"" +
          htmlEscape(staIp) + "\">";
  html += "<label>Router / gateway IP (only if static board IP)</label>";
  html += "<input name=gateway placeholder=\"needed with static board IP\" value=\"" +
          htmlEscape(gw) + "\">";
  html += "<button type=submit>Save and reboot</button>";
  html += "</form></body></html>";
  sServer.send(200, "text/html", html);
}

static void handleSave() {
  if (!sServer.hasArg("ssid")) {
    sServer.send(400, "text/plain", "missing ssid");
    return;
  }
  String ssid = sServer.arg("ssid");
  String pass = sServer.arg("pass");
  String consoleIp = sServer.arg("console_ip");
  String staIp = sServer.arg("sta_ip");
  String gw = sServer.arg("gateway");

  if (pass.isEmpty()) {
    String old = readWifiField("sta_pass");
    if (!old.isEmpty()) pass = old;
  }

  String json = "{\n";
  json += "  \"mode\": \"home\",\n";
  json += "  \"sta_ssid\": \"" + jsonEscape(ssid) + "\",\n";
  json += "  \"sta_pass\": \"" + jsonEscape(pass) + "\",\n";
  if (!staIp.isEmpty()) {
    json += "  \"sta_ip\": \"" + jsonEscape(staIp) + "\",\n";
    if (!gw.isEmpty())
      json += "  \"sta_gateway\": \"" + jsonEscape(gw) + "\",\n";
    json += "  \"sta_subnet\": \"255.255.255.0\",\n";
  }
  if (!consoleIp.isEmpty()) {
    json += "  \"console_ip\": \"" + jsonEscape(consoleIp) + "\",\n";
  }
  json += "  \"softap\": true\n";
  json += "}\n";

  File f = LittleFS.open(kWifiConfigPath, "w");
  if (!f) {
    sServer.send(500, "text/plain", "could not write wifi.json");
    return;
  }
  f.print(json);
  f.close();
  LOG.println("[setup] saved /wifi.json - rebooting");

  String html = kPageHead;
  html += "<h1 class=ok>Saved</h1>";
  html += "<p>Board is rebooting and will join your home Wi-Fi (DHCP unless you "
          "set a static IP). SoftAP AutoStore stays up so you can open "
          "http://10.1.1.1 again. After it joins, this page also shows the "
          "board LAN IP.</p></body></html>";
  sServer.send(200, "text/html", html);
  delay(500);
  ESP.restart();
}


static void handleCaptiveRedirect() {
  sServer.sendHeader("Location", "http://10.1.1.1/", true);
  sServer.send(302, "text/plain", "");
}

static void handleGenerate204() {
  // Android captive check: 302 forces the portal browser open.
  handleCaptiveRedirect();
}

static void handleHotspotDetect() {
  // iOS/macOS captive check
  sServer.send(200, "text/html",
               "<HTML><HEAD><TITLE>Success</TITLE></HEAD>"
               "<BODY>Success<script>location.href=\"http://10.1.1.1/\";</script>"
               "</BODY></HTML>");
}

static void handleConnectTest() {
  // Windows NCSI
  handleCaptiveRedirect();
}

bool setupHttpBegin() {
  sServer.on("/", HTTP_GET, handleRoot);
  sServer.on("/save", HTTP_POST, handleSave);
  sServer.on("/generate_204", HTTP_GET, handleGenerate204);
  sServer.on("/gen_204", HTTP_GET, handleGenerate204);
  sServer.on("/hotspot-detect.html", HTTP_GET, handleHotspotDetect);
  sServer.on("/library/test/success.html", HTTP_GET, handleHotspotDetect);
  sServer.on("/connecttest.txt", HTTP_GET, handleConnectTest);
  sServer.on("/ncsi.txt", HTTP_GET, handleConnectTest);
  sServer.on("/canonical.html", HTTP_GET, handleCaptiveRedirect);
  sServer.on("/success.txt", HTTP_GET, handleCaptiveRedirect);
  sServer.onNotFound(handleRoot);
  sServer.begin();
  sUp = true;
  LOG.println("[setup] SoftAP setup page at http://10.1.1.1/");
  return true;
}

void setupHttpLoop() {
  if (sUp) sServer.handleClient();
}
