#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// ESP32 Single Channel LoRaWAN Gateway Configuration
// =============================================================================

// Serial configuration
#define SERIAL_BAUD_RATE 115200

// WiFi defaults
#define WIFI_HOSTNAME_DEFAULT "LoRaGateway"
#define WIFI_SSID_DEFAULT "LoRaGW-Config"
#define WIFI_PASS_DEFAULT "lorawan123"
#define WIFI_AP_MODE_DEFAULT true
#define WIFI_MAX_NETWORKS 5

// ChirpStack/Network Server defaults
#define NS_HOST_DEFAULT "localhost"
#define NS_PORT_UP_DEFAULT 1700
#define NS_PORT_DOWN_DEFAULT 1700

// LoRa defaults - US915
#define LORA_FREQUENCY_DEFAULT 915200000  // US915 channel 0 (915.2 MHz)
#define LORA_SF_DEFAULT 7                  // Spreading Factor 7 (fastest)
#define LORA_BW_DEFAULT 125.0              // Bandwidth 125 kHz
#define LORA_CR_DEFAULT 5                  // Coding Rate 4/5
#define LORA_SYNC_WORD_DEFAULT 0x34        // LoRaWAN public sync word
#define LORA_POWER_DEFAULT 14              // TX power in dBm

// I2C Clock Speed
#define I2C_CLOCK_SPEED 100000

// Gateway EUI (generated from WiFi MAC if not configured)
// Format: FFFE inserted in the middle of MAC address

// OLED Configuration (if available)
#ifndef OLED_ENABLED
#define OLED_ENABLED 0
#endif

#ifndef OLED_SDA
#define OLED_SDA 4
#endif

#ifndef OLED_SCL
#define OLED_SCL 15
#endif

#ifndef OLED_RST
#define OLED_RST 16
#endif

// LCD 16x2 I2C Configuration
#ifndef LCD_ENABLED
#define LCD_ENABLED 0
#endif

#ifndef LCD_SDA
#define LCD_SDA 21
#endif

#ifndef LCD_SCL
#define LCD_SCL 22
#endif

#ifndef LCD_ADDRESS
#define LCD_ADDRESS 0x27
#endif

#ifndef LCD_COLS
#define LCD_COLS 16
#endif

#ifndef LCD_ROWS
#define LCD_ROWS 2
#endif

// LoRa Pin Defaults (can be overridden by platformio.ini)
#ifndef LORA_MISO
#define LORA_MISO 19
#endif

#ifndef LORA_MOSI
#define LORA_MOSI 27
#endif

#ifndef LORA_SCK
#define LORA_SCK 5
#endif

#ifndef LORA_NSS
#define LORA_NSS 18
#endif

#ifndef LORA_RST
#define LORA_RST 14
#endif

#ifndef LORA_DIO0
#define LORA_DIO0 26
#endif

// Vext pin (Heltec boards - controls power to LoRa and OLED)
#ifndef VEXT_PIN
#define VEXT_PIN -1
#endif

// Semtech UDP Protocol versions
#define PROTOCOL_VERSION 2

// Packet types (Semtech UDP protocol)
#define PKT_PUSH_DATA 0x00
#define PKT_PUSH_ACK 0x01
#define PKT_PULL_DATA 0x02
#define PKT_PULL_RESP 0x03
#define PKT_PULL_ACK 0x04
#define PKT_TX_ACK 0x05

// Timing
#define STAT_INTERVAL 30000          // Statistics push interval (30s)
#define PULL_INTERVAL 10000          // PULL_DATA interval (10s)
#define KEEP_ALIVE_TIMEOUT 60000     // Keep alive timeout (60s)

// Buffer sizes
#define UDP_BUFFER_SIZE 2048
#define JSON_BUFFER_SIZE 4096

#endif // CONFIG_H
