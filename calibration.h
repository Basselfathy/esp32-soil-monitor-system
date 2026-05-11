#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"

// ── Calibration persistence ───────────────────────────
// Loads / saves the four mutable calibration globals
// (CAP_AIR, CAP_WATER, LDR_DARK, LDR_BRIGHT) to SPIFFS
// so they survive reboots without reflashing.

const char *CALIB_FILE = "/calib.json";

void loadCalib()
{
  // File is RAII: destructor calls close() when f goes out of scope,
  // whether we return early or reach the end normally.
  File f = SPIFFS.open(CALIB_FILE, "r");
  if (!f) return;  // no saved file — keep compile-time defaults

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    if (doc.containsKey("cap_air"))    CAP_AIR    = doc["cap_air"].as<int>();
    if (doc.containsKey("cap_water"))  CAP_WATER  = doc["cap_water"].as<int>();
    if (doc.containsKey("ldr_dark"))   LDR_DARK   = doc["ldr_dark"].as<int>();
    if (doc.containsKey("ldr_bright")) LDR_BRIGHT = doc["ldr_bright"].as<int>();
  }
  logf("Calib loaded: moisture %d..%d  light %d..%d\n",
       CAP_WATER, CAP_AIR, LDR_DARK, LDR_BRIGHT);
}

void saveCalib()
{
  File f = SPIFFS.open(CALIB_FILE, "w");
  if (!f) { log("Calib: failed to open file for write"); return; }

  StaticJsonDocument<128> doc;
  doc["cap_air"]    = CAP_AIR;
  doc["cap_water"]  = CAP_WATER;
  doc["ldr_dark"]   = LDR_DARK;
  doc["ldr_bright"] = LDR_BRIGHT;
  serializeJson(doc, f);
  // destructor flushes and closes f here
  logf("Calib saved: moisture %d..%d  light %d..%d\n",
       CAP_WATER, CAP_AIR, LDR_DARK, LDR_BRIGHT);
}
