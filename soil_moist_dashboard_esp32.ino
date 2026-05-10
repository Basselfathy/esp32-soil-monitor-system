#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <time.h>
#include <WebSocketsServer.h>
#include "dashboard.h"
#include "ota_page.h"
#include "secrets.h"

// ══════════════════════════════════════════════════════
//  CONFIG
// ══════════════════════════════════════════════════════
const char *WIFI_SSID1 = SECRET_WIFI_SSID1;
const char *WIFI_PASS1 = SECRET_WIFI_PASS1;
const char *WIFI_SSID2 = SECRET_WIFI_SSID2;
const char *WIFI_PASS2 = SECRET_WIFI_PASS2;
const char *AUTH_USER = SECRET_AUTH_USER;
const char *AUTH_PASS = SECRET_AUTH_PASS;
const int PIN_CAP = 34;
const int CAP_AIR = 3060;
const int CAP_WATER = 940;
const int PIN_RELAY = 26; // HIGH = relay ON (active-high)
                          // change to LOW logic below if needed

const int SAMPLE_COUNT = 5;

unsigned long sampleIntervalMs = 60000;
const unsigned long WIFI_TIMEOUT = 15000;
const unsigned long WIFI_RETRY = 30000;
const int MAX_RECORDS = 2880;   // 48 h at 1-min interval — safe with NDJSON
const int TRIM_CHUNK  = 288;    // drop oldest 10 % when full
const int MAX_BUFFER  = 10;

unsigned long pumpDurationMs = 30000; // 30 seconds default, adjustable from dashboard
unsigned long pumpStartedAt = 0;

const char *LOG_FILE = "/readings.json";
const char *TMP_FILE = "/readings.tmp";
const char *CFG_FILE = "/pump.json";
const char *SYS_FILE = "/syslog.json";

// ══════════════════════════════════════════════════════
//  GLOBALS
// ══════════════════════════════════════════════════════
WebServer server(80);

unsigned long lastSample = 0;
unsigned long lastWifiRetry = 0;
unsigned long lastSyslogSave = 0;
int lastMoisture = -1;
bool ntpSynced = false;
uint32_t lastFreeHeap = 0;

struct Reading
{
  long t;
  int m;
};
Reading offlineBuffer[MAX_BUFFER];
int bufferCount  = 0;
int recordCount  = 0;

WebSocketsServer ws(81);

// ── Log buffer ────────────────────────────────────────
#define LOG_BUFFER_SIZE 50
struct LogEntry
{
  unsigned long ts;
  char msg[120];
};
LogEntry logBuffer[LOG_BUFFER_SIZE];
int logHead = 0;  // next write position
int logCount = 0; // total entries written (for client to detect new ones)

bool _logging = false;

void log(const char *msg)
{
  Serial.println(msg);
  if (_logging)
    return;
  _logging = true;
  LogEntry &e = logBuffer[logHead];
  e.ts = millis();
  strncpy(e.msg, msg, sizeof(e.msg) - 1);
  e.msg[sizeof(e.msg) - 1] = '\0';
  // strip trailing newlines so JSON serialisation never breaks
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
  if (_logging)
    return;
  char buf[120];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  log(buf);
}

void saveSyslog()
{
  DynamicJsonDocument doc(12288);
  JsonArray arr = doc.to<JsonArray>();
  int total = min(logCount, LOG_BUFFER_SIZE);
  int start = (logHead - total + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
  for (int i = 0; i < total; i++)
  {
    int idx = (start + i) % LOG_BUFFER_SIZE;
    JsonObject entry = arr.createNestedObject();
    entry["ts"] = logBuffer[idx].ts;
    entry["m"] = logBuffer[idx].msg;
  }
  File f = SPIFFS.open(SYS_FILE, "w");
  if (f)
  {
    serializeJson(doc, f);
    f.close();
  }
}

void loadSyslog()
{
  File f = SPIFFS.open(SYS_FILE, "r");
  if (!f)
    return;
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err || !doc.is<JsonArray>())
    return;
  for (JsonObject obj : doc.as<JsonArray>())
  {
    LogEntry &e = logBuffer[logHead];
    e.ts = obj["ts"] | 0UL;
    const char *m = obj["m"] | "";
    strncpy(e.msg, m, sizeof(e.msg) - 1);
    e.msg[sizeof(e.msg) - 1] = '\0';
    logHead = (logHead + 1) % LOG_BUFFER_SIZE;
    logCount++;
  }
}

