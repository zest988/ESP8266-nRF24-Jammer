# ESP8266 nRF24L01+ Jammer with Web Interface

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP8266-blue)](https://www.arduino.cc/reference/en/esp8266/)
[![Library](https://img.shields.io/badge/library-RF24-orange)](https://github.com/nRF24/RF24)

A **2.4 GHz signal jammer** built with **ESP8266 D1 (AZDelivery)** and **nRF24L01+** module. Features a responsive web interface for remote control, channel presets, adjustable burst packets, power level selection, auto-stop timer, and built‑in LED indication.

> ⚠️ **WARNING**: This project is for **educational purposes only**. Unauthorized jamming of wireless communications is illegal in most countries. Use only in a controlled, shielded environment with explicit permission.

## Features

- ✅ **Web Interface** – control from any device via Wi‑Fi (AP mode)
- ✅ **Real‑time status** – shows jamming state and elapsed time
- ✅ **Channel Presets** – full 2.4GHz (0‑125) or Wi‑Fi only (1‑11)
- ✅ **Adjustable Burst Packets** – 1 to 20 packets per channel
- ✅ **Power Level** – LOW (-12dBm), MEDIUM (-6dBm), HIGH (0dBm)
- ✅ **Auto‑stop Timer** – set jamming duration in minutes
- ✅ **LED Indication** – onboard LED shows status (solid ON = jamming)
- ✅ **Non‑blocking** – uses Ticker for smooth web server response

## Hardware Required

| Component | Description |
|-----------|-------------|
| ESP8266 D1 (AZDelivery / NodeMCU) | Main microcontroller |
| nRF24L01+ module | 2.4 GHz transceiver |
| Breadboard & jumper wires | For connections |
| USB cable (data capable) | Power and programming |

## Pin Connections

| nRF24L01 Pin | ESP8266 D1 Label | GPIO |
|--------------|------------------|------|
| **VCC**      | 3.3V             | -    |
| **GND**      | GND              | -    |
| **CE**       | D2               | 4    |
| **CSN**      | D8               | 15   |
| **SCK**      | D5               | 14   |
| **MOSI**     | D7               | 13   |
| **MISO**     | D6               | 12   |

> 💡 The built‑in LED is on **GPIO2 (D4)** – used for status indication.

## Installation & Setup

### 1. Install Arduino IDE & ESP8266 Board

- Install [Arduino IDE](https://www.arduino.cc/en/software)
- Add ESP8266 board support:  
  **File → Preferences → Additional Boards Manager URLs**  
  Add: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
- Install board: **Tools → Board → Boards Manager** → search `esp8266` → install

### 2. Install Required Libraries

- **RF24** by TMRh20 (Library Manager)
- **ESP8266WiFi** (built-in)
- **ESP8266WebServer** (built-in)
- **Ticker** (built-in)

### 3. Upload the Code

- Download `esp8266_nrf24_jammer.ino` from this repo
- Open in Arduino IDE
- Select board: **NodeMCU 1.0 (ESP‑12E Module)**
- Select correct COM port
- Click **Upload**

### 4. Using the Web Interface

After uploading:

1. Open **Serial Monitor** (115200 baud) – confirm IP address (should be `192.168.4.1`)
2. On your phone/laptop, connect to Wi‑Fi: **`NRF_Jammer`** (password: `12345678`)
3. Open browser and go to **`http://192.168.4.1`**
4. Use START/STOP buttons and adjust settings

## LED Indication

| LED Behavior | Meaning |
|--------------|---------|
| 3 blinks at startup | Board ready, Wi‑Fi AP active |
| Solid ON | Jamming active |
| OFF | Jamming stopped |
| Fast blinking | nRF24 not detected (check wiring) |

## Customization

You can modify these constants in the code:

| Parameter | Location | Description |
|-----------|----------|-------------|
| `ap_ssid` | line ~30 | Wi‑Fi AP name |
| `ap_password` | line ~31 | Wi‑Fi password |
| `JAMMER_INTERVAL_MS` | line ~95 | Channel hop speed (ms) |
| `payload[]` size | line ~98 | Bytes per packet (16 default) |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Wi‑Fi network not visible | Check USB cable (data capable). Reset board. |
| nRF24 not detected | Verify wiring, especially VCC (3.3V) and GND. Try `RF24_PA_LOW` if power unstable. |
| Compilation error | Ensure RF24 library installed. Select correct board. |
| Web page not loading | Connect to `NRF_Jammer` Wi‑Fi. Use `http://192.168.4.1` (not HTTPS). |

## Legal Disclaimer

**This software is provided for educational and research purposes only.**  
The author is not responsible for any misuse or damage caused by this device.  
Jammers are illegal to operate without authorization in many jurisdictions, including the US (FCC), EU, and others. Always comply with local laws.

## Credits

- Original concept by [W0rthlessS0ul](https://github.com/W0rthlessS0ul/nRF24_jammer)
- Adapted and extended for ESP8266 D1 with web interface and LED indication
- RF24 library by [TMRh20](https://github.com/nRF24/RF24)

## License

MIT License – see [LICENSE](LICENSE) file.

---

⭐ If this project helped you learn about RF or ESP8266, consider giving it a star!
