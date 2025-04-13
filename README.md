

## 🧠 ESP32 Blind Controller with Home Assistant MQTT Auto-Discovery

This project is a smart blind controller using an **ESP32** and a **DRV8825 stepper motor driver**, with full integration into **Home Assistant** using **MQTT Auto-Discovery**.

### ✨ Features

- Control a stepper motor to open/close window blinds
- Supports position calibration (top/bottom limit)
- Stores calibration in EEPROM
- MQTT-based control with Home Assistant auto-registration
- Supports manual calibration mode and Home Assistant automation
- Wi-Fi and MQTT connection with retained status

---

### 🛠 Hardware Required

- ESP32 Dev Board
- DRV8825 Stepper Motor Driver
- Stepper Motor (e.g., NEMA 17)
- Limit Switches (for top/bottom positions)
- External Power Supply (12V/24V for motor)
- Optional: buttons or LEDs for feedback

---

### 🔌 Pin Connections

| ESP32 Pin | DRV8825 Pin |
|-----------|-------------|
| GPIO 18   | STEP        |
| GPIO 19   | DIR         |
| GPIO 21   | ENABLE      |
| GPIO 34   | Limit Switch Top (Input Pull-Up) |
| GPIO 35   | Limit Switch Bottom (Input Pull-Up) |

> Modify pin numbers in code if needed.

---

### 📦 Libraries Used

- [`WiFi.h`](https://www.arduino.cc/en/Reference/WiFi)
- [`PubSubClient`](https://pubsubclient.knolleary.net/) (MQTT)
- [`EEPROM`](https://www.arduino.cc/en/Reference/EEPROM)
- [`ArduinoJson`](https://arduinojson.org/) (for MQTT discovery)

Install them via Library Manager or PlatformIO.

---

### 🔐 Secrets Configuration

Move your credentials to a `secrets.h` file:

```cpp
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASS     "your_wifi_password"
#define MQTT_SERVER   "your_mqtt_broker_ip"
#define MQTT_USER     "your_mqtt_username"
#define MQTT_PASS     "your_mqtt_password"
```

Include it in your sketch:
```cpp
#include "secrets.h"
```

---

### 🧩 MQTT Topics

| Topic                                | Function                     |
|--------------------------------------|------------------------------|
| `home/blind/<room>/set`              | Open, close, or position (%) |
| `home/blind/<room>/state`            | Report current position (%)  |
| `home/blind/<room>/calibrate`        | Trigger auto calibration     |
| `home/blind/status`                  | Online status                |

You can also manually send:
- `save_top`
- `save_bottom`
- `set_position`

For manual calibration, if implemented.

---

### 🏠 Home Assistant Integration

Auto-discovers using:
```
homeassistant/cover/<room>_blind/config
```

Then appears automatically in **Devices & Services → MQTT → Devices**.

Control via Dashboard with `cover` card or use in automations.

---

### 🚀 Flash & Run

1. Flash the code using Arduino IDE or PlatformIO.
2. Open Serial Monitor (115200 baud) to see logs.
3. Ensure MQTT and Wi-Fi credentials are correct.
4. Entity will appear in Home Assistant.

---

### 📷 Example Usage

```yaml
cover:
  - platform: mqtt
    name: "Living Room Blind"
    command_topic: "home/blind/living_room/set"
    state_topic: "home/blind/living_room/state"
    set_position_topic: "home/blind/living_room/set"
    retain: true
```

---