unsigned long soakMs = 300000;
unsigned long soakUntil = 0;
bool inSoak = false;

// ── Pump state ────────────────────────────────────────
bool pumpOn = false;
bool autoMode = true;
int lowThreshold = 30;             // start pump below this
unsigned long cooldownMs = 300000; // 5 min default cooldown
unsigned long cooldownUntil = 0;
long lastPumpRun = 0;

// ══════════════════════════════════════════════════════
// OTA SETUP
// ══════════════════════════════════════════════════════
void setupOTA()
{
  ArduinoOTA.setHostname("soil-monitor");
  ArduinoOTA.setPassword(AUTH_PASS);

  ArduinoOTA.onStart([]()
                     {
    esp_task_wdt_delete(NULL);
    const char *type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    logf("OTA start — updating %s\n", type);
    // Safety — turn pump off before update
    setPump(false); });

  ArduinoOTA.onEnd([]()
                   { log("OTA complete — rebooting"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { logf("OTA progress: %u%%\n", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    logf("OTA error [%u]: ", error);
    if      (error == OTA_AUTH_ERROR)    log("auth failed");
    else if (error == OTA_BEGIN_ERROR)   log("begin failed");
    else if (error == OTA_CONNECT_ERROR) log("connect failed");
    else if (error == OTA_RECEIVE_ERROR) log("receive failed");
    else if (error == OTA_END_ERROR)     log("end failed"); });

  ArduinoOTA.begin();
  log("OTA ready");
}
void setupWebOTA()
{
  server.on("/update", HTTP_GET, []()
            {
    if (!isAuthenticated()) return;
    server.send(200, "text/html", UPDATE_PAGE); });
  server.on("/update", HTTP_POST, []()
            {
      if (!isAuthenticated()) return;
      server.send(Update.hasError() ? 500 : 200, "text/plain",
                  Update.hasError() ? Update.errorString() : "OK");
      delay(500);
      ESP.restart(); }, []()
            {
      if (!isAuthenticated()) return;
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        logf("Web OTA start: %s\n", upload.filename.c_str());
        setPump(false); // safety — relay off before flash write
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
        if (Update.end(true)) {
          logf("Web OTA complete: %u bytes written\n", upload.totalSize);
        } else {
          logf("Web OTA end failed: %s\n", Update.errorString());
          Update.printError(Serial);
        }
      } });
}
// ══════════════════════════════════════════════════════
//  RELAY HELPERS
// ══════════════════════════════════════════════════════
// Active-high:
//   HIGH  signal → relay coil energised → pump ON
//   LOW signal → relay coil off       → pump OFF
// If your module is active-low, swap the two digitalWrite values.

void setPump(bool on)
{
  pumpOn = on;
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  if (on && time(nullptr) > 100000)
    lastPumpRun = (long)time(nullptr);
  logf("Pump %s\n", on ? "ON" : "OFF");
  wsPushStatus();
}

// ══════════════════════════════════════════════════════
//  PUMP CONFIG PERSISTENCE
// ══════════════════════════════════════════════════════
void savePumpConfig()
{
  DynamicJsonDocument doc(256);
  doc["sample_interval"] = (int)(sampleIntervalMs / 1000);
  doc["low"] = lowThreshold;
  doc["cooldown"] = (int)(cooldownMs / 1000);
  doc["duration"] = (int)(pumpDurationMs / 1000);
  doc["soak"] = (int)(soakMs / 1000);
  doc["auto"] = autoMode;
  doc["last_pump_run"] = lastPumpRun;
  File f = SPIFFS.open(CFG_FILE, "w");
  if (f)
  {
    serializeJson(doc, f);
    f.close();
  }
}

void loadPumpConfig()
{
  File f = SPIFFS.open(CFG_FILE, "r");

  if (!f)
    return;
  DynamicJsonDocument doc(256);
  if (!deserializeJson(doc, f))
  {
    sampleIntervalMs = (long)(doc["sample_interval"] | 60) * 1000;
    lowThreshold = doc["low"] | 30;
    cooldownMs = (long)(doc["cooldown"] | 300) * 1000;
    pumpDurationMs = (long)(doc["duration"] | 30) * 1000;
    soakMs = (long)(doc["soak"] | 300) * 1000;
    autoMode = doc["auto"] | true;
    lastPumpRun = doc["last_pump_run"] | 0L;
  }
  f.close();
}

// ══════════════════════════════════════════════════════
//  WIFI
// ══════════════════════════════════════════════════════
bool connectToNetwork(const char *ssid, const char *pass)
{
  logf("Trying %s ", ssid);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - start > WIFI_TIMEOUT)
    {
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
  if (connectToNetwork(WIFI_SSID1, WIFI_PASS1))
    return true;
  if (connectToNetwork(WIFI_SSID2, WIFI_PASS2))
    return true;
  log("Both networks failed");
  return false;
}

void ensureWifi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;
  unsigned long now = millis();
  if (now - lastWifiRetry < WIFI_RETRY)
    return;
  lastWifiRetry = now; // always update so retry spacing is respected
  log("WiFi lost — reconnecting...");
  WiFi.disconnect(true);
  delay(100);
  if (connectWifi() && !ntpSynced)
  {
    syncNTP(); // immediately attempt NTP if it was never synced
  }
}

// ══════════════════════════════════════════════════════
//  NTP
// ══════════════════════════════════════════════════════
unsigned long lastNtpAttempt = 0;
const unsigned long NTP_RETRY = 30000; // retry NTP every 30s if not synced

void syncNTP()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  unsigned long now = millis();
  if (now - lastNtpAttempt < NTP_RETRY)
    return;
  lastNtpAttempt = now;

  configTime(10800, 0, "pool.ntp.org");
  Serial.print("Waiting for NTP");
  struct tm timeinfo;
  unsigned long start = millis();
  while (!getLocalTime(&timeinfo))
  {
    if (millis() - start > 10000)
    {
      log(" timed out — will retry");
      return;
    }
    delay(500);
    Serial.print(".");
  }
  log(" synced");
  ntpSynced = true;
}

