#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_AM2320.h>
#include "config.h"

// ── Sensor objects ────────────────────────────────────
OneWire           _oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&_oneWire);
Adafruit_AM2320   am2320;

// ── Last-read sensor values (sentinels when unavailable)
int   lastMoisture = -1;     // 0–100 %, -1 = never read
float lastSoilTemp = -127.0f; // °C, -127 = DS18B20 not found
float lastAirTemp  = -127.0f; // °C, -127 = AM2320 not found
float lastAirHumid =   -1.0f; // 0–100 % RH, -1 = not found
int   lastLight    =   -1;   // 0–100 %, -1 = never read

// ── Setup ─────────────────────────────────────────────
void setupSensors()
{
  // DS18B20 — soil temperature
  ds18b20.begin();
  ds18b20.setResolution(9); // 9-bit: ±0.5°C, ~100 ms conversion
  int devCount = ds18b20.getDeviceCount();
  if (devCount > 0)
    logf("DS18B20 found (%d device%s)\n", devCount, devCount == 1 ? "" : "s");
  else
    log("DS18B20 not found — soil temp unavailable");

  // AM2320 — air temperature + humidity (I2C)
  Wire.begin(); // SDA=21, SCL=22 on ESP32
  if (am2320.begin())
    log("AM2320 ready");
  else
    log("AM2320 not found — air readings unavailable");
}

// ── Capacitive moisture sensor ────────────────────────
int readMoisture()
{
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(PIN_CAP);
    delay(10);
  }
  return constrain(map(sum / SAMPLE_COUNT, CAP_AIR, CAP_WATER, 0, 100), 0, 100);
}

// ── DS18B20 soil temperature ──────────────────────────
// Returns °C; returns -127 if sensor not connected.
float readSoilTemp()
{
  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(0);
  return (t == DEVICE_DISCONNECTED_C) ? -127.0f : t;
}

// ── AM2320 air temperature + humidity ─────────────────
// Updates lastAirTemp and lastAirHumid; keeps last good
// value if a read fails (NaN returned by the library).
void readAirTempHumid()
{
  float t = am2320.readTemperature();
  float h = am2320.readHumidity();
  if (!isnan(t)) lastAirTemp  = t;
  if (!isnan(h)) lastAirHumid = h;
}

// ── LDR light sensor ─────────────────────────────────
// Returns 0 (dark) – 100 (full brightness).
// Calibrate LDR_DARK / LDR_BRIGHT in config.h.
int readLight()
{
  // Average 3 samples to reduce ADC noise
  long sum = 0;
  for (int i = 0; i < 3; i++) { sum += analogRead(PIN_LDR); delay(5); }
  return constrain(map(sum / 3, LDR_DARK, LDR_BRIGHT, 0, 100), 0, 100);
}

// ── Read all sensors, update last* globals ────────────
void readAllSensors()
{
  lastMoisture = readMoisture();
  lastSoilTemp = readSoilTemp();
  readAirTempHumid(); // updates lastAirTemp, lastAirHumid
  lastLight    = readLight();
}
