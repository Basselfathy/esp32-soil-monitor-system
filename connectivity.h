#pragma once
#include <WiFi.h>
#include <time.h>
#include "config.h"

// Forward declaration (syncNTP called from ensureWifi before its definition)
void syncNTP();

// ── State ─────────────────────────────────────────────
bool          ntpSynced     = false;
unsigned long lastWifiRetry = 0;
unsigned long lastNtpAttempt = 0;

// ── WiFi ──────────────────────────────────────────────
static bool connectToNetwork(const char *ssid, const char *pass)
{
  logf("Trying %s", ssid);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT) {
      WiFi.disconnect(true);
      log(" timed out");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  logf(" connected! IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

bool connectWifi()
{
  if (connectToNetwork(WIFI_SSID1, WIFI_PASS1)) return true;
  if (connectToNetwork(WIFI_SSID2, WIFI_PASS2)) return true;
  log("Both networks failed");
  return false;
}

void ensureWifi()
{
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastWifiRetry < WIFI_RETRY) return;
  lastWifiRetry = now;
  log("WiFi lost — reconnecting...");
  WiFi.disconnect(true);
  delay(100);
  if (connectWifi() && !ntpSynced)
    syncNTP();
}

// ── NTP ───────────────────────────────────────────────
void syncNTP()
{
  if (WiFi.status() != WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastNtpAttempt < NTP_RETRY) return;
  lastNtpAttempt = now;

  configTime(10800, 0, "pool.ntp.org"); // UTC+3; adjust for your timezone
  Serial.print("Waiting for NTP");
  struct tm timeinfo;
  unsigned long start = millis();
  while (!getLocalTime(&timeinfo)) {
    if (millis() - start > 10000) {
      log(" timed out — will retry");
      return;
    }
    delay(500);
    Serial.print(".");
  }
  log(" synced");
  ntpSynced = true;
}
