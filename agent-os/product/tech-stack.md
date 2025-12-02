# Tech Stack

## Hardware Platforms

### Primary Target: JVTech v4.1 Board
- **Main Processor:** ESP32 (Espressif32)
- **Auxiliary Processor:** ATmega328P (atmelavr) for Ethernet and peripheral management
- **LoRa Module:** SX1276/RFM95W
- **Ethernet Module:** W5500
- **GPS Module:** L80-M39
- **RTC:** DS1307
- **Displays:** LCD 16x2 (I2C), OLED SSD1306
- **Storage:** SD Card slot
- **Audio:** Buzzer for alerts

### Supported Alternative Boards
- Heltec WiFi LoRa 32 V2
- TTGO LoRa32
- Custom ESP32 configurations with SX1276/RFM95W

## Development Environment

| Component | Technology |
|-----------|------------|
| Build System | PlatformIO |
| Framework | Arduino |
| ESP32 Platform | espressif32 |
| ATmega Platform | atmelavr |

## Firmware Libraries

### Core Libraries
| Library | Purpose |
|---------|---------|
| RadioLib | LoRa radio communication (SX1276/RFM95W) |
| ESPAsyncWebServer | Async HTTP server for web interface |
| AsyncTCP | TCP support for async web server |
| ArduinoJson | JSON parsing and serialization |

### Display Libraries
| Library | Purpose |
|---------|---------|
| Adafruit GFX | Graphics primitives for displays |
| Adafruit SSD1306 | OLED display driver |
| LiquidCrystal_I2C | LCD 16x2 I2C driver |

### System Libraries
| Library | Purpose |
|---------|---------|
| LittleFS | Filesystem for configuration storage |
| WiFi | ESP32 WiFi connectivity |
| Wire | I2C communication |
| SPI | SPI communication for LoRa and SD |

## Web Frontend

| Component | Technology |
|-----------|------------|
| Markup | HTML5 |
| Styling | CSS3 (vanilla) |
| Logic | JavaScript (vanilla, ES6+) |
| Storage | LittleFS on ESP32 flash |

## Network Protocols

| Protocol | Purpose |
|----------|---------|
| Semtech UDP Packet Forwarder v2 | LoRaWAN packet forwarding to network server |
| HTTP/REST | Web interface and API |
| NTP | Network time synchronization |
| DHCP | Network configuration (WiFi and Ethernet) |

## Network Server Compatibility

- **ChirpStack:** Primary target, fully compatible
- **The Things Network:** Compatible via UDP packet forwarder protocol
- **Other LoRaWAN servers:** Any server supporting Semtech UDP Packet Forwarder v2

## Filesystem Structure

```
/data/                  # LittleFS partition
  /config/              # Configuration files (JSON)
  /www/                 # Web interface files (HTML, CSS, JS)
```

## Build Configurations

Multiple build environments configured in `platformio.ini`:
- `jvtech_v4` - JVTech v4.1 board with ATmega bridge
- `heltec_v2` - Heltec WiFi LoRa 32 V2
- `ttgo_lora32` - TTGO LoRa32 board
- `atmega328p` - ATmega328P auxiliary processor firmware
