// Range test: every 10 s prints the WiFi networks heard (with RSSI) and the BLE devices heard
// (with RSSI), flagging names that look like an OBD adapter. It also joins "Vitoi" (demo network)
// and reports the RSSI and IP. Nothing is written to flash or the network apart from joining.
#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

static const char *HOME_SSID = "Vitoi";
static const char *HOME_PASS = "10203040";   // demo network, per the owner
static bool looksLikeObd(const String &n) {
  String u = n; u.toUpperCase();
  return u.indexOf("OBD") >= 0 || u.indexOf("ELM") >= 0 || u.indexOf("KONNWEI") >= 0 || u.indexOf("VLINK") >= 0 || u.indexOf("IOS-") >= 0;
}

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("RANGETEST start");
  Serial.printf("PSRAM %u KB, heap %u KB\n", ESP.getPsramSize() / 1024, ESP.getFreeHeap() / 1024);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("rangetest");
  NimBLEDevice::init("");
}

void loop() {
  static uint32_t joinStart = 0;
  int n = WiFi.scanNetworks();
  Serial.printf("\nWiFi scan: %d networks\n", n);
  for (int i = 0; i < n && i < 10; i++) Serial.printf("  %-24s ch%-2d %4d dBm\n", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
  WiFi.scanDelete();

  if (WiFi.status() != WL_CONNECTED) {
    if (!joinStart) { WiFi.begin(HOME_SSID, HOME_PASS); joinStart = millis(); Serial.printf("joining \"%s\"...\n", HOME_SSID); }
    else if (millis() - joinStart > 20000) { WiFi.disconnect(); joinStart = 0; Serial.println("join timed out"); }
  } else {
    Serial.printf("WiFi joined \"%s\": %s  RSSI %d dBm\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  NimBLEScanResults r = scan->start(5, false);
  Serial.printf("BLE scan: %d devices\n", r.getCount());
  for (int i = 0; i < r.getCount(); i++) {
    NimBLEAdvertisedDevice d = r.getDevice(i);
    String name = d.getName().c_str();
    Serial.printf("  %s %4d dBm %s%s\n", d.getAddress().toString().c_str(), d.getRSSI(), name.c_str(), looksLikeObd(name) ? "   <-- OBD adapter?" : "");
  }
  scan->clearResults();
  delay(3000);
}
