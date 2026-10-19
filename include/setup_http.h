#pragma once

// Tiny SoftAP setup page at http://10.1.1.1 (SSID AutoStore).
// Lets the user save home Wi-Fi + console IP to LittleFS without rebuilding.

bool setupHttpBegin();
void setupHttpLoop();
