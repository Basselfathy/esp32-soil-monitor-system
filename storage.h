#pragma once
#include <SPIFFS.h>
#include "config.h"

// Forward declarations — defined in websocket_srv.h and connectivity.h
void wsPushReading(long t, int m, float st, float at, float ah, int lx);
extern bool ntpSynced;

// ── Offline buffer (used when WiFi is down) ───────────
struct Reading {
  long  t;
  int   m;
  float st; // soil temp  °C
  float at; // air temp   °C
  float ah; // air humid  %
  int   lx; // light      0–100 %
};
Reading offlineBuffer[MAX_BUFFER];
int bufferCount = 0;

// ── NDJSON record count (maintained in RAM) ───────────
int      recordCount = 0;
uint32_t lastFreeHeap = 0; // sampled after each SPIFFS write (quiet moment)

// ── Boot: count records + auto-migrate old format ─────
void countRecordsFromFile()
{
  recordCount = 0;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f) return;
  int first = f.read();
  if (first == '[') {
    SPIFFS.remove(LOG_FILE);
    log("Old JSON format cleared — starting fresh NDJSON log");
    return;
  }
  while (f.available())
    if (f.read() == '\n') recordCount++;
  logf("Log: %d records loaded\n", recordCount);
}

// ── Append one record (trim if full) ──────────────────
void flushReading(long t, int m, float st, float at, float ah, int lx)
{
  // Bulk-trim oldest TRIM_CHUNK records when log is full
  if (recordCount >= MAX_RECORDS) {
    File src = SPIFFS.open(LOG_FILE, "r");
    File dst = SPIFFS.open(TMP_FILE, "w");
    if (src && dst) {
      int skipped = 0;
      while (src.available() && skipped < TRIM_CHUNK)
        if (src.read() == '\n') skipped++;
      uint8_t buf[128];
      size_t n;
      while ((n = src.read(buf, sizeof(buf))) > 0)
        dst.write(buf, n);
      recordCount -= skipped;
    }

    SPIFFS.remove(LOG_FILE);
    SPIFFS.rename(TMP_FILE, LOG_FILE);
    logf("Trimmed log to %d records\n", recordCount);
  }

  // Append as NDJSON line — sentinel values become JSON null
  File f = SPIFFS.open(LOG_FILE, "a");
  if (!f) { log("Log append failed"); return; }
  char stStr[8], atStr[8], ahStr[8];
  if (st > -100.0f) snprintf(stStr, sizeof(stStr), "%.1f", st); else strcpy(stStr, "null");
  if (at > -100.0f) snprintf(atStr, sizeof(atStr), "%.1f", at); else strcpy(atStr, "null");
  if (ah >= 0.0f)   snprintf(ahStr, sizeof(ahStr), "%.1f", ah); else strcpy(ahStr, "null");
  char line[96];
  snprintf(line, sizeof(line),
           "{\"t\":%ld,\"m\":%d,\"st\":%s,\"at\":%s,\"ah\":%s,\"lx\":%d}\n",
           t, m, stStr, atStr, ahStr, lx);
  f.print(line);

  recordCount++;
  lastFreeHeap = ESP.getFreeHeap(); // quiet moment — no large allocs alive
  wsPushReading(t, m, st, at, ah, lx);
  logf("Saved %d records (heap free: %u B)\n", recordCount, lastFreeHeap);
}

// ── Append a reading using current sensor globals ─────
void appendReading()
{
  if (!ntpSynced || time(nullptr) < 100000) {
    log("Skipping — NTP not ready");
    return;
  }
  flushReading((long)time(nullptr),
               lastMoisture, lastSoilTemp, lastAirTemp, lastAirHumid, lastLight);
}

// ── Flush the offline buffer after WiFi reconnects ────
void flushOfflineBuffer()
{
  if (!bufferCount) return;
  logf("Flushing %d buffered readings\n", bufferCount);
  for (int i = 0; i < bufferCount; i++) {
    Reading &r = offlineBuffer[i];
    flushReading(r.t, r.m, r.st, r.at, r.ah, r.lx);
  }
  bufferCount = 0;
}

// ── Buffer a reading when WiFi / NTP is unavailable ───
void bufferReading()
{
  if (!ntpSynced || time(nullptr) < 100000) return;
  if (bufferCount >= MAX_BUFFER) {
    for (int i = 1; i < MAX_BUFFER; i++)
      offlineBuffer[i - 1] = offlineBuffer[i];
    bufferCount = MAX_BUFFER - 1;
  }
  offlineBuffer[bufferCount++] = {
    (long)time(nullptr),
    lastMoisture, lastSoilTemp, lastAirTemp, lastAirHumid, lastLight
  };
}
