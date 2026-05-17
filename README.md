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
9. [LED dimming](#led-dimming)
10. [Settings reference](#settings-reference)
11. [Updating firmware over Wi-Fi](#updating-firmware-over-wi-fi)
12. [Troubleshooting](#troubleshooting)

---

## Hardware

| Part                            | Notes                                                                  |
| ------------------------------- | ---------------------------------------------------------------------- |
| ESP32 WROOM-32 dev board        | Any 4 MB flash variant                                                 |
| Capacitive soil moisture sensor | Outputs an analog voltage — **not** resistive type                     |
| 5 V relay module                | Active-high (default) or active-low — see [Wiring](#wiring)            |
| Submersible mini pump           | Rated for your relay's switching capacity                              |
| DS18B20 soil temperature sensor | Waterproof probe; requires a 4.7 kΩ pull-up resistor                  |
| AM2320 air temp + humidity      | I²C sensor; no pull-ups needed (internal)                              |
| LDR (20 mm CDS photoresistor)   | Voltage divider with a 10 kΩ resistor to GND                          |
| LED (any colour)                | Driven via PWM through a 220 Ω series resistor (red/green) or 47 Ω (blue/white) |
| 3.7 V power supply              | Shared for ESP32 + relay; pump may need a separate supply              |

---

## Wiring

<img width="3000" height="1863" alt="circuit_image" src="https://github.com/user-attachments/assets/c8398140-dcb2-4f54-bf4b-86c4270b24a7" />

```
  ESP32                    Soil Moisture Sensor
  ─────────────────────    ────────────────────
  3.3V     ─────────────► VCC
  GND      ─────────────► GND
  GPIO 34  ◄────────────── AOUT


  ESP32                    Relay Module
  ─────────────────────    ────────────
  GPIO 26  ─────────────► IN
  5V       ─────────────► VCC
  GND      ─────────────► GND


  PSU (+) ──────────────► Relay NO ──┐
                                     │ (closed when pump ON)
  Relay COM ◄──────────────────────┘
  Relay COM ────────────► Pump (+)
  PSU (GND) ────────────► Pump (−)


  ESP32                    DS18B20 (soil temperature)
  ─────────────────────    ──────────────────────────
  3.3V     ─────────────► VCC  (also connect to DATA via 4.7 kΩ pull-up)
  GND      ─────────────► GND
  GPIO 4   ◄────────────── DATA


  ESP32                    AM2320 (air temp + humidity)
  ─────────────────────    ───────────────────────────
  3.3V     ─────────────► VCC
  GND      ─────────────► GND
  GPIO 21  ──────────────► SDA
  GPIO 22  ──────────────► SCL


  ESP32                    LDR (light sensor)
  ─────────────────────    ──────────────────
  3.3V  ── LDR ──┬──────── GPIO 35
                 └── 10 kΩ ── GND


  ESP32                    LED
  ─────────────────────    ───
  GPIO 25 ── 220 Ω ── LED anode (+) ── LED cathode (−) ── GND
  (use 47 Ω for blue or white LEDs)
```

| Wire                  | From                    | To                          |
| --------------------- | ----------------------- | --------------------------- |
| Moisture sensor power | ESP32 3.3 V             | Sensor VCC                  |
| Moisture sensor GND   | ESP32 GND               | Sensor GND                  |
| Moisture sensor signal| Sensor AOUT             | ESP32 **GPIO 34**           |
| Relay signal          | ESP32 **GPIO 26**       | Relay IN                    |
| Relay power           | 5 V supply              | Relay VCC                   |
| Relay ground          | ESP32 GND               | Relay GND                   |
| Pump power (switched) | PSU +                   | Relay NO                    |
| Pump return           | Relay COM               | Pump +                      |
| Pump ground           | PSU GND                 | Pump −                      |
| DS18B20 power         | ESP32 3.3 V             | DS18B20 VCC                 |
| DS18B20 ground        | ESP32 GND               | DS18B20 GND                 |
| DS18B20 data          | ESP32 **GPIO 4**        | DS18B20 DATA (+ 4.7 kΩ to 3.3 V) |
| AM2320 power          | ESP32 3.3 V             | AM2320 VCC                  |
| AM2320 ground         | ESP32 GND               | AM2320 GND                  |
| AM2320 SDA            | ESP32 **GPIO 21**       | AM2320 SDA                  |
| AM2320 SCL            | ESP32 **GPIO 22**       | AM2320 SCL                  |
| LDR (top leg)         | ESP32 3.3 V             | LDR leg 1                   |
| LDR (bottom leg)      | LDR leg 2               | ESP32 **GPIO 35** + 10 kΩ to GND |
| LED resistor          | ESP32 **GPIO 25**       | 220 Ω resistor              |
| LED anode             | 220 Ω resistor          | LED + (longer leg)          |
| LED cathode           | LED − (shorter leg)     | GND                         |

> **Active-low relay?** Open `soil_moist_dashboard_esp32.ino`, find `setPump()`, and swap `HIGH`/`LOW` in the two `digitalWrite` calls.

> **Blue or white LED?** The forward voltage (~3.0–3.3 V) leaves almost no headroom from 3.3 V through 220 Ω. Use a **47 Ω** resistor instead.

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

### Sensor calibration

All four calibration values can be tuned **at runtime** from the **Calib tab** in the dashboard — no reflash needed. Changes are saved to flash immediately.

| Constant     | Default | Meaning                                        |
| ------------ | ------- | ---------------------------------------------- |
| `CAP_AIR`    | 3060    | Raw ADC reading with sensor in open air (0 %)  |
| `CAP_WATER`  | 940     | Raw ADC reading with sensor fully submerged (100 %) |
| `LDR_DARK`   | 0       | Raw ADC reading in complete darkness           |
| `LDR_BRIGHT` | 4095    | Raw ADC reading under maximum brightness       |

To calibrate the moisture sensor: note the raw ADC value shown in the Calib tab while the probe is in dry air, then while submerged, and enter both values.

To calibrate the LDR: cover it completely (dark) and note the value, then expose it to the brightest available light and note the value.

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


The dashboard is organised into five tabs:

### Overview

- **Moisture gauge** — current soil moisture percentage
- **Environment cards** — soil temperature (DS18B20), air temperature and humidity (AM2320), ambient light (LDR)
- **History chart** — up to 48 hours of readings with date and time labels
- Connection status and RSSI in the header

### Controls

**Pump card**
- Live pump status (ON / OFF) and current mode (AUTO / MANUAL / SOAKING / COOLDOWN)
- Progress bars for run duration, soak period, and cooldown
- **ON / OFF** buttons for manual control
- **Auto** toggle to switch between automatic and manual mode
- Last run timestamp

**LED card**
- Mode badge (AUTO / MANUAL) and live brightness indicator
- **Brightness slider** — in manual mode, directly sets LED brightness (0–100 %); dragging it while in auto mode switches to manual
- **Threshold slider** — in auto mode, sets the ambient light % above which the LED turns off
- **Full / Off** buttons for quick manual override
- **Auto** toggle to switch between automatic and manual mode

### Settings

- **Low threshold** — moisture percentage below which the pump turns on automatically
- **Sample interval** — how often the full sensor suite is read (seconds)
- **Run duration** — how long the pump runs per cycle (seconds)
- **Soak time** — how long to wait after pumping before re-evaluating moisture (seconds)
- **Cooldown** — minimum time between two automatic pump cycles (seconds)
- **Firmware update** button — opens the OTA update page

### Logs

- Live event log showing device activity in real time

### Calib

- Live raw ADC readings for the moisture sensor and LDR
- Input fields to update `CAP_AIR`, `CAP_WATER`, `LDR_DARK`, and `LDR_BRIGHT` without reflashing
- Values are saved to flash immediately and take effect on the next sensor read

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

## LED dimming

The LED is driven by the ESP32's LEDC peripheral at 1 kHz with 12-bit resolution, giving 4096 brightness steps with no visible flicker.

The LDR is sampled every **50 ms** independently of the main sensor interval, so the LED responds to light changes in near real-time. The dashboard indicator updates at the slower main sensor interval.

### Auto mode

When auto mode is on, brightness is calculated from the ambient light level and the configured threshold:

```
ambient light >= threshold  →  LED off (0 %)
ambient light <  threshold  →  brightness = map(light, 0, threshold, 100, 0)
```

Examples with threshold = 60 %:

| Ambient light | LED brightness |
| ------------- | -------------- |
| 70 % (bright) | 0 % (off)      |
| 50 %          | 17 %           |
| 30 %          | 50 %           |
| 0 % (dark)    | 100 %          |

### Manual mode

Dragging the brightness slider or pressing **Full** / **Off** switches to manual mode and holds the LED at the chosen brightness regardless of ambient light. Press **Auto** to return to automatic control.

---

## Settings reference

### Irrigation

| Setting         | Default       | Description                                       |
| --------------- | ------------- | ------------------------------------------------- |
| Low threshold   | 30 %          | Pump activates when moisture drops below this     |
| Sample interval | 60 s          | Full sensor suite reading frequency               |
| Run duration    | 30 s          | How long the pump runs each cycle                 |
| Soak time       | 300 s (5 min) | Wait after pumping before checking moisture again |
| Cooldown        | 300 s (5 min) | Minimum gap between automatic pump starts         |

### LED

| Setting        | Default | Description                                                   |
| -------------- | ------- | ------------------------------------------------------------- |
| Auto mode      | ON      | Follow LDR reading; off when ambient light ≥ threshold        |
| Threshold      | 50 %    | Ambient light level above which auto mode turns the LED off   |
| Manual bright  | 50 %    | Brightness used when switching to manual mode                 |

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

| Symptom                                            | Likely cause                              | Fix                                                                                           |
| -------------------------------------------------- | ----------------------------------------- | --------------------------------------------------------------------------------------------- |
| Device not connecting to Wi-Fi                     | Wrong credentials                         | Double-check `secrets.h` and re-flash                                                         |
| Moisture always reads 0 % or 100 %                 | Sensor not calibrated                     | Use the **Calib tab** to set `CAP_AIR` and `CAP_WATER`                                        |
| Light always reads 0 % or 100 %                    | LDR not calibrated                        | Use the **Calib tab** to set `LDR_DARK` and `LDR_BRIGHT`                                      |
| Pump never turns on in auto mode                   | Threshold too low or cooldown active      | Check the Controls tab for active cooldown; raise the threshold                               |
| LED stays off even when dark                       | Auto threshold too low, or manual mode    | Open Controls tab — check threshold and mode badge; try pressing **Auto** to re-enable        |
| LED is on but no visible light                     | LED wired backwards                       | Flip the LED — the longer leg (anode +) must face the resistor / GPIO side                   |
| LED very dim even at 100 %                         | Blue/white LED with 220 Ω (too large)    | Replace with a **47 Ω** resistor; blue/white LEDs have ~3.0–3.3 V forward voltage           |
| Chart shows no data                                | NTP not synced yet                        | Check Logs tab — readings are skipped until time is known                                    |
| ArduinoOTA fails with error 2 (connect failed)     | WebSocket I/O interfering during OTA      | Already fixed in firmware — re-flash via USB once, then OTA will work                        |
| OTA upload completes but device keeps old firmware | Wrong `.bin` file used                    | Use `*.ino.bin`, not `*.merged.bin`                                                           |
| Web OTA fails mid-upload                           | File too large for available flash        | The sketch + all headers must fit within the OTA partition (~1.8 MB on a 4 MB chip)           |
| "Update failed" message                            | Flash write error                         | Check the Logs tab for the specific error; try rebooting the device first                     |
| Dashboard login not accepted                       | Wrong credentials                         | Edit `secrets.h`, re-flash                                                                    |
