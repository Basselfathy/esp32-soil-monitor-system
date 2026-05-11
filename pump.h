#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"

// Forward declaration — defined in websocket_srv.h
void wsPushStatus();

// ── Pump & automation state ───────────────────────────
bool          pumpOn        = false;
bool          autoMode      = true;
int           lowThreshold  = 30;       // start pump below this %
unsigned long cooldownMs    = 300000;   // 5 min default
unsigned long cooldownUntil = 0;
unsigned long soakMs        = 300000;   // 5 min default
unsigned long soakUntil     = 0;
bool          inSoak        = false;
unsigned long pumpDurationMs = 30000;  // 30 s default
unsigned long pumpStartedAt  = 0;
unsigned long sampleIntervalMs = 60000; // 60 s default
unsigned long lastSample     = 0;      // used by runPumpLogic + main loop
long          lastPumpRun    = 0;      // unix timestamp of last pump ON

// ── Drive the relay ───────────────────────────────────
// Active-high: HIGH energises relay → pump ON
// Swap HIGH/LOW below if your relay module is active-low.
void setPump(bool on)
{
  pumpOn = on;
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  if (on && time(nullptr) > 100000)
    lastPumpRun = (long)time(nullptr);
  logf("Pump %s\n", on ? "ON" : "OFF");
  wsPushStatus();
}

// ── Persist config to flash ───────────────────────────
void savePumpConfig()
{
  DynamicJsonDocument doc(256);
  doc["sample_interval"] = (int)(sampleIntervalMs / 1000);
  doc["low"]             = lowThreshold;
  doc["cooldown"]        = (int)(cooldownMs / 1000);
  doc["duration"]        = (int)(pumpDurationMs / 1000);
  doc["soak"]            = (int)(soakMs / 1000);
  doc["auto"]            = autoMode;
  doc["last_pump_run"]   = lastPumpRun;
  File f = SPIFFS.open(CFG_FILE, "w");
  if (f) { serializeJson(doc, f); f.close(); }
}

void loadPumpConfig()
{
  File f = SPIFFS.open(CFG_FILE, "r");
  if (!f) return;
  DynamicJsonDocument doc(256);
  if (!deserializeJson(doc, f)) {
    sampleIntervalMs = (long)(doc["sample_interval"] | 60)  * 1000;
    lowThreshold     = doc["low"]      | 30;
    cooldownMs       = (long)(doc["cooldown"]  | 300) * 1000;
    pumpDurationMs   = (long)(doc["duration"]  |  30) * 1000;
    soakMs           = (long)(doc["soak"]      | 300) * 1000;
    autoMode         = doc["auto"]     | true;
    lastPumpRun      = doc["last_pump_run"] | 0L;
  }
  f.close();
}

// ── Automation state machine (call every loop) ────────
// Pass moisture=-1 for timer-only check; real value for start decision.
void runPumpLogic(int moisture)
{
  if (!autoMode) return;

  unsigned long now = millis();

  // Phase 1: pump running — check stop timer
  if (pumpOn) {
    if (now - pumpStartedAt >= pumpDurationMs) {
      inSoak    = true;
      soakUntil = now + soakMs;
      setPump(false); // wsPushStatus() fires inside; inSoak already set
      logf("Pump OFF — soak started (%lus)\n", soakMs / 1000);
    }
    return;
  }

  // Phase 2: soak period
  if (inSoak) {
    if (now < soakUntil) return;
    inSoak        = false;
    cooldownUntil = now + cooldownMs;
    logf("Soak complete — cooldown started (%lus)\n", cooldownMs / 1000);
    wsPushStatus();
    return;
  }

  // Phase 3: cooldown
  if (now < cooldownUntil) return;
  if (cooldownUntil > 0) {
    cooldownUntil = 0;
    lastSample    = 0; // force immediate sensor read next loop
    wsPushStatus();
    return;
  }

  // Phase 4: start decision — needs a real moisture reading
  if (moisture == -1) return;

  if (moisture < lowThreshold) {
    pumpStartedAt = now;
    setPump(true);
    logf("Pump ON — %d%% below %d%%, runs %lus\n",
         moisture, lowThreshold, pumpDurationMs / 1000);
  }
}
