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

// Buzzer Configuration (GPIO26 on JVTECH v4.1)
#ifndef BUZZER_PIN
#define BUZZER_PIN 26
#endif

#ifndef BUZZER_ENABLED
#define BUZZER_ENABLED 1
#endif

// Debug LED Configuration (GPIO2 - built-in LED on most ESP32 boards)
#ifndef LED_DEBUG_PIN
#define LED_DEBUG_PIN 2
#endif

#ifndef LED_DEBUG_ENABLED
#define LED_DEBUG_ENABLED 1
#endif

#define LED_KEEPALIVE_INTERVAL 3000  // Blink every 3 seconds
#define LED_KEEPALIVE_ON_TIME  50    // LED on for 50ms

// GPS Configuration
#ifndef GPS_ENABLED
#define GPS_ENABLED 1
#endif

#ifndef GPS_RX_PIN
#define GPS_RX_PIN 16  // ESP32 RX from GPS TX
#endif

#ifndef GPS_TX_PIN
#define GPS_TX_PIN 17  // ESP32 TX to GPS RX
#endif

#ifndef GPS_BAUD_RATE
#define GPS_BAUD_RATE 9600
#endif

// GPS Power Control (GND via transistor on GPIO13 - HIGH to enable)
#ifndef GPS_ENABLE_PIN
#define GPS_ENABLE_PIN 13
#endif

// GPS Reset Pin (GPIO15 active LOW)
#ifndef GPS_RESET_PIN
#define GPS_RESET_PIN 15
#endif

// GPS defaults
#define GPS_ENABLED_DEFAULT     true
#define GPS_USE_FIXED_DEFAULT   false
#define GPS_LATITUDE_DEFAULT    0.0
#define GPS_LONGITUDE_DEFAULT   0.0
#define GPS_ALTITUDE_DEFAULT    0
#define GPS_UPDATE_INTERVAL     1000  // 1 second

// RTC DS1307 Configuration
#ifndef RTC_ENABLED
#define RTC_ENABLED 1
#endif

#ifndef RTC_ADDRESS
#define RTC_ADDRESS 0x68  // DS1307 default I2C address
#endif

#ifndef RTC_SDA
#define RTC_SDA 21        // Same as LCD (shared I2C bus)
#endif

#ifndef RTC_SCL
#define RTC_SCL 22        // Same as LCD (shared I2C bus)
#endif

// RTC defaults
#define RTC_SYNC_WITH_NTP_DEFAULT   true
#define RTC_SYNC_INTERVAL_DEFAULT   3600     // 1 hour in seconds
#define RTC_TIMEZONE_OFFSET_DEFAULT -3       // BRT (Brasilia Time)

// =============================================================================
// ATmega Bridge Configuration
// =============================================================================

// ATmega Bridge UART pins (ESP32 <-> ATmega328P)
#ifndef ATMEGA_RX_PIN
#define ATMEGA_RX_PIN 3   // ESP32 RX from ATmega TX
#endif

#ifndef ATMEGA_TX_PIN
#define ATMEGA_TX_PIN 1   // ESP32 TX to ATmega RX
#endif

#ifndef ATMEGA_BAUD_RATE
#define ATMEGA_BAUD_RATE 115200
#endif

#ifndef ATMEGA_ENABLED
#define ATMEGA_ENABLED 1
#endif

// =============================================================================
// Network Manager Configuration
// =============================================================================

// Network defaults
#define NET_WIFI_ENABLED_DEFAULT        true
#define NET_ETHERNET_ENABLED_DEFAULT    true
#define NET_PRIMARY_WIFI_DEFAULT        true     // true = WiFi primary, false = Ethernet primary
#define NET_FAILOVER_ENABLED_DEFAULT    true
#define NET_FAILOVER_TIMEOUT_DEFAULT    30000    // 30 seconds
#define NET_RECONNECT_INTERVAL_DEFAULT  10000    // 10 seconds

// Ethernet defaults (via ATmega)
#define ETH_DHCP_DEFAULT               true
#define ETH_DHCP_TIMEOUT_DEFAULT       10000    // 10 seconds
#define ETH_STATIC_IP_DEFAULT          "192.168.1.100"
#define ETH_GATEWAY_DEFAULT            "192.168.1.1"
#define ETH_SUBNET_DEFAULT             "255.255.255.0"
#define ETH_DNS_DEFAULT                "8.8.8.8"

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
