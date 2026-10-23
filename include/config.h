#pragma once

#include <IPAddress.h>

// SoftAP fallback (always available unless disabled in wifi.json)
static const char *kApSsid = "AutoStore";
static const char *kApPass = "12345678";
static const IPAddress kApIp(10, 1, 1, 1);
static const IPAddress kApGateway(10, 1, 1, 1);
static const IPAddress kApSubnet(255, 255, 255, 0);

// Console OS readiness: either port open means the system is up
static const uint16_t kPs5PortA = 9295;
static const uint16_t kPs5PortB = 41800;
static const uint32_t kMinWaitAfterUsbMs = 16000;
// Keys go out this long after power-on, no network wait
static const uint32_t kFixedKeyDelayMs = 18500;
static const uint32_t kDetectPollMs = 1000;
static const uint32_t kDetectTimeoutMs = 180000;

// Limited retries of detect + HID cycle
static const int kMaxRetries = 3;
static const uint32_t kRetryBackoffMs = 10000;

// Auto-scan: per host connect timeout (first port, second gets half)
static const uint32_t kScanHostTimeoutMs = 150;

// LittleFS paths
static const char *kSequencePath = "/config/nav_sequence.txt";
static const char *kWifiConfigPath = "/wifi.json";

// SoftAP default first DHCP client (used only in SoftAP-only path)
static const IPAddress kDefaultClientIp(10, 1, 1, 2);

// STA join timeout
static const uint32_t kStaConnectTimeoutMs = 30000;
