#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declaration — defined in websocket_srv.h
void wsPushLog(int idx);

// ── Log ring buffer ───────────────────────────────────
#define LOG_BUFFER_SIZE 50
struct LogEntry {
  unsigned long ts;
  char msg[120];
};
LogEntry logBuffer[LOG_BUFFER_SIZE];
int logHead  = 0; // next write position (ring)
int logCount = 0; // total entries ever written

static bool _logging = false; // re-entrancy guard

void log(const char *msg)
{
  Serial.println(msg);
  if (_logging) return;
  _logging = true;

  LogEntry &e = logBuffer[logHead];
  e.ts = millis();
  strncpy(e.msg, msg, sizeof(e.msg) - 1);
  e.msg[sizeof(e.msg) - 1] = '\0';
  // Strip trailing newlines — keeps JSON serialisation clean
  int len = strlen(e.msg);
  while (len > 0 && (e.msg[len - 1] == '\n' || e.msg[len - 1] == '\r'))
    e.msg[--len] = '\0';

  int savedIdx = logHead;
  logHead = (logHead + 1) % LOG_BUFFER_SIZE;
  logCount++;
  _logging = false;
  wsPushLog(savedIdx);
}

void logf(const char *fmt, ...)
{
  if (_logging) return;
  char buf[120];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  log(buf);
}

// ── Syslog persistence ────────────────────────────────
void saveSyslog()
{
  DynamicJsonDocument doc(12288);
  JsonArray arr = doc.to<JsonArray>();
  int total = min(logCount, LOG_BUFFER_SIZE);
  int start = (logHead - total + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
  for (int i = 0; i < total; i++) {
    int idx = (start + i) % LOG_BUFFER_SIZE;
    JsonObject entry = arr.createNestedObject();
    entry["ts"] = logBuffer[idx].ts;
    entry["m"]  = logBuffer[idx].msg;
  }
  File f = SPIFFS.open(SYS_FILE, "w");
  if (f) { serializeJson(doc, f); }
}

void loadSyslog()
{
  File f = SPIFFS.open(SYS_FILE, "r");
  if (!f) return;
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, f);
  if (err || !doc.is<JsonArray>()) return;
  for (JsonObject obj : doc.as<JsonArray>()) {
    LogEntry &e = logBuffer[logHead];
    e.ts = obj["ts"] | 0UL;
    const char *m = obj["m"] | "";
    strncpy(e.msg, m, sizeof(e.msg) - 1);
    e.msg[sizeof(e.msg) - 1] = '\0';
    logHead = (logHead + 1) % LOG_BUFFER_SIZE;
    logCount++;
  }
}