// ══════════════════════════════════════════════════════
//  SENSOR
// ══════════════════════════════════════════════════════
int readMoisture()
{
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++)
  {
    sum += analogRead(PIN_CAP);
    delay(10);
  }
  return constrain(map(sum / SAMPLE_COUNT, CAP_AIR, CAP_WATER, 0, 100), 0, 100);
}

// ══════════════════════════════════════════════════════
//  STORAGE
// ══════════════════════════════════════════════════════

void countRecordsFromFile()
{
  recordCount = 0;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f) return;

  int first = f.read();
  if (first == '[') {
    f.close();
    SPIFFS.remove(LOG_FILE);
    log("Old JSON format cleared — starting fresh NDJSON log");
    return;
  }

  while (f.available())
    if (f.read() == '\n') recordCount++;
  f.close();
  logf("Log: %d records loaded\n", recordCount);
}

void flushReading(long t, int m)
{
  // ── Bulk-trim oldest records when file is full ──────────────────────
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
    if (src) src.close();
    if (dst) dst.close();
    SPIFFS.remove(LOG_FILE);
    SPIFFS.rename(TMP_FILE, LOG_FILE);
    logf("Trimmed log to %d records\n", recordCount);
  }

  // ── Append new record as a single NDJSON line ───────────────────────
  File f = SPIFFS.open(LOG_FILE, "a");
  if (!f) { log("Log append failed"); return; }
  char line[32];
  snprintf(line, sizeof(line), "{\"t\":%ld,\"m\":%d}\n", t, m);
  f.print(line);
  f.close();
  recordCount++;
  lastFreeHeap = ESP.getFreeHeap();
  wsPushReading(t, m);
  logf("Saved %d records (heap free: %u B)\n", recordCount, lastFreeHeap);
}

