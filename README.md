# Garden Moisture Monitor

Battery-powered garden soil monitor using an **Arduino Nano ESP32**, **15 moisture sensors**, and **Blynk IoT** over WiFi. A solar panel charges a LiPo battery between readings.

> **Note:** You wrote "Blink" — this project uses **[Blynk IoT](https://blynk.io)** (the common Arduino cloud platform). If you meant a different service, say which one and we can adapt.

## What you need

### Hardware
| Item | Qty | Notes |
|------|-----|-------|
| Arduino Nano ESP32 | 1 | ABX00083 |
| Capacitive soil moisture sensors | 15 | Prefer capacitive over resistive (less corrosion) |
| CD74HC4067 analog multiplexer | 1 | 16 channels total (use 15) |
| LiPo battery | 1 | 3.7 V, sized for your runtime |
| TP4056 charge module | 1 | Solar → battery |
| Solar panel | 1 | 5–6 V, current depends on battery size |
| N-channel MOSFET (optional) | 1 | Switches sensor 3.3 V rail (pin D8) |
| Resistors 100 kΩ | 2 | Battery voltage divider for A7 |

### Why multiplexers?
The Nano ESP32 has **8 analog pins** (A0–A7). Fifteen sensors fit on a single **CD74HC4067 16-channel multiplexer** sharing analog pin **A0**.

## Wiring

### Power
```
Solar (+) ──► TP4056 IN+
Solar (-) ──► TP4056 IN-
TP4056 BAT+ ──► LiPo (+) ──► Nano VIN (6–21 V) or 3.3 V regulator
TP4056 BAT- ──► GND
```

For **3.3 V-only** setups, use a buck converter from the battery to **3.3 V** and feed the Nano **3.3 V** pin (stay within board limits).

**Battery monitor** (A7): LiPo (+) ── 100 kΩ ── A7 ── 100 kΩ ── GND  
This 2:1 divider keeps A7 below 3.3 V at full charge.

### Moisture sensors (×15)
Each capacitive sensor:
- **VCC** → 3.3 V (ideally through MOSFET on **D8**)
- **GND** → GND
- **AOUT** → CD74HC4067 channel input (Y0–Y14)

### CD74HC4067 (sensors 1–15)
| CD74HC4067 | Nano ESP32 |
|------------|------------|
| COM (Z) | A0 |
| VDD | 3.3 V |
| VEE | GND |
| EN | D10 (LOW = enabled) |
| S0 | D2 |
| S1 | D3 |
| S2 | D4 |
| S3 | D5 |
| Y0–Y14 | Sensor analog outs 1–15 |

Only one channel is selected at a time; the firmware handles the address lines.

## Software setup

### 1. Arduino IDE
1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. **Boards Manager** → install **Arduino ESP32 Boards** (includes Nano ESP32)
3. **Library Manager** → install **Blynk** (by Volodymyr Shymanskyy)

### 2. Blynk Cloud template
1. Create a free account at [blynk.cloud](https://blynk.cloud)
2. **Developer Zone → My Templates → New Template**
   - Name: `Garden Moisture` (keep short for WiFi provisioning)
   - Hardware: **ESP32**
   - Connection: **WiFi**
3. **Datastreams** — create these:

| Name | Pin | Type | Min | Max | Units |
|------|-----|------|-----|-----|-------|
| zone_1 … zone_15 | V0 … V14 | Integer | 0 | 100 | % |
| battery | V15 | Integer | 0 | 100 | % |
| sleep_minutes | V16 | Integer | 5 | 1440 | min |
| alert_threshold | V17 | Integer | 0 | 100 | % |
| cal_dry_raw | V18 | Integer | 0 | 4095 | — |
| cal_wet_raw | V19 | Integer | 0 | 4095 | — |
| cal_mode | V20 | Integer | 0 | 1 | — |
| cal_zone | V21 | Integer | 1 | 15 | — |
| capture_dry | V22 | Integer | 0 | 1 | — |
| capture_wet | V23 | Integer | 0 | 1 | — |
| cal_raw_live | V24 | Integer | 0 | 4095 | — |

4. **Events** — Template → **Events** → New Event:
   - Name: `Low moisture`
   - Code: `low_moisture` (must match firmware)
   - Enable **Notifications** (push/email)

5. Copy **Firmware configuration** from the template (Template ID + Name)
5. Paste into `GardenMoisture/config.h`:

```cpp
#define BLYNK_TEMPLATE_ID   "TMPxxxxxx"
#define BLYNK_TEMPLATE_NAME "Garden Moisture"
```

6. Build a mobile dashboard with 15 **Gauge** widgets (V0–V14), battery (V15), and a **Settings** section for calibration (see below).

### Calibration (from Blynk app)

1. Turn **V20 (cal_mode)** ON — device stays awake and V0–V14 show **raw ADC** values instead of %
2. Select zone with **V21 (cal_zone)** (1–15)
3. Insert probe in **dry** soil → tap **V22 (capture_dry)**
4. Insert probe in **wet** soil → tap **V23 (capture_wet)**
5. Repeat for each zone; values are saved to flash automatically
6. Turn **V20** OFF to return to normal 30-minute reporting + deep sleep

You can also manually edit **V18/V19** if needed. **V24** shows live raw for the selected zone.

### Low moisture alerts

- Set **V17 (alert_threshold)** — default 25%
- When any zone drops below the threshold, firmware fires `Blynk.logEvent("low_moisture", ...)`
- **Max 1 alert per zone per calendar day** (tracked in flash; resets at midnight local time)
- Enable notifications on the `low_moisture` event in Blynk Console
- Adjust `TIMEZONE_TZ` in `config.h` if daily reset should use a different timezone (e.g. `"EST5EDT"`)

### 3. Copy Blynk.Edgent support files
In PowerShell, from the `GardenMoisture` folder:

```powershell
.\setup-from-blynk.ps1
```

This copies `BlynkEdgent.h`, `ConfigMode.h`, etc. from your installed Blynk library.

### 4. Upload firmware
1. Open `GardenMoisture/GardenMoisture.ino`
2. **Tools → Board → Arduino Nano ESP32**
3. **Tools → Port** → your USB port
4. For **first upload / debugging**, set in `config.h`:
   ```cpp
   #define ENABLE_DEEP_SLEEP false
   ```
5. Upload

### 5. Connect to WiFi (Blynk app)
1. Install **Blynk IoT** app (iOS/Android)
2. **Add New Device → Find Devices Nearby**
3. Select your template name when the provisioning network appears
4. Enter home WiFi credentials
5. Device appears online in the app

**Reset WiFi:** hold **BOOT** button ~10 seconds (Edgent factory reset).

## Calibration

In `config.h`, adjust after testing one sensor:

```cpp
#define MOISTURE_RAW_DRY  3200   // probe in air / dry soil
#define MOISTURE_RAW_WET   1400   // probe in water / saturated soil
```

Open **Serial Monitor** at **115200 baud** to see raw behavior while tuning.

## Power behavior

- Default: wake every **30 minutes**, read all sensors, push to Blynk, **deep sleep**
- Change interval from the app via **V16** (5–1440 minutes); stored in RTC memory across sleeps
- Set `ENABLE_DEEP_SLEEP false` for USB debugging (stays awake)

Expected draw: most time in deep sleep (~µA on ESP32-S3); WiFi bursts dominate energy use. Shorter intervals = faster battery drain.

## Pin map summary

| Function | Pin |
|----------|-----|
| Mux analog in | A0 |
| Mux1 select | D2, D3, D4 |
| Mux1 enable | D10 |
| Mux2 select | D5, D6, D7 |
| Mux2 enable | D9 |
| Sensor power switch | D8 |
| Battery sense | A7 |
| Status LED | D13 |
| WiFi reset button | BOOT (GPIO0) |

## Troubleshooting

| Problem | Fix |
|---------|-----|
| All sensors read 0 or 100 | Recalibrate `MOISTURE_RAW_DRY` / `MOISTURE_RAW_WET` |
| WiFi provisioning fails | Shorten template name; update ESP32 board package |
| Device sleeps before you can provision | Set `ENABLE_DEEP_SLEEP false`, upload, provision, re-enable |
| Battery reads wrong | Check divider ratio and `BATTERY_V_MIN` / `BATTERY_V_MAX` |
| Compile error: missing BlynkEdgent.h | Run `setup-from-blynk.ps1` |

## Detailed Guides

For step-by-step recovery and future maintenance, see:

- `GardenMoisture/CONNECTION_GUIDE.md`
- `GardenMoisture/BOOT_DEBUGGING_GUIDE.md`

## Project structure

```
Moisture/
├── README.md
└── GardenMoisture/
    ├── GardenMoisture.ino   ← main firmware
    ├── config.h             ← your settings + Blynk template ID
    ├── sensors.h            ← multiplexer + ADC logic
    ├── Settings.h             ← Nano ESP32 board config for Edgent
    ├── setup-from-blynk.ps1   ← copies Edgent headers from library
    └── (Edgent files from script)
```

## Next steps

- Label each zone in the Blynk dashboard (tomatoes, herbs, etc.)
- Add Blynk **Alerts** when moisture drops below a threshold
- Tune sleep interval vs. battery + solar capacity for your garden
