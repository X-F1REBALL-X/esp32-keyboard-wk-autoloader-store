#pragma once

#include <IPAddress.h>
#include <stdint.h>

bool consolePortOpen(IPAddress ip, uint16_t port, uint32_t timeoutMs = 200);
bool consoleIsReady(IPAddress ip);

// Incremental LAN scan. Call scanBegin once, then scanStep each loop until
// it returns true (done). If a console was found, found is set and hasFound
// is true.
void consoleScanBegin();
bool consoleScanStep(IPAddress &found, bool &hasFound);