void appendReading(int moisture)
{
  if (!ntpSynced || time(nullptr) < 100000)
  {
    log("Skipping — NTP not ready");
    return;
  }
  flushReading((long)time(nullptr), moisture);
}

void flushOfflineBuffer()
{
  if (!bufferCount)
    return;
  logf("Flushing %d buffered readings\n", bufferCount);
  for (int i = 0; i < bufferCount; i++)
    flushReading(offlineBuffer[i].t, offlineBuffer[i].m);
  bufferCount = 0;
}

void bufferReading(int moisture)
{
  if (!ntpSynced || time(nullptr) < 100000)
    return;
  if (bufferCount >= MAX_BUFFER)
  {
    for (int i = 1; i < MAX_BUFFER; i++)
      offlineBuffer[i - 1] = offlineBuffer[i];
    bufferCount = MAX_BUFFER - 1;
  }
  offlineBuffer[bufferCount++] = {(long)time(nullptr), moisture};
}

// ══════════════════════════════════════════════════════
//  PUMP LOGIC
// ══════════════════════════════════════════════════════
void runPumpLogic(int moisture)
{
  if (!autoMode)
    return;

  unsigned long now = millis();

  // ── Phase 1: pump is running — check stop timer ──
  if (pumpOn)
  {
    if (now - pumpStartedAt >= pumpDurationMs)
    {
      inSoak = true;
      soakUntil = now + soakMs;
      setPump(false); // wsPushStatus() called inside, inSoak already set
      logf("Pump OFF — soak started (%lus)\n", soakMs / 1000);
    }
    return;
  }

  // ── Phase 2: soak period — check soak timer ──────
  if (inSoak)
  {
    if (now < soakUntil)
      return;
    inSoak = false;
    cooldownUntil = now + cooldownMs;
    logf("Soak complete — cooldown started (%lus)\n", cooldownMs / 1000);
    wsPushStatus();
    return;
  }

  // ── Phase 3: cooldown — check cooldown timer ─────
  if (now < cooldownUntil)
    return;
  if (cooldownUntil > 0)
  {
    // Cooldown just expired — force an immediate sensor reading next loop
    cooldownUntil = 0;
    lastSample = 0;
    wsPushStatus();
    return;
  }

  // ── Phase 4: start decision — needs real moisture ─
  if (moisture == -1)
    return;

  if (moisture < lowThreshold)
  {
    pumpStartedAt = now;
    setPump(true);
    logf("Pump ON — %d%% below %d%%, runs %lus\n",
         moisture, lowThreshold, pumpDurationMs / 1000);
  }
}

// ══════════════════════════════════════════════════════
//  WEBSOCKET
// ══════════════════════════════════════════════════════
static bool wsReady = false;
char wsToken[17] = {0};
bool wsClientAuthed[WEBSOCKETS_SERVER_CLIENT_MAX] = {false};

void wsBroadcast(const String &msg) {
  for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
    if (wsClientAuthed[i]) ws.sendTXT(i, msg.c_str());
  }
}

