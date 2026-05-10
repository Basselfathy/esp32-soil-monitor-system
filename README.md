# Soil Moisture Dashboard — ESP32

A self-hosted soil moisture monitor with automated irrigation, a real-time web dashboard, and over-the-air firmware updates.

---

## Table of Contents

1. [Hardware](#hardware)
2. [Wiring](#wiring)
3. [Software setup](#software-setup)
4. [Configuration](#configuration)
5. [First flash](#first-flash)
6. [Accessing the dashboard](#accessing-the-dashboard)
7. [Dashboard overview](#dashboard-overview)
8. [Automatic irrigation](#automatic-irrigation)
9. [Settings reference](#settings-reference)
10. [Updating firmware over Wi-Fi](#updating-firmware-over-wi-fi)
11. [Troubleshooting](#troubleshooting)

---

## Hardware

| Part                            | Notes                                                     |
| ------------------------------- | --------------------------------------------------------- |
| ESP32 WROOM-32 dev board        | Any 4 MB flash variant                                    |
| Capacitive soil moisture sensor | Outputs an analog voltage —**not** resistive type  |
| 5 V relay module                | Active-high (default) or active-low — see[Wiring](#wiring)  |
| Submersible mini pump           | Rated for your relay's switching capacity                 |
| 3.7 V power supply                | Shared for ESP32 + relay; pump may need a separate supply |

---

## Wiring

<img width="3000" height="1863" alt="circuit_image" src="https://github.com/user-attachments/assets/c8398140-dcb2-4f54-bf4b-86c4270b24a7" />

```
  ESP32                    Soil Sensor
  ─────────────────────    ───────────
  5V     ──────────────► VCC
  GND      ──────────────► GND
  GPIO 34  ◄────────────── AOUT


  ESP32                    Relay Module
  ─────────────────────    ────────────
  GPIO 26  ──────────────► IN
  5V       ──────────────► VCC
  GND      ──────────────► GND


  PSU (+3.7V) ──────────────► Relay NO ──┐
                                        │ (closed when pump ON)
  Relay COM ◄─────────────────────────┘
  Relay COM ──────────────► Pump  (+)
  PSU (GND) ──────────────► Pump  (−)
```

| Wire                  | From                   | To                     |
| --------------------- | ---------------------- | ---------------------- |
| Sensor power          | ESP32 5 V            | Sensor VCC             |
| Sensor ground         | ESP32 GND              | Sensor GND             |
| Sensor signal         | Sensor AOUT            | ESP32**GPIO 34** |
| Relay signal          | ESP32**GPIO 26** | Relay IN               |
| Relay power           | battery 3.7 V              | Relay VCC              |
| Relay ground          | ESP32 GND              | Relay GND              |
| Pump power (switched) | PSU +3.7 V               | Relay NO               |
| Pump return           | Relay COM              | Pump +                 |
| Pump ground           | PSU GND                | Pump −                |

> **Active-low relay?** Open `soil_moist_dashboard_esp32.ino`, find `setPump()`, and swap `HIGH`/`LOW` in the two `digitalWrite` calls.

---

## Software setup

### Arduino IDE

1. Install **Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software)
2. Add the ESP32 board package:
   - Go to **File → Preferences → Additional boards manager URLs** and add:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Open **Tools → Board → Boards Manager**, search **esp32** (by Espressif), and install it
3. Select your board: **Tools → Board → esp32 → ESP32 Dev Module**

### Required libraries

Install all of these via **Tools → Manage Libraries**:

| Library     | Author                     |
| ----------- | -------------------------- |
| ArduinoJson | Benoit Blanchon            |
| WebSockets  | Markus Sattler (Links2004) |

> `ArduinoOTA`, `WebServer`, `SPIFFS`, and `WiFi` are bundled with the ESP32 board package — no separate install needed.

---

## Configuration

Open `secrets.h` and fill in your details:

```cpp
// Up to two Wi-Fi networks — the device tries the first, falls back to the second
#define SECRET_WIFI_SSID1  "YourNetwork"
#define SECRET_WIFI_PASS1  "YourPassword"

#define SECRET_WIFI_SSID2  "YourBackupNetwork"
#define SECRET_WIFI_PASS2  "YourBackupPassword"

// Username and password for the dashboard and OTA update page
#define SECRET_AUTH_USER   "admin"
#define SECRET_AUTH_PASS   "changeme"
```

### Sensor calibration (optional)

If your sensor reads inaccurate percentages, measure the raw ADC values in dry air and submerged in water and update these two constants at the top of the main `.ino` file:

```cpp
const int CAP_AIR   = 3060;   // raw ADC reading in open air (0 % moisture)
const int CAP_WATER =  940;   // raw ADC reading fully submerged (100 % moisture)
```

---

## First flash

The first upload **must** be done over USB:

1. Connect the ESP32 to your PC via USB
2. Select the correct port: **Tools → Port**
3. Click **Upload** (the arrow button)
4. Open **Tools → Serial Monitor** at **115200 baud** to watch the boot log
5. The device will print its IP address when connected — note it down

After the first flash all future updates can be done over Wi-Fi (see [Updating firmware over Wi-Fi](#updating-firmware-over-wi-fi)).

---

## Accessing the dashboard

Open a browser and navigate to the device's IP address ( you must be on the same network ):

```
http://192.168.x.x/
```

You will be prompted for the username and password set in `secrets.h`.

> **Tip:** Assign a static IP or a DHCP reservation to the ESP32 in your router settings so the address never changes.

---

## Dashboard overview
<img width="1919" height="911" alt="Screenshot 2026-05-10 155528" src="https://github.com/user-attachments/assets/f20de7a8-050b-4669-98d2-6927b55299f6" />


The dashboard is organised into four tabs:

### Overview

- **Moisture gauge** — current soil moisture percentage
- **History chart** — up to 48 hours of readings with date and time labels
- Connection status and RSSI in the header

### Pump

- Live pump status (ON / OFF) and current mode (AUTO / MANUAL / SOAKING / COOLDOWN)
- Progress bars for run duration, soak period, and cooldown
- **ON / OFF** buttons for manual control
- **Auto** toggle to switch between automatic and manual mode
- Last run timestamp

### Settings

- **Low threshold** — moisture percentage below which the pump turns on automatically
- **Sample interval** — how often the sensor is read (seconds)
- **Run duration** — how long the pump runs per cycle (seconds)
- **Soak time** — how long to wait after pumping before re-evaluating moisture (seconds)
- **Cooldown** — minimum time between two automatic pump cycles (seconds)
- **Firmware update** button — opens the OTA update page

### Logs

- Live serial monitor showing device events in real time

---

## Automatic irrigation

When **Auto mode** is on the device follows this cycle:

```
Soil moisture < Low threshold
        ↓
   Pump runs for [Run duration]
        ↓
   Soak period [Soak time]
   (waits for water to absorb)
        ↓
   Cooldown [Cooldown]
   (prevents rapid re-cycling)
        ↓
   Re-evaluate moisture → repeat if still dry
```

Turning the pump **OFF** manually while in auto mode starts the cooldown timer. Turning it **ON** manually bypasses the cycle entirely and runs the pump until you turn it off or switch back to auto.

---

## Settings reference

| Setting         | Default       | Description                                       |
| --------------- | ------------- | ------------------------------------------------- |
| Low threshold   | 30 %          | Pump activates when moisture drops below this     |
| Sample interval | 60 s          | Sensor reading frequency                          |
| Run duration    | 30 s          | How long the pump runs each cycle                 |
| Soak time       | 300 s (5 min) | Wait after pumping before checking moisture again |
| Cooldown        | 300 s (5 min) | Minimum gap between automatic pump starts         |

All settings are saved to flash and survive reboots.

---

## Updating firmware over Wi-Fi

After the initial USB flash you can update wirelessly in two ways:

### Option A — Web upload (recommended)

1. In Arduino IDE 2.x: **Sketch → Export Compiled Binary**
2. Find the exported file — it will be named`soil_moist_dashboard_esp32.ino.bin`
   > ⚠️ Do **not** use the `*.merged.bin` file — that includes the bootloader and will not work for OTA.
   >
3. Open the dashboard → **Settings tab** → click **Firmware update**
4. Select the `.bin` file and click **Upload & Update**
5. Wait for the progress bar to complete — the device reboots automatically

### Option B — Arduino IDE OTA

1. Make sure the device is on the same network as your PC
2. Go to **Tools → Port** — a network port named `soil-monitor` should appear
3. Select it and click **Upload** as normal
4. Enter the OTA password (same as `SECRET_AUTH_PASS`) when prompted

---

## Troubleshooting

| Symptom                                            | Likely cause                         | Fix                                                                                 |
| -------------------------------------------------- | ------------------------------------ | ----------------------------------------------------------------------------------- |
| Device not connecting to Wi-Fi                     | Wrong credentials                    | Double-check `secrets.h` and re-flash                                             |
| Moisture always reads 0 % or 100 %                 | Sensor not calibrated                | Adjust `CAP_AIR` / `CAP_WATER` constants                                        |
| Pump never turns on in auto mode                   | Threshold too low or cooldown active | Check the Pump tab for active cooldown; raise the threshold                         |
| Chart shows no data                                | NTP not synced yet                   | Check Logs tab — readings are skipped until time is known                          |
| OTA upload completes but device keeps old firmware | Wrong `.bin` file used             | Use `*.ino.bin`, not `*.merged.bin`                                             |
| Web OTA fails mid-upload                           | File too large for available flash   | The sketch + all headers must fit within the OTA partition (~1.8 MB on a 4 MB chip) |
| "Update failed" message                            | Flash write error                    | Check the Logs tab for the specific error; try rebooting the device first           |
| Dashboard login not accepted                       | Wrong credentials                    | Edit `secrets.h`, re-flash                                                        |
