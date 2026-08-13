# OctoScreen 🖨️✨

**OctoScreen** is an advanced, ESP32-based touchscreen control interface designed for 3D printers. It provides native, real-time support for both **OctoPrint** (via HTTP REST API and event-driven MQTT modes) and **Klipper (Moonraker)** systems.

---

## 🚀 Comprehensive Feature List

### 1. Dual Protocol & Connection Management
* **Multi-Server Support:** Seamlessly switch or monitor between OctoPrint and Klipper (Moonraker) backends.
* **Connection Modes:** Supports both timed polling and low-latency event-driven MQTT modes with connection status indicators.
* **Wi-Fi Provisioning & AP Mode:** First-boot Access Point portal (`OctoScreen Configuration`) for easy Wi-Fi network scanning, credential entry, static IP setup, and server parameters.

### 2. Main Screen & Real-Time Monitoring
* **Precise Temperature Tracking:** Real-time nozzle and bed temperatures displayed with **0.01°C precision** and sophisticated threshold filtering.
* **Dynamic Heating Animations:** Pulsing visual indicators for active nozzle and bed heating states.
* **Print Status & Progress:** Live progress bar, status text translation, elapsed/remaining time, and total print time display.
* **Print Controls:** Instant access to Pause, Tune, and Cancel functions directly from the main screen during active prints.

### 3. Preparation & Quick Actions Menu (`SUB_PREPARE`)
* **Auto Home (G28):** Homing routine with animated button state feedback.
* **Steppers Off (M84):** Disable stepper motors on demand.
* **Cooldown:** Instant one-touch command to reset both nozzle and bed target temperatures to 0°C (via HTTP or MQTT).

### 4. Advanced Calibration & Bed Mesh Suite
* **Bed Leveling Wizard:** Interactive corner-by-corner manual leveling routine (Front-Left, Front-Right, Back-Right, Back-Left coordinates).
* **Bed Mesh Management:** Generate 3x3 or 5x5 mesh grids, save heights to EEPROM, and handle missing mesh popups.
* **Visual Mesh Viewer:** Color-coded 2D height map grid representation directly on the screen with detailed coordinate readings.
* **Other Calibrations (`SUB_OTHER_CALIB`):**
  * **E-Step Calibration:** Material selection presets (PLA at 200°C, PETG at 230°C, ABS at 240°C), 100mm extrusion routine, and error/remainder calculation.
  * **PID Autotune (M303):** Automated hotend tuning and EEPROM saving.
  * **MPC Calibration:** Model Predictive Control tuning support (where supported by firmware).

### 5. Tune & Z-Offset Menus
* **Live Adjustments:** Real-time modification of printing speed percentage, nozzle temperature, and bed temperature.
* **Z-Offset Fine-Tuning:** Precise micro-adjustments ($\pm0.01$) with dedicated Z-Homing and EEPROM save (`M500`) capabilities.

### 6. System & Maintenance Tools
* **System Info Screen:** Monitor Free RAM, Firmware Version, OctoPrint version, and total SD card file count.
* **Maintenance Actions:** Immediate device restart, configuration deletion, and full SD card formatting.

### 7. Customization & Localization
* **Multi-Language Support (i18n):** Fully localized dictionaries for:
  * 🇭🇺 Hungarian (`hu`)
  * 🇬🇧 English (`en`)
  * 🇩🇪 German (`de`)
  * 🇵🇱 Polish (`pl`)
* **Theme & Skin Engine:** Switch between **Dark**, **Light**, and **Colorful** skins with automatic header icon and text color adaptations.

---

## 📦 Prerequisites & Bridge Plugin

To enable full OctoPrint event integration and mesh downloading capabilities, ensure the companion bridge plugin is installed on your OctoPrint server:
* **OctoKlipscreenBridge** (Repository: [`github.com/karolyia79/OctoklipscreenBridge`](https://github.com/karolyia79/OctoklipscreenBridge))

---

## 🛠️ Hardware & Environment

* **Microcontroller:** ESP32
* **Display:** TFT Display driven by the `TFT_eSPI` library
* **Touch Controller:** CST820 Capacitive Touch
* **Build Framework:** PlatformIO / Arduino IDE
