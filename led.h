#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declaration — defined in websocket_srv.h
void wsPushStatus();

// ── LED state ─────────────────────────────────────────
bool ledAutoMode     = true;  // true = follow LDR inverse
int  ledBrightness   = 0;     // current output 0–100 %
int  ledManualBright = 50;    // remembered manual setpoint
// Auto mode: LED turns on when ambient light drops below this %
// Above threshold → LED fully off; below → scales inversely.
int  ledThreshold    = 50;    // % ambient light level

// ── LEDC PWM output ───────────────────────────────────
// 12-bit resolution (0–4095), 1 kHz — smooth flicker-free dimming.
static const int LED_FREQ      = 1000;
static const int LED_RESOLUTION = 12;  // bits → 0..4095

void setLed(int pct)
{
  ledBrightness = constrain(pct, 0, 100);
  uint32_t duty = (uint32_t)((ledBrightness / 100.0f) * 4095);
  ledcWrite(PIN_LED, duty);
}

void setupLed()
{
  ledcAttach(PIN_LED, LED_FREQ, LED_RESOLUTION);
  setLed(0); // start off
  logf("LED PWM ready on pin %d", PIN_LED);
}

// ── Auto mode: invert LDR reading ─────────────────────
// When ambient light < ledThreshold %  → LED brightness scales up
// When ambient light >= ledThreshold % → LED off
void runLedLogic()
{
  if (!ledAutoMode) return;
  if (lastLight < 0) return; // LDR not yet read

  if (lastLight >= ledThreshold) {
    setLed(0);
  } else {
    // Map 0..ledThreshold → 100..0 (dark = bright LED)
    int brightness = map(lastLight, 0, ledThreshold, 100, 0);
    setLed(constrain(brightness, 0, 100));
  }
}

// ── Persist config ────────────────────────────────────
void saveLedConfig()
{
  File f = SPIFFS.open(LED_FILE, "w");
  if (!f) { log("LED config: write failed"); return; }
  StaticJsonDocument<128> doc;
  doc["auto"]      = ledAutoMode;
  doc["manual"]    = ledManualBright;
  doc["threshold"] = ledThreshold;
  serializeJson(doc, f);
  logf("LED config saved: auto=%d manual=%d%% threshold=%d%%\n",
       ledAutoMode, ledManualBright, ledThreshold);
}

void loadLedConfig()
{
  File f = SPIFFS.open(LED_FILE, "r");
  if (!f) return; // no saved file — keep defaults
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    if (doc.containsKey("auto"))      ledAutoMode     = doc["auto"].as<bool>();
    if (doc.containsKey("manual"))    ledManualBright = doc["manual"].as<int>();
    if (doc.containsKey("threshold")) ledThreshold    = doc["threshold"].as<int>();
  }
  logf("LED config loaded: auto=%d manual=%d%% threshold=%d%%\n",
       ledAutoMode, ledManualBright, ledThreshold);
}