void buildStatusDoc(DynamicJsonDocument &doc)
{
  doc["sample_interval_s"] = (int)(sampleIntervalMs / 1000);
  doc["uptime_s"] = millis() / 1000;
  doc["wifi_ssid"] = WiFi.SSID();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["ip"] = WiFi.localIP().toString();
  doc["ntp_synced"] = ntpSynced;
  doc["free_heap"] = lastFreeHeap ? lastFreeHeap : ESP.getFreeHeap();
  doc["spiffs_used"] = (int)SPIFFS.usedBytes();
  doc["spiffs_total"] = (int)SPIFFS.totalBytes();
  doc["last_moisture"] = lastMoisture;
  doc["buffer_count"] = bufferCount;
  doc["pump_on"] = pumpOn;
  doc["auto_mode"] = autoMode;
  doc["low_threshold"] = lowThreshold;
  doc["cooldown_s"] = (int)(cooldownMs / 1000);
  doc["cooldown_remaining_s"] = (millis() < cooldownUntil)
                                    ? (int)((cooldownUntil - millis()) / 1000)
                                    : 0;
  doc["pump_duration_s"] = (int)(pumpDurationMs / 1000);
  doc["pump_remaining_s"] = (autoMode && pumpOn && millis() > pumpStartedAt)
                                ? (int)((pumpDurationMs - (millis() - pumpStartedAt)) / 1000)
                                : 0;
  doc["pump_elapsed_s"] = (pumpOn && millis() >= pumpStartedAt)
                              ? (int)((millis() - pumpStartedAt) / 1000)
                              : 0;
  doc["in_soak"] = inSoak;
  doc["soak_remaining_s"] = (inSoak && millis() < soakUntil)
                                ? (int)((soakUntil - millis()) / 1000)
                                : 0;
  doc["soak_s"] = (int)(soakMs / 1000);
  doc["last_pump_run"] = lastPumpRun;
}

void wsPushStatus()
{
  if (!wsReady)
    return;
  DynamicJsonDocument doc(640);
  buildStatusDoc(doc);
  doc["type"] = "status";
  String out;
  serializeJson(doc, out);
  wsBroadcast(out);
}

void wsPushLog(int idx)
{
  if (!wsReady)
    return;
  String m = logBuffer[idx].msg;
  m.replace("\\", "\\\\");
  m.replace("\"", "\\\"");
  String frame = "{\"type\":\"log\",\"ts\":" + String(logBuffer[idx].ts) + ",\"m\":\"" + m + "\"}";
  wsBroadcast(frame);
}

void wsPushReading(long t, int m)
{
  if (!wsReady)
    return;
  String frame = "{\"type\":\"reading\",\"t\":" + String(t) + ",\"m\":" + String(m) + "}";
  wsBroadcast(frame);
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_DISCONNECTED) {
    wsClientAuthed[num] = false;
  } else if (type == WStype_CONNECTED) {
    wsClientAuthed[num] = false;
    // Wait for auth message before sending any data
  } else if (type == WStype_TEXT) {
    if (!wsClientAuthed[num]) {
      DynamicJsonDocument doc(128);
      DeserializationError err = deserializeJson(doc, payload, length);
      if (!err
          && strcmp(doc["type"] | "", "auth") == 0
          && strcmp(doc["token"] | "", wsToken) == 0) {
        wsClientAuthed[num] = true;
        // Send status immediately after auth
        DynamicJsonDocument statusDoc(640);
        buildStatusDoc(statusDoc);
        statusDoc["type"] = "status";
        String out;
        serializeJson(statusDoc, out);
        ws.sendTXT(num, out.c_str());
        // Replay buffered log entries so serial monitor populates
        int total = min(logCount, LOG_BUFFER_SIZE);
        int start = (logHead - total + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
        for (int i = 0; i < total; i++) {
          int idx = (start + i) % LOG_BUFFER_SIZE;
          String m = logBuffer[idx].msg;
          m.replace("\\", "\\\\");
          m.replace("\"", "\\\"");
          String frame = "{\"type\":\"log\",\"ts\":" + String(logBuffer[idx].ts) + ",\"m\":\"" + m + "\"}";
          ws.sendTXT(num, frame.c_str());
        }
      } else {
        ws.disconnect(num);
      }
    }
  }
}

void handleWsToken()
{
  if (!isAuthenticated()) return;
  server.send(200, "text/plain", wsToken);
}

// ══════════════════════════════════════════════════════
//  HTTP HANDLERS
// ══════════════════════════════════════════════════════
void handleRoot()
{
  if (!isAuthenticated())
    return;
  server.send_P(200, "text/html", DASHBOARD);
}

