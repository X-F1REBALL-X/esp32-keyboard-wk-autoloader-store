#pragma once

#include <Arduino.h>
#include <IPAddress.h>

bool wifiPortalBegin();
void wifiPortalLoop();

// True when SoftAP has at least one client (SoftAP-only detect path)
bool wifiPortalHasApClient();

// Console IP from wifi.json when set; otherwise 0.0.0.0 until scanned
IPAddress wifiPortalConsoleIp();

// True when wifi.json included a console_ip
bool wifiPortalConsoleIpKnown();

// Home mode uses STA; SoftAP always stays up for the setup page
bool wifiPortalHomeMode();

// Remember a scanned console IP for this boot
void wifiPortalSetConsoleIp(IPAddress ip);
void wifiPortalClearConsoleIp();
uint8_t wifiPortalLastStaReason();
