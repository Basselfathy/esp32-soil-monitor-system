#pragma once
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "config.h"

// ── Server instance ───────────────────────────────────
WebSocketsServer ws(81);
static bool wsReady = false;
char wsToken[17] = {0};
bool wsClientAuthed[WEBSOCKETS_SERVER_CLIENT_MAX] = {false};

// ── Broadcast to all authenticated clients ────────────
void wsBroadcast(const String &msg)
{
  for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++)
    if (wsClientAuthed[i]) ws.sendTXT(i, msg.c_str());
}

// ── Build the full status JSON document ───────────────
void buildStatusDoc(DynamicJsonDocument &doc)
{
  unsigned long now = millis();
  doc["sample_interval_s"]   = (int)(sampleIntervalMs / 1000);
  doc["uptime_s"]            = now / 1000;
  doc["wifi_ssid"]           = WiFi.SSID();
  doc["wifi_rssi"]           = WiFi.RSSI();
  doc["ip"]                  = WiFi.localIP().toString();
  doc["ntp_synced"]          = ntpSynced;
  doc["free_heap"]           = lastFreeHeap ? lastFreeHeap : ESP.getFreeHeap();
  doc["spiffs_used"]         = (int)SPIFFS.usedBytes();
  doc["spiffs_total"]        = (int)SPIFFS.totalBytes();
  // Moisture + environment (sentinel → null: -127 = not connected, -1 = unavailable)
  doc["last_moisture"]       = lastMoisture;
  doc["soil_temp"]           = (lastSoilTemp > -100.0f) ? lastSoilTemp : (float)NAN;
  doc["air_temp"]            = (lastAirTemp  > -100.0f) ? lastAirTemp  : (float)NAN;
  doc["air_humid"]           = (lastAirHumid >= 0.0f)   ? lastAirHumid : (float)NAN;
  doc["light"]               = lastLight;
  doc["buffer_count"]        = bufferCount;
  // Pump
  doc["pump_on"]             = pumpOn;
  doc["auto_mode"]           = autoMode;
  doc["low_threshold"]       = lowThreshold;
  doc["cooldown_s"]          = (int)(cooldownMs / 1000);
  doc["cooldown_remaining_s"]= (now < cooldownUntil) ? (int)((cooldownUntil - now) / 1000) : 0;
  doc["pump_duration_s"]     = (int)(pumpDurationMs / 1000);
  doc["pump_remaining_s"]    = (autoMode && pumpOn && now > pumpStartedAt)
                                 ? (int)((pumpDurationMs - (now - pumpStartedAt)) / 1000) : 0;
  doc["pump_elapsed_s"]      = (pumpOn && now >= pumpStartedAt)
                                 ? (int)((now - pumpStartedAt) / 1000) : 0;
  doc["in_soak"]             = inSoak;
  doc["soak_remaining_s"]    = (inSoak && now < soakUntil)
                                 ? (int)((soakUntil - now) / 1000) : 0;
  doc["soak_s"]              = (int)(soakMs / 1000);
  doc["last_pump_run"]       = lastPumpRun;
}

// ── Push helpers ──────────────────────────────────────
void wsPushStatus()
{
  if (!wsReady) return;
  DynamicJsonDocument doc(768);
  buildStatusDoc(doc);
  doc["type"] = "status";
  String out;
  serializeJson(doc, out);
  wsBroadcast(out);
}

void wsPushLog(int idx)
{
  if (!wsReady) return;
  String m = logBuffer[idx].msg;
  m.replace("\\", "\\\\");
  m.replace("\"", "\\\"");
  String frame = "{\"type\":\"log\",\"ts\":" + String(logBuffer[idx].ts)
               + ",\"m\":\"" + m + "\"}";
  wsBroadcast(frame);
}

void wsPushReading(long t, int m, float st, float at, float ah, int lx)
{
  if (!wsReady) return;
  // Sentinel values → JSON null
  char stStr[8], atStr[8], ahStr[8];
  if (st > -100.0f) snprintf(stStr, sizeof(stStr), "%.1f", st); else strcpy(stStr, "null");
  if (at > -100.0f) snprintf(atStr, sizeof(atStr), "%.1f", at); else strcpy(atStr, "null");
  if (ah >= 0.0f)   snprintf(ahStr, sizeof(ahStr), "%.1f", ah); else strcpy(ahStr, "null");
  char frame[192];
  snprintf(frame, sizeof(frame),
           "{\"type\":\"reading\",\"t\":%ld,\"m\":%d"
           ",\"st\":%s,\"at\":%s,\"ah\":%s,\"lx\":%d}",
           t, m, stStr, atStr, ahStr, lx);
  wsBroadcast(String(frame));
}

// ── WebSocket event handler ───────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_DISCONNECTED) {
    wsClientAuthed[num] = false;
  } else if (type == WStype_CONNECTED) {
    wsClientAuthed[num] = false; // wait for auth message
  } else if (type == WStype_TEXT) {
    if (wsClientAuthed[num]) return; // already authenticated
    DynamicJsonDocument doc(128);
    DeserializationError err = deserializeJson(doc, payload, length);
    if (!err
        && strcmp(doc["type"]  | "", "auth") == 0
        && strcmp(doc["token"] | "", wsToken) == 0) {
      wsClientAuthed[num] = true;
      // Send full status
      DynamicJsonDocument statusDoc(768);
      buildStatusDoc(statusDoc);
      statusDoc["type"] = "status";
      String out;
      serializeJson(statusDoc, out);
      ws.sendTXT(num, out.c_str());
      // Replay buffered log
      int total = min(logCount, LOG_BUFFER_SIZE);
      int start = (logHead - total + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
      for (int i = 0; i < total; i++) {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        String m = logBuffer[idx].msg;
        m.replace("\\", "\\\\");
        m.replace("\"", "\\\"");
        String frame = "{\"type\":\"log\",\"ts\":" + String(logBuffer[idx].ts)
                     + ",\"m\":\"" + m + "\"}";
        ws.sendTXT(num, frame.c_str());
      }
    } else {
      ws.disconnect(num);
    }
  }
}
