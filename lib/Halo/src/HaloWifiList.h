// HaloWifiList — the networks a scan found, one log line each: "wifi: net <dBm> <0 — open, 1 — a password> <name>"
// (PopkaStudio reads them: studio.py, the Functions menu's list of networks). Then the scan's results are let go.
// HaloClock.h's "wifi scan" and a sketch without HaloClock (PopkaWatch's wifiscan.h) both print them this way:
//   const int n = WiFi.scanNetworks();  halo_wifi_list(n);
#pragma once

#include <Halo.h>
#include <WiFi.h>

static inline void halo_wifi_list(int n) {
  for (int i = 0; i < n; i++)
    if (WiFi.SSID(i).length())  // (hidden networks have no name)
      LOG("wifi: net %d %d %s\n", (int)WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? 0 : 1, WiFi.SSID(i).c_str());
  WiFi.scanDelete();
}
