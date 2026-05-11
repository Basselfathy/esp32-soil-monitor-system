// ══════════════════════════════════════════════════════
//  Soil-moisture monitor — ESP32
//  All logic lives in the .h modules below.
// ══════════════════════════════════════════════════════
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <time.h>
#include <WebSocketsServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_AM2320.h>

#include "config.h"
#include "logger.h"
#include "calibration.h"
#include "sensors.h"
#include "storage.h"
#include "pump.h"
#include "connectivity.h"
#include "websocket_srv.h"
#include "http_handlers.h"
// dashboard.h and ota_page.h are included inside http_handlers.h

// ══════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════
void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Relay pin — drive LOW immediately to keep pump off on boot
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  // Watchdog: 30 s, panic on expire
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms    = 30000,
      .idle_core_mask = (1 << 0),
      .trigger_panic  = true};
  if (esp_task_wdt_init(&wdt_config) == ESP_ERR_INVALID_STATE)
    esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  if (!SPIFFS.begin(true))
    log("SPIFFS mount failed");

  countRecordsFromFile();
  loadSyslog();
  loadCalib();
  loadPumpConfig();
  setupSensors();
  connectWifi();
  syncNTP();
  setupOTA();
  setupWebOTA();

  // WebSocket auth token — rotates every boot
  snprintf(wsToken, sizeof(wsToken), "%08x%08x",
           (unsigned int)esp_random(), (unsigned int)esp_random());

  setupRoutes();
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

  // Retry NTP if not yet synced
  if (!ntpSynced && WiFi.status() == WL_CONNECTED)
    syncNTP();

  // Flush offline buffer once WiFi is back
  if (WiFi.status() == WL_CONNECTED && bufferCount > 0)
    flushOfflineBuffer();

  // Persist syslog to SPIFFS every 5 minutes
  static unsigned long lastSyslogSave = 0;
  if (millis() - lastSyslogSave >= 300000UL) {
    lastSyslogSave = millis();
    saveSyslog();
  }

  // Pump timer check (not tied to sample interval)
  runPumpLogic(-1);

  // Push status every second during active pump / soak / cooldown
  static unsigned long lastWsPush = 0;
  if ((pumpOn || inSoak || millis() < cooldownUntil) && millis() - lastWsPush >= 1000) {
    lastWsPush = millis();
    wsPushStatus();
  }

  // Sensor sampling — once per sampleIntervalMs
  if (millis() - lastSample >= sampleIntervalMs) {
    lastSample = millis();
    readAllSensors();
    if (WiFi.status() == WL_CONNECTED)
      appendReading();
    else
      bufferReading();
    runPumpLogic(lastMoisture);
  }
}

