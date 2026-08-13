# OctoScreen 🖨️✨

**OctoScreen** is an advanced, ESP32-based touchscreen control interface designed for 3D printers. It provides native, real-time support for both **OctoPrint** (via HTTP REST API and event-driven MQTT modes) and **Klipper (Moonraker)** systems.

---

## 🚀 Comprehensive Feature List

### 1. Dual Protocol & Connection Management
* **Multi-Server Support:** Seamlessly switch or monitor between OctoPrint and Klipper (Moonraker) backends[cite: 21, 23].
* **Connection Modes:** Supports both timed polling and low-latency event-driven MQTT modes with connection status indicators[cite: 19, 21].
* **Wi-Fi Provisioning & AP Mode:** First-boot Access Point portal (`OctoScreen Configuration`) for easy Wi-Fi network scanning, credential entry, static IP setup, and server parameters[cite: 19, 21].

### 2. Main Screen & Real-Time Monitoring
* **Precise Temperature Tracking:** Real-time nozzle and bed temperatures displayed with **0.01°C precision** and sophisticated threshold filtering.
* **Dynamic Heating Animations:** Pulsing visual indicators for active nozzle and bed heating states.
* **Print Status & Progress:** Live progress bar, status text translation, elapsed/remaining time, and total print time display.
* **Print Controls:** Instant access to Pause, Tune, and Cancel functions directly from the main screen during active prints[cite: 21].

### 3. Preparation & Quick Actions Menu (`SUB_PREPARE`)
* **Auto Home (G28):** Homing routine with animated button state feedback[cite: 19, 21, 25].
* **Steppers Off (M84):** Disable stepper motors on demand[cite: 19, 21, 25].
* **Cooldown:** Instant one-touch command to reset both nozzle and bed target temperatures to 0°C (via HTTP or MQTT)[cite: 19, 21, 25].

### 4. Advanced Calibration & Bed Mesh Suite
* **Bed Leveling Wizard:** Interactive corner-by-corner manual leveling routine (Front-Left, Front-Right, Back-Right, Back-Left coordinates)[cite: 19, 21].
* **Bed Mesh Management:** Generate 3x3 or 5x5 mesh grids, save heights to EEPROM, and handle missing mesh popups[cite: 19, 21].
* **Visual Mesh Viewer:** Color-coded 2D height map grid representation directly on the screen with detailed coordinate readings[cite: 21].
* **Other Calibrations (`SUB_OTHER_CALIB`):**
  * **E-Step Calibration:** Material selection presets (PLA at 200°C, PETG at 230°C, ABS at 240°C), 100mm extrusion routine, and error/remainder calculation[cite: 19, 21].
  * **PID Autotune (M303):** Automated hotend tuning and EEPROM saving[cite: 19, 21].
  * **MPC Calibration:** Model Predictive Control tuning support (where supported by firmware)[cite: 19, 21].

### 5. Tune & Z-Offset Menus
* **Live Adjustments:** Real-time modification of printing speed percentage, nozzle temperature, and bed temperature[cite: 21].
* **Z-Offset Fine-Tuning:** Precise micro-adjustments ($\pm0.01$) with dedicated Z-Homing and EEPROM save (`M500`) capabilities[cite: 19, 21].

### 6. System & Maintenance Tools
* **System Info Screen:** Monitor Free RAM, Firmware Version, OctoPrint version, and total SD card file count[cite: 19, 21].
* **Maintenance Actions:** Immediate device restart, configuration deletion, and full SD card formatting[cite: 19, 21].

### 7. Customization & Localization
* **Multi-Language Support (i18n):** Fully localized dictionaries for:
  * 🇭🇺 Hungarian (`hu`)[cite: 19]
  * 🇬🇧 English (`en`)[cite: 21]
  * 🇩🇪 German (`de`)[cite: 22]
  * 🇵🇱 Polish (`pl`)[cite: 23]
* **Theme & Skin Engine:** Switch between **Dark**, **Light**, and **Colorful** skins with automatic header icon and text color adaptations[cite: 19, 21, 22, 23].

---

## 📦 Prerequisites & Bridge Plugin

To enable full OctoPrint event integration and mesh downloading capabilities, ensure the companion bridge plugin is installed on your OctoPrint server[cite: 19, 21]:
* **OctoKlipscreenBridge** (Repository: [`github.com/karolyia79/OctoklipscreenBridge`](https://github.com/karolyia79/OctoklipscreenBridge))[cite: 19, 21]

---

## 🛠️ Hardware & Environment

* **Microcontroller:** ESP32[cite: 21]
* **Display:** TFT Display driven by the `TFT_eSPI` library[cite: 21]
* **Touch Controller:** CST820 Capacitive Touch[cite: 21]
* **Build Framework:** PlatformIO / Arduino IDE[cite: 21]