void handleLogs()
{
  if (!isAuthenticated())
    return;


  int since = 0;
  if (server.hasArg("since"))
    since = server.arg("since").toInt();

  String json = "{\"count\":" + String(logCount) + ",\"entries\":[";
  bool first = true;

  if (logCount > 0)
  {

    int total = min(logCount, LOG_BUFFER_SIZE);
    int newOnes = logCount - since;
    int toSend = min(newOnes, total);

    if (toSend > 0)
    {
  
      int start = (logHead - toSend + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
      for (int i = 0; i < toSend; i++)
      {
        int idx = (start + i) % LOG_BUFFER_SIZE;
        if (!first)
          json += ",";
        json += "{\"ts\":" + String(logBuffer[idx].ts);
        json += ",\"m\":\"";

        String m = logBuffer[idx].msg;
        m.replace("\"", "\\\"");
        json += m + "\"}";
        first = false;
      }
    }
  }

  json += "]}";
  server.send(200, "application/json", json);
}

void handleData()
{
  if (!isAuthenticated()) return;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f) {
    server.send(200, "application/json", "[]");
    return;
  }

  String resp;
  resp.reserve(f.size() + 4);
  resp = "[";
  bool first = true;
  String line;
  line.reserve(32);
  while (f.available()) {
    char c = (char)f.read();
    if (c == '\n') {
      if (line.length() > 2) {
        if (!first) resp += ',';
        resp += line;
        first = false;
      }
      line = "";
    } else {
      line += c;
    }
  }
  if (line.length() > 2) {
    if (!first) resp += ',';
    resp += line;
  }
  resp += "]";
  f.close();
  server.send(200, "application/json", resp);
}

void handleRead()
{
  if (!isAuthenticated())
    return;
  int m = readMoisture();
  lastMoisture = m;
  appendReading(m);
  server.send(200, "application/json",
              "{\"ok\":true,\"moisture\":" + String(m) + "}");
}

