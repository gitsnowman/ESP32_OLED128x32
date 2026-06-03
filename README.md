# 🌦️ ESP32 Internet Clock with Weather Forecast (LCD 2004)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Wokwi Simulation](https://img.shields.io/badge/Wokwi-Simulation-2ea44f)](https://wokwi.com/projects/447447731628210177)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red)](https://www.espressif.com/en/products/socs/esp32)

This project is a simple weather station based on an ESP32 microcontroller and a character LCD 2004 display. The device connects to Wi-Fi, fetches accurate time from an NTP server, and retrieves weather data from the OpenWeatherMap API, displaying everything on the screen.

**🖥️ Live simulation on Wokwi:** [Click here to view](https://wokwi.com/projects/447447731628210177)

## 🛠️ Hardware Required

To build the physical device you will need:

| Component | Purpose | Note |
|:---|:---|:---|
| **ESP32** (DevKit v1) | Main microcontroller | Any WiFi-enabled ESP32 board |
| **LCD 2004 (I2C)** | Display for time and weather | Default I2C address: `0x27` |
| **Jumper wires (F-F)** | Connections | 4 wires (VCC, GND, SDA, SCL) |

### Wiring Diagram

| ESP32 Pin | LCD 2004 (I2C) Pin |
|:---|:---|
| **3.3V / 5V** | VCC |
| **GND** | GND |
| **GPIO 21 (SDA)** | SDA |
| **GPIO 22 (SCL)** | SCL |

> **💡 Note:** Make sure your display has an I2C backpack. The I2C address may be `0x27` or `0x3F`. Change it in the code if necessary.

## 📚 Required Libraries (for Arduino IDE)

Install the following libraries via the Library Manager (`Sketch` -> `Include Library` -> `Manage Libraries...`):

| Library | Version (min) | Purpose |
|:---|:---|:---|
| **[LiquidCrystal I2C](https://github.com/johnrickman/LiquidCrystal_I2C)** | - | Control LCD 2004 via I2C |
| **[ArduinoJson](https://arduinojson.org/)** | 6.x | Parse JSON responses from the weather API |
| **[WiFi](https://github.com/esp8266/Arduino/tree/master/libraries/WiFi)** | - | Built-in library for Wi-Fi connection |
| **[HTTPClient](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient)** | - | Built-in library for HTTP requests |
| **[Time](https://github.com/PaulStoffregen/Time)** | - | Time and timezone management |

Libraries `WiFi`, `HTTPClient` and `Time` are usually pre-installed with the ESP32 board package.

## ⚙️ Configuration & Setup

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/YOUR_USERNAME/esp32-lcd2004-web-weather.git
