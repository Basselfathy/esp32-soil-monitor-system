#pragma once
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <SPIFFS.h>
#include "config.h"
#include "dashboard.h"
#include "ota_page.h"

// ── HTTP server instance ──────────────────────────────
WebServer server(80);

// ── Auth guard ────────────────────────────────────────
bool isAuthenticated()
{
  if (!server.authenticate(AUTH_USER, AUTH_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// ── Dashboard ─────────────────────────────────────────
void handleRoot()
{
  if (!isAuthenticated()) return;
  server.send_P(200, "text/html", DASHBOARD);
}

// ── WS auth token ─────────────────────────────────────
void handleWsToken()
{
  if (!isAuthenticated()) return;
  server.send(200, "text/plain", wsToken);
}

// ── Log buffer (HTTP polling fallback) ────────────────
void handleLogs()
{
  if (!isAuthenticated()) return;
  int since = server.hasArg("since") ? server.arg("since").toInt() : 0;
  String json = "{\"count\":" + String(logCount) + ",\"entries\":[";
  bool first = true;
  int total  = min(logCount, LOG_BUFFER_SIZE);
  int toSend = min(logCount - since, total);
  if (toSend > 0) {
    int start = (logHead - toSend + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
    for (int i = 0; i < toSend; i++) {
      int idx = (start + i) % LOG_BUFFER_SIZE;
      if (!first) json += ",";
      String m = logBuffer[idx].msg;
      m.replace("\"", "\\\"");
      json += "{\"ts\":" + String(logBuffer[idx].ts) + ",\"m\":\"" + m + "\"}";
      first = false;
    }
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ── Historical data — streamed, no heap String ───────
// ?last=N  returns the most-recent N records (default 1440 = 24 h at 1 min)
void handleData()
{
  if (!isAuthenticated()) return;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f) { server.send(200, "application/json", "[]"); return; }

  int limit = server.hasArg("last") ? server.arg("last").toInt() : 1440;
  if (limit <= 0 || limit > recordCount) limit = recordCount;
  int skip = recordCount - limit;

  // ── Phase 1: fast-forward past 'skip' newlines using block reads ──
  if (skip > 0) {
    uint8_t rb[256];
    int    skipped = 0;
    size_t bytePos = 0;
    bool   done    = false;
    while (f.available() && !done) {
      size_t n = f.read(rb, sizeof(rb));
      for (size_t i = 0; i < n; i++) {
        bytePos++;
        if (rb[i] == '\n' && ++skipped == skip) {
          f.seek(bytePos); // position right after the skip-th newline
          done = true;
          break;
        }
      }
    }
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("[");

  // ── Phase 2: accumulate records into a send buffer, flush in ~1 KB chunks ──
  // static keeps them off the stack; safe because handleClient is single-threaded.
  static char outBuf[1024];
  static char lineBuf[96];
  int  outPos  = 0;
  int  linePos = 0;
  bool first   = true;

  auto flushOut = [&]() {
    if (outPos > 0) {
      outBuf[outPos] = '\0';
      server.sendContent(outBuf);
      outPos = 0;
    }
  };

  auto emitLine = [&]() {
    if (linePos == 0) return;
    lineBuf[linePos] = '\0';
    int len  = linePos;
    int need = (first ? 0 : 1) + len; // optional comma + record bytes
    if (outPos + need >= (int)sizeof(outBuf) - 1) flushOut();
    if (!first) outBuf[outPos++] = ',';
    memcpy(outBuf + outPos, lineBuf, len);
    outPos  += len;
    linePos  = 0;
    first    = false;
  };

  uint8_t rb[256];
  while (f.available()) {
    size_t n = f.read(rb, sizeof(rb));
    for (size_t i = 0; i < n; i++) {
      char c = (char)rb[i];
      if (c == '\n') emitLine();
      else if (linePos < (int)sizeof(lineBuf) - 1) lineBuf[linePos++] = c;
    }
  }
  emitLine(); // handle last line without trailing newline
  flushOut();

  server.sendContent("]");
  server.sendContent(""); // end chunked transfer
}

// ── Force immediate sensor read + save ───────────────
void handleRead()
{
  if (!isAuthenticated()) return;
  readAllSensors();
  appendReading();
  char resp[128];
  snprintf(resp, sizeof(resp),
           "{\"ok\":true,\"moisture\":%d,\"soil_temp\":%.1f"
           ",\"air_temp\":%.1f,\"humidity\":%.1f,\"light\":%d}",
           lastMoisture, lastSoilTemp, lastAirTemp, lastAirHumid, lastLight);
  server.send(200, "application/json", resp);
}

// ── Clear log ─────────────────────────────────────────
void handleClear()
{
  if (!isAuthenticated()) return;
  SPIFFS.remove(LOG_FILE);
  lastMoisture = -1;
  recordCount  = 0;
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── Status JSON ───────────────────────────────────────
void handleStatus()
{
  if (!isAuthenticated()) return;
  DynamicJsonDocument doc(768);
  buildStatusDoc(doc);
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// ── Raw log file (debug) — streamed in 256-byte chunks ─
void handleDebug()
{
  if (!isAuthenticated()) return;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f) { server.send(200, "text/plain", "File not found"); return; }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/plain", "");

  // Header line — no heap String of the whole file
  char hdr[64];
  snprintf(hdr, sizeof(hdr), "Size: %u bytes, records: %d\n\n",
           (unsigned)f.size(), recordCount);
  server.sendContent(hdr);

  uint8_t buf[256];
  size_t  n;
  while ((n = f.read(buf, sizeof(buf))) > 0) {
    buf[n] = '\0';
    server.sendContent(reinterpret_cast<const char *>(buf));
  }
  f.close();
  server.sendContent(""); // end chunked transfer
}

// ── Pump controls ─────────────────────────────────────
void handlePumpOn()
{
  if (!isAuthenticated()) return;
  inSoak = false; soakUntil = 0; cooldownUntil = 0;
  pumpStartedAt = millis();
  setPump(true);
  server.send(200, "application/json", "{\"ok\":true,\"pump\":\"on\"}");
}

void handlePumpOff()
{
  if (!isAuthenticated()) return;
  setPump(false);
  inSoak = false; soakUntil = 0;
  if (autoMode) cooldownUntil = millis() + cooldownMs;
  server.send(200, "application/json", "{\"ok\":true,\"pump\":\"off\"}");
}

void handlePumpAuto()
{
  if (!isAuthenticated()) return;
  autoMode = !autoMode;
  if (!autoMode) setPump(false); // safety
  savePumpConfig();
  wsPushStatus();
  server.send(200, "application/json",
              "{\"ok\":true,\"auto\":" + String(autoMode ? "true" : "false") + "}");
}

void handlePumpConfig()
{
  if (!isAuthenticated()) return;
  bool changed = false;
  auto tryInt = [&](const char *key, int lo, int hi, auto &dest) {
    if (server.hasArg(key)) {
      int v = server.arg(key).toInt();
      if (v >= lo && v <= hi) { dest = v; changed = true; }
    }
  };
  tryInt("low",             0,  100, lowThreshold);
  if (server.hasArg("cooldown")) {
    int v = server.arg("cooldown").toInt();
    if (v >= 5) { cooldownMs = (unsigned long)v * 1000; changed = true; }
  }
  if (server.hasArg("duration")) {
    int v = server.arg("duration").toInt();
    if (v >= 5 && v <= 3600) { pumpDurationMs = (unsigned long)v * 1000; changed = true; }
  }
  if (server.hasArg("soak")) {
    int v = server.arg("soak").toInt();
    if (v >= 5 && v <= 7200) { soakMs = (unsigned long)v * 1000; changed = true; }
  }
  if (server.hasArg("sample_interval")) {
    int v = server.arg("sample_interval").toInt();
    if (v >= 5 && v <= 3600) { sampleIntervalMs = (unsigned long)v * 1000; changed = true; }
  }
  if (changed) { savePumpConfig(); wsPushStatus(); }
  server.send(200, "application/json",
    "{\"ok\":true,\"low\":"      + String(lowThreshold)      +
    ",\"cooldown\":"             + String(cooldownMs / 1000)  +
    ",\"duration\":"             + String(pumpDurationMs / 1000) + "}");
}

// ── Calibration — GET: current config + live raw ADC readings ───────
void handleCalibGet()
{
  if (!isAuthenticated()) return;
  long ldrSum = 0, capSum = 0;
  for (int i = 0; i < 8; i++) {
    ldrSum += analogRead(PIN_LDR);
    capSum += analogRead(PIN_CAP);
    delay(5);
  }
  char resp[256];
  snprintf(resp, sizeof(resp),
    "{\"cap_air\":%d,\"cap_water\":%d"
    ",\"ldr_dark\":%d,\"ldr_bright\":%d"
    ",\"cap_raw\":%ld,\"ldr_raw\":%ld"
    ",\"cap_pct\":%d,\"ldr_pct\":%d}",
    CAP_AIR, CAP_WATER, LDR_DARK, LDR_BRIGHT,
    capSum / 8, ldrSum / 8,
    lastMoisture, lastLight);
  server.send(200, "application/json", resp);
}

// ── Calibration — POST: update and persist calibration values ────────
void handleCalibPost()
{
  if (!isAuthenticated()) return;
  bool changed = false;
  auto tryInt = [&](const char *key, int &dest) -> bool {
    if (!server.hasArg(key)) return true;   // not sent — skip
    int v = server.arg(key).toInt();
    if (v < 0 || v > 4095) return false;   // out of 12-bit range
    dest = v; changed = true; return true;
  };
  if (!tryInt("cap_air",    CAP_AIR)   ||
      !tryInt("cap_water",  CAP_WATER) ||
      !tryInt("ldr_dark",   LDR_DARK)  ||
      !tryInt("ldr_bright", LDR_BRIGHT)) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"value must be 0-4095\"}");
    return;
  }
  if (CAP_AIR == CAP_WATER || LDR_DARK == LDR_BRIGHT) {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"calibration range cannot be zero\"}");
    return;
  }
  if (changed) saveCalib();
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── LED config — GET: current state ───────────────────────
void handleLedGet()
{
  if (!isAuthenticated()) return;
  char resp[128];
  snprintf(resp, sizeof(resp),
    "{\"auto\":%s,\"brightness\":%d,\"manual\":%d,\"threshold\":%d}",
    ledAutoMode ? "true" : "false",
    ledBrightness, ledManualBright, ledThreshold);
  server.send(200, "application/json", resp);
}

// ── LED config — POST: ?auto=0|1  ?manual=0-100  ?threshold=0-100 ──
void handleLedConfig()
{
  if (!isAuthenticated()) return;
  bool changed = false;
  if (server.hasArg("auto")) {
    ledAutoMode = server.arg("auto").toInt() != 0;
    if (ledAutoMode) runLedLogic();  // apply immediately
    changed = true;
  }
  if (server.hasArg("manual")) {
    int v = server.arg("manual").toInt();
    if (v >= 0 && v <= 100) { ledManualBright = v; changed = true; }
    if (!ledAutoMode) setLed(ledManualBright);
  }
  if (server.hasArg("threshold")) {
    int v = server.arg("threshold").toInt();
    if (v >= 0 && v <= 100) { ledThreshold = v; changed = true; }
  }
  if (changed) { saveLedConfig(); wsPushStatus(); }
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── LED manual brightness — POST: ?brightness=0-100 ──────────
void handleLedSet()
{
  if (!isAuthenticated()) return;
  if (server.hasArg("brightness")) {
    int v = server.arg("brightness").toInt();
    if (v >= 0 && v <= 100) {
      ledAutoMode     = false;
      ledManualBright = v;
      setLed(v);
      saveLedConfig();
      wsPushStatus();
    }
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

// ── OTA (Arduino IDE / network) ───────────────────────
void setupOTA()
{
  ArduinoOTA.setHostname("soil-monitor");
  ArduinoOTA.setPassword(AUTH_PASS);
  ArduinoOTA.onStart([]() {
    esp_task_wdt_delete(NULL);
    // Kill all WebSocket activity before OTA touches the network/flash.
    // wsPushStatus() and wsPushLog() both check wsReady first, so setting
    // it false here makes every subsequent log/push call a safe no-op.
    wsReady = false;
    for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++)
      ws.disconnect(i);
    const char *type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("OTA start — updating %s\n", type);
    setPump(false);
  });
  ArduinoOTA.onEnd([]()   { Serial.println("OTA complete — rebooting"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t)
                         { Serial.printf("OTA progress: %u%%\n", p / (t / 100)); });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("OTA error [%u]: ", e);
    if      (e == OTA_AUTH_ERROR)    Serial.println("auth failed");
    else if (e == OTA_BEGIN_ERROR)   Serial.println("begin failed");
    else if (e == OTA_CONNECT_ERROR) Serial.println("connect failed");
    else if (e == OTA_RECEIVE_ERROR) Serial.println("receive failed");
    else if (e == OTA_END_ERROR)     Serial.println("end failed");
  });
  ArduinoOTA.begin();
  log("OTA ready");
}

// ── OTA (web page upload) ─────────────────────────────
void setupWebOTA()
{
  server.on("/update", HTTP_GET, []() {
    if (!isAuthenticated()) return;
    server.send(200, "text/html", UPDATE_PAGE);
  });
  server.on("/update", HTTP_POST,
    []() {
      if (!isAuthenticated()) return;
      server.send(Update.hasError() ? 500 : 200, "text/plain",
                  Update.hasError() ? Update.errorString() : "OK");
      delay(500);
      ESP.restart();
    },
    []() {
      if (!isAuthenticated()) return;
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        logf("Web OTA start: %s\n", upload.filename.c_str());
        setPump(false);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          logf("Web OTA begin failed: %s\n", Update.errorString());
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          logf("Web OTA write error: %s\n", Update.errorString());
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true))
          logf("Web OTA complete: %u bytes written\n", upload.totalSize);
        else {
          logf("Web OTA end failed: %s\n", Update.errorString());
          Update.printError(Serial);
        }
      }
    }
  );
}

// ── Register all routes ───────────────────────────────
void setupRoutes()
{
  server.on("/",            handleRoot);
  server.on("/wstoken",     handleWsToken);
  server.on("/logs",        handleLogs);
  server.on("/data",        handleData);
  server.on("/read",        handleRead);
  server.on("/clear",       handleClear);
  server.on("/status",      handleStatus);
  server.on("/debug",       handleDebug);
  server.on("/calib",       HTTP_GET,  handleCalibGet);
  server.on("/calib",       HTTP_POST, handleCalibPost);
  server.on("/pump/on",     handlePumpOn);
  server.on("/pump/off",    handlePumpOff);
  server.on("/pump/auto",   handlePumpAuto);
  server.on("/pump/config", handlePumpConfig);
  server.on("/led",         HTTP_GET,  handleLedGet);
  server.on("/led/config",  HTTP_POST, handleLedConfig);
  server.on("/led/set",     HTTP_POST, handleLedSet);
}
