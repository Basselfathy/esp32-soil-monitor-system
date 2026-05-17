
#pragma once
#include "secrets.h"

// ── Credentials ───────────────────────────────────────
const char *AUTH_USER  = SECRET_AUTH_USER;
const char *AUTH_PASS  = SECRET_AUTH_PASS;
const char *WIFI_SSID1 = SECRET_WIFI_SSID1;
const char *WIFI_PASS1 = SECRET_WIFI_PASS1;
const char *WIFI_SSID2 = SECRET_WIFI_SSID2;
const char *WIFI_PASS2 = SECRET_WIFI_PASS2;

// ── GPIO pins ─────────────────────────────────────────
const int PIN_CAP     = 34;  // Capacitive moisture sensor (analog)
const int PIN_RELAY   = 26;  // Relay signal — active-HIGH
const int PIN_DS18B20 =  4;  // DS18B20 soil temp — OneWire, 4.7kΩ pull-up to 3.3V
// AM2320 air temp+humidity — I2C: SDA=21, SCL=22 (ESP32 defaults)
const int PIN_LDR     = 35;  // LDR (CDS) — voltage divider: 3.3V→LDR→GPIO35→10kΩ→GND
const int PIN_LED     = 25;  // Dimmable LED — 220Ω series resistor to GND

// ── Sensor calibration (runtime-adjustable — saved to /calib.json) ──
// These are plain ints so the Calibration tab can update them at runtime.
int CAP_AIR    = 3060; // moisture: raw ADC in open air  → 0 %
int CAP_WATER  =  940; // moisture: raw ADC submerged    → 100 %
const int SAMPLE_COUNT = 5;  // ADC samples to average

int LDR_DARK   =    0; // light: raw ADC in total darkness  → 0 %
int LDR_BRIGHT = 4095; // light: raw ADC in brightest light → 100 %

// ── Timing ────────────────────────────────────────────
const unsigned long WIFI_TIMEOUT = 15000;
const unsigned long WIFI_RETRY   = 30000;
const unsigned long NTP_RETRY    = 30000;

// ── Storage limits ────────────────────────────────────
const int MAX_RECORDS = 2880; // 48 h at 1-min interval
const int TRIM_CHUNK  =  288; // drop oldest 10 % when full
const int MAX_BUFFER  =   10; // offline reading buffer size

// ── SPIFFS file paths ─────────────────────────────────
const char *LOG_FILE = "/readings.json";
const char *TMP_FILE = "/readings.tmp";
const char *CFG_FILE   = "/pump.json";
const char *LED_FILE   = "/led.json";
const char *SYS_FILE   = "/syslog.json";