void handleClear()
{
  if (!isAuthenticated()) return;
  SPIFFS.remove(LOG_FILE);
  lastMoisture = -1;
  recordCount  = 0;
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleStatus()
{
  if (!isAuthenticated())
    return;
  DynamicJsonDocument doc(640);
  buildStatusDoc(doc);
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handlePumpOn()
{
  if (!isAuthenticated())
    return;

  inSoak = false;
  soakUntil = 0;
  cooldownUntil = 0;
  pumpStartedAt = millis();
  setPump(true);
  server.send(200, "application/json", "{\"ok\":true,\"pump\":\"on\"}");
}

void handlePumpOff()
{
  if (!isAuthenticated())
    return;
  setPump(false);
  inSoak = false;
  soakUntil = 0;

  if (autoMode)
    cooldownUntil = millis() + cooldownMs;
  server.send(200, "application/json", "{\"ok\":true,\"pump\":\"off\"}");
}

void handlePumpAuto()
{
  if (!isAuthenticated())
    return;
  autoMode = !autoMode;
  if (!autoMode)
    setPump(false); // safety — turn off when leaving auto
  savePumpConfig();
  wsPushStatus();
  server.send(200, "application/json",
              "{\"ok\":true,\"auto\":" + String(autoMode ? "true" : "false") + "}");
}

void handlePumpConfig()
{
  if (!isAuthenticated())
    return;
  bool changed = false;
  if (server.hasArg("low"))
  {
    int v = server.arg("low").toInt();
    if (v >= 0 && v <= 100)
    {
      lowThreshold = v;
      changed = true;
    }
  }
  if (server.hasArg("cooldown"))
  {
    int v = server.arg("cooldown").toInt();
    if (v >= 30)
    {
      cooldownMs = (unsigned long)v * 1000;
      changed = true;
    }
  }
  if (server.hasArg("duration"))
  {
    int v = server.arg("duration").toInt();
    if (v >= 5 && v <= 3600)
    {
      pumpDurationMs = (unsigned long)v * 1000;
      changed = true;
    }
  }
  if (server.hasArg("soak"))
  {
    int v = server.arg("soak").toInt();
    if (v >= 30 && v <= 7200)
    {
      soakMs = (unsigned long)v * 1000;
      changed = true;
    }
  }
  if (server.hasArg("sample_interval"))
  {
    int v = server.arg("sample_interval").toInt();
    if (v >= 10 && v <= 3600)
    {
      sampleIntervalMs = (unsigned long)v * 1000;
      changed = true;
    }
  }
  if (changed)
  {
    savePumpConfig();
    wsPushStatus();
  }
  server.send(200, "application/json",
              "{\"ok\":true,\"low\":" + String(lowThreshold) + ",\"cooldown\":" + String(cooldownMs / 1000) + ",\"duration\":" + String(pumpDurationMs / 1000) + "}");
}

void handleDebug()
{
  if (!isAuthenticated())
    return;
  File f = SPIFFS.open(LOG_FILE, "r");
  if (!f)
  {
    server.send(200, "text/plain", "File not found");
    return;
  }
  String out = "Size: " + String(f.size()) + " bytes\n\n" + f.readString();
  f.close();
  server.send(200, "text/plain", out);
}

bool isAuthenticated()
{
  if (!server.authenticate(AUTH_USER, AUTH_PASS))
  {
    server.requestAuthentication();
    return false;
  }
  return true;
}
// ══════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════
void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Relay pin — set LOW first to keep pump LOW on boot
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 30000,
      .idle_core_mask = (1 << 0),
      .trigger_panic = true};
  // init first; if already initialised by the framework, reconfigure instead
  if (esp_task_wdt_init(&wdt_config) == ESP_ERR_INVALID_STATE)
    esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  if (!SPIFFS.begin(true))
    log("SPIFFS mount failed");
  countRecordsFromFile();

  loadSyslog();
  loadPumpConfig();
  connectWifi();
  syncNTP();
  setupOTA();
  setupWebOTA();

  // Generate WebSocket auth token (rotates every boot)
  snprintf(wsToken, sizeof(wsToken), "%08x%08x",
           (unsigned int)esp_random(), (unsigned int)esp_random());

  server.on("/", handleRoot);
  server.on("/wstoken", handleWsToken);
  server.on("/logs", handleLogs);
  server.on("/data", handleData);
  server.on("/read", handleRead);
  server.on("/clear", handleClear);
  server.on("/status", handleStatus);
  server.on("/debug", handleDebug);
  server.on("/pump/on", handlePumpOn);
  server.on("/pump/off", handlePumpOff);
  server.on("/pump/auto", handlePumpAuto);
  server.on("/pump/config", handlePumpConfig);
  server.begin();
  ws.begin();
  ws.onEvent(onWsEvent);
  wsReady = true;
  log("Server started");
  log("WebSocket on port 81");
}

// ══════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════
void loop()
{
  esp_task_wdt_reset();
  ArduinoOTA.handle();
  server.handleClient();
  ws.loop();
  ensureWifi();

  // Retry NTP if not synced yet and WiFi is up
  if (!ntpSynced && WiFi.status() == WL_CONNECTED)
  {
    syncNTP();
  }

  if (WiFi.status() == WL_CONNECTED && bufferCount > 0)
    flushOfflineBuffer();

  // ── Persist syslog to SPIFFS every 5 minutes ──
  if (millis() - lastSyslogSave >= 300000UL)
  {
    lastSyslogSave = millis();
    saveSyslog();
  }

  // ── Pump timing — runs every loop, not tied to sample interval ──
  runPumpLogic(-1); // -1 means "timer check only, no moisture decision"

  // ── Push status every second during active states ──
  static unsigned long lastWsPush = 0;
  if ((pumpOn || inSoak || millis() < cooldownUntil) && millis() - lastWsPush >= 1000)
  {
    lastWsPush = millis();
    wsPushStatus();
  }

  // ── Sensor sampling — every 60s ──
  if (millis() - lastSample >= sampleIntervalMs)
  {
    lastSample = millis();
    int m = readMoisture();
    lastMoisture = m;
    if (WiFi.status() == WL_CONNECTED)
      appendReading(m);
    else
      bufferReading(m);
    runPumpLogic(m); // real moisture value — allow start decision
  }
}