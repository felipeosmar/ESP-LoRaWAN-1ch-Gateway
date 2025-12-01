/**
 * ESP32 Single Channel LoRaWAN Gateway
 *
 * A simple single-channel LoRaWAN gateway for ESP32 with SX1276/SX1278
 * Compatible with ChirpStack and other LoRaWAN Network Servers
 *
 * Features:
 * - Single channel LoRa packet reception
 * - Semtech UDP Packet Forwarder protocol
 * - Web interface for configuration
 * - OTA firmware updates
 * - OLED display support (Heltec, TTGO)
 *
 * Hardware Support:
 * - Heltec WiFi LoRa 32 V2
 * - TTGO LoRa32 V1
 * - Generic ESP32 + SX1276/SX1278
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <vector>

#include "config.h"
#include "lora_gateway.h"
#include "udp_forwarder.h"
#include "web_server.h"
#include "oled_manager.h"
#include "lcd_manager.h"
#include "ntp_manager.h"
#include "buzzer_manager.h"
#include "gps_manager.h"
#include "rtc_manager.h"
#include "atmega_bridge.h"
#include "network_manager.h"

// WiFi network structure
struct WiFiNetwork {
    String ssid;
    String password;
};

// WiFi configuration (global for web server access)
String wifiHostname = WIFI_HOSTNAME_DEFAULT;
String wifiSSID = WIFI_SSID_DEFAULT;           // Current connected SSID
String wifiPassword = WIFI_PASS_DEFAULT;       // Current connected password
bool wifiAPMode = WIFI_AP_MODE_DEFAULT;
bool wifiConnectedToInternet = false;

// Multiple WiFi networks support
std::vector<WiFiNetwork> wifiNetworks;

// Display update timing
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;

// Statistics update timing
unsigned long lastStatsUpdate = 0;
const unsigned long STATS_UPDATE_INTERVAL = 5000;

// Keep-alive LED timing
unsigned long lastLedBlink = 0;
bool ledState = false;

// ATmega Bridge and Network Manager instances
#if ATMEGA_ENABLED
ATmegaBridge atmegaBridge(Serial2, ATMEGA_RX_PIN, ATMEGA_TX_PIN);
#endif

// Function declarations
void setupWiFi();
bool loadConfig();
void setDefaultConfig();
void log(const String& msg);

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);

    // CRITICAL: Disable WiFi at boot to prevent auto-connect issues
    // This must be done BEFORE any other WiFi operations
    WiFi.persistent(false);           // Don't save credentials to NVS
    WiFi.setAutoConnect(false);       // Don't auto-connect on boot
    WiFi.setAutoReconnect(false);     // Don't auto-reconnect if disconnected
    WiFi.mode(WIFI_OFF);              // Turn off WiFi completely
    WiFi.disconnect(true, true);      // Disconnect and erase stored credentials
    delay(100);                       // Allow WiFi to stabilize

    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP32 Single Channel LoRaWAN Gateway");
    Serial.println("========================================");
    Serial.println();

    // Initialize Vext (Heltec boards - controls power to peripherals)
    #if VEXT_PIN >= 0
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);  // Enable Vext (active LOW)
    delay(100);
    Serial.println("[Main] Vext enabled");
    #endif

    // Initialize LittleFS
    Serial.println("[Main] Initializing LittleFS...");
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        Serial.println("[Main] LittleFS initialization failed!");
        Serial.println("[Main] System halted - filesystem required");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("[Main] LittleFS initialized");

    // Load configuration
    if (!loadConfig()) {
        Serial.println("[Main] Using default configuration");
        setDefaultConfig();
    }

    // Initialize OLED display
    #if OLED_ENABLED
    if (oledManager.begin()) {
        Serial.println("[Main] OLED display ready");
        oledManager.showLogo();
        delay(2000);
    }
    #endif

    // Initialize LCD display
    if (lcdManager.getConfig().enabled) {
        if (lcdManager.begin()) {
            Serial.println("[Main] LCD display ready");
            delay(2000);
        }
    }

    // Initialize buzzer
    #if BUZZER_ENABLED
    if (buzzer.begin()) {
        Serial.println("[Main] Buzzer initialized");
    }
    #endif

    // Initialize keep-alive LED
    #if LED_DEBUG_ENABLED
    pinMode(LED_DEBUG_PIN, OUTPUT);
    digitalWrite(LED_DEBUG_PIN, LOW);
    Serial.println("[Main] Keep-alive LED initialized on GPIO2");
    #endif

    // Initialize GPS
    #if GPS_ENABLED
    if (gpsManager.begin()) {
        Serial.println("[Main] GPS module initialized");
    }
    #endif

    // Initialize RTC (DS1307)
    #if RTC_ENABLED
    if (rtcManager.begin()) {
        Serial.println("[Main] RTC DS1307 initialized");
    }
    #endif

    // Initialize ATmega Bridge (for Ethernet W5500 and RTC via ATmega)
    #if ATMEGA_ENABLED
    Serial.println("[Main] Initializing ATmega Bridge...");
    if (atmegaBridge.begin(ATMEGA_BAUD_RATE)) {
        Serial.println("[Main] ATmega Bridge ready");

        // Get ATmega version
        uint8_t major, minor, patch;
        if (atmegaBridge.getVersion(major, minor, patch)) {
            Serial.printf("[Main] ATmega firmware: v%d.%d.%d\n", major, minor, patch);
        }

        // Create NetworkManager with bridge reference
        networkManager = new NetworkManager(atmegaBridge);
    } else {
        Serial.println("[Main] ATmega Bridge not responding - Ethernet disabled");
        // Create NetworkManager anyway but without functional Ethernet
        networkManager = new NetworkManager(atmegaBridge);
        networkManager->getConfig().ethernetEnabled = false;
    }
    #else
    // ATmega disabled - create dummy bridge for NetworkManager
    static ATmegaBridge dummyBridge(Serial2);
    networkManager = new NetworkManager(dummyBridge);
    networkManager->getConfig().ethernetEnabled = false;
    #endif

    // Setup WiFi
    setupWiFi();

    // Initialize NetworkManager (after WiFi is setup)
    if (networkManager) {
        Serial.println("[Main] Initializing Network Manager...");
        if (networkManager->begin()) {
            Serial.printf("[Main] Network Manager ready, active: %s\n",
                         networkManager->getActiveInterface() ?
                         networkManager->getActiveInterface()->getName() : "none");
        } else {
            Serial.println("[Main] Network Manager - no interfaces available");
        }
    }

    // Initialize LoRa gateway
    if (loraGateway.begin()) {
        Serial.println("[Main] LoRa radio initialized");

        // Start receiving
        if (loraGateway.startReceive()) {
            Serial.println("[Main] LoRa receiving started");
        }
    } else {
        Serial.println("[Main] LoRa initialization failed!");
        #if OLED_ENABLED
        oledManager.showError("LoRa Init Failed!");
        #endif
        if (lcdManager.isAvailable()) {
            lcdManager.showError("LoRa Init Failed!");
        }
    }

    // Initialize UDP forwarder (only if connected to internet)
    // Check both legacy WiFi connection and NetworkManager
    bool hasNetwork = wifiConnectedToInternet ||
                     (networkManager && networkManager->isConnected());

    if (hasNetwork) {
        if (udpForwarder.begin()) {
            Serial.println("[Main] UDP forwarder initialized");
        } else {
            Serial.println("[Main] UDP forwarder initialization failed!");
        }
    } else {
        Serial.println("[Main] UDP forwarder skipped (no internet)");
    }

    // Initialize web server
    webServer.begin();

    // Show initial status on display
    #if OLED_ENABLED
    if (oledManager.isAvailable()) {
        oledManager.showStatus(
            udpForwarder.getGatewayEuiString().c_str(),
            udpForwarder.isConnected() && wifiConnectedToInternet,
            loraGateway.isReceiving()
        );
    }
    #endif

    if (lcdManager.isAvailable()) {
        lcdManager.showStatus(
            udpForwarder.getGatewayEuiString().c_str(),
            udpForwarder.isConnected() && wifiConnectedToInternet,
            loraGateway.isReceiving()
        );
    }

    Serial.println();
    Serial.println("========================================");
    Serial.printf("  Gateway EUI: %s\n", udpForwarder.getGatewayEuiString().c_str());
    Serial.printf("  Web interface: http://%s/\n",
                  wifiAPMode ? WiFi.softAPIP().toString().c_str()
                             : WiFi.localIP().toString().c_str());
    Serial.printf("  mDNS hostname: http://%s.local/\n", wifiHostname.c_str());

    // Network Manager status
    if (networkManager) {
        Serial.println("  Network:");
        Serial.printf("    Active: %s\n",
                     networkManager->getActiveInterface() ?
                     networkManager->getActiveInterface()->getName() : "none");
        Serial.printf("    WiFi: %s\n",
                     networkManager->getWiFi()->isConnected() ? "Connected" : "Disconnected");
        #if ATMEGA_ENABLED
        Serial.printf("    Ethernet: %s\n",
                     networkManager->getEthernet()->isConnected() ? "Connected" :
                     (networkManager->getEthernet()->isLinkUp() ? "Link Up" : "No Cable"));
        #endif
        Serial.printf("    Failover: %s\n",
                     networkManager->getConfig().failoverEnabled ? "Enabled" : "Disabled");
    }

    Serial.println("========================================");
    Serial.println();
}

void loop() {
    // Update LoRa gateway (process interrupts)
    loraGateway.update();

    // Update buzzer (for non-blocking beeps)
    #if BUZZER_ENABLED
    buzzer.update();
    #endif

    // Update GPS
    #if GPS_ENABLED
    gpsManager.update();
    #endif

    // Update Network Manager (handles WiFi/Ethernet failover)
    if (networkManager) {
        networkManager->update();
    }

    // Keep-alive LED blink
    #if LED_DEBUG_ENABLED
    unsigned long now = millis();
    if (ledState) {
        // LED is ON, check if it's time to turn it off
        if (now - lastLedBlink >= LED_KEEPALIVE_ON_TIME) {
            digitalWrite(LED_DEBUG_PIN, LOW);
            ledState = false;
        }
    } else {
        // LED is OFF, check if it's time to blink
        if (now - lastLedBlink >= LED_KEEPALIVE_INTERVAL) {
            digitalWrite(LED_DEBUG_PIN, HIGH);
            ledState = true;
            lastLedBlink = now;
        }
    }
    #endif

    // Check for received packets
    while (loraGateway.hasPacket()) {
        LoRaPacket packet = loraGateway.getPacket();

        if (packet.valid) {
            Serial.printf("[Main] Packet received: %d bytes, RSSI: %.1f, SNR: %.1f\n",
                          packet.length, packet.rssi, packet.snr);

            // Play packet received sound
            #if BUZZER_ENABLED
            buzzer.playPacketRx();
            #endif

            // Show packet on display
            #if OLED_ENABLED
            if (oledManager.isAvailable()) {
                oledManager.showPacketInfo(
                    (int)packet.rssi,
                    packet.snr,
                    packet.length,
                    packet.frequency
                );
            }
            #endif

            if (lcdManager.isAvailable()) {
                lcdManager.showPacketInfo(
                    (int)packet.rssi,
                    packet.snr,
                    packet.length,
                    packet.frequency
                );
            }

            // Forward to network server
            bool networkAvailable = wifiConnectedToInternet ||
                                   (networkManager && networkManager->isConnected());
            if (networkAvailable && udpForwarder.isConnected()) {
                if (udpForwarder.forwardPacket(packet)) {
                    GatewayStats& stats = loraGateway.getStats();
                    stats.rxPacketsForwarded++;
                    Serial.println("[Main] Packet forwarded to server");
                } else {
                    Serial.println("[Main] Failed to forward packet");
                }
            }

            // Broadcast to WebSocket clients
            webServer.broadcastLog("Packet received: " + String(packet.length) +
                                   " bytes, RSSI: " + String(packet.rssi, 1) + " dBm");
        }
    }

    // Update UDP forwarder (send keep-alive, receive downlinks)
    bool hasNetworkConnection = wifiConnectedToInternet ||
                               (networkManager && networkManager->isConnected());
    if (hasNetworkConnection) {
        udpForwarder.update();
        ntpManager.update();
    }

    // Update RTC
    #if RTC_ENABLED
    rtcManager.update();
    #endif

    // Update web server
    webServer.loop();

    // Update OLED display periodically
    #if OLED_ENABLED
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = now;
        oledManager.update();
    }
    #endif

    // Update LCD display periodically
    if (lcdManager.isAvailable()) {
        unsigned long nowLcd = millis();
        if (nowLcd - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
            lastDisplayUpdate = nowLcd;
            lcdManager.update();
        }
    }

    // Periodic statistics display
    unsigned long now2 = millis();
    if (now2 - lastStatsUpdate >= STATS_UPDATE_INTERVAL) {
        lastStatsUpdate = now2;

        // Rotate display between status and stats every 10 seconds
        static uint8_t displayMode = 0;
        displayMode = (displayMode + 1) % 2;

        #if OLED_ENABLED
        if (oledManager.isAvailable()) {
            if (displayMode == 0) {
                oledManager.showStatus(
                    udpForwarder.getGatewayEuiString().c_str(),
                    udpForwarder.isConnected() && wifiConnectedToInternet,
                    loraGateway.isReceiving()
                );
            } else {
                GatewayStats& stats = loraGateway.getStats();
                oledManager.showStats(
                    stats.rxPacketsReceived,
                    stats.txPacketsSent,
                    stats.rxPacketsCrcError
                );
            }
        }
        #endif

        if (lcdManager.isAvailable()) {
            if (displayMode == 0) {
                lcdManager.showStatus(
                    udpForwarder.getGatewayEuiString().c_str(),
                    udpForwarder.isConnected() && wifiConnectedToInternet,
                    loraGateway.isReceiving()
                );
            } else {
                GatewayStats& stats = loraGateway.getStats();
                lcdManager.showStats(
                    stats.rxPacketsReceived,
                    stats.txPacketsSent,
                    stats.rxPacketsCrcError
                );
            }
        }
    }

    // Small delay to prevent watchdog issues
    delay(10);
}

void setupWiFi() {
    Serial.println("[WiFi] Setting up WiFi...");

    // Prevent flash wear and potential conflicts
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);

    // Ensure clean start - disconnect and erase any stored credentials
    WiFi.disconnect(true, true);  // disconnect=true, eraseap=true
    WiFi.mode(WIFI_OFF);
    delay(300);  // Delay for hardware stability

    if (wifiAPMode) {
        // Access Point mode
        Serial.println("[WiFi] Starting in AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(wifiSSID.c_str(), wifiPassword.c_str());
        Serial.printf("[WiFi] AP SSID: %s\n", wifiSSID.c_str());
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        wifiConnectedToInternet = false;

        // Initialize mDNS for .local domain resolution (AP mode)
        if (MDNS.begin(wifiHostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            Serial.printf("[WiFi] mDNS started: %s.local\n", wifiHostname.c_str());
        }

        #if OLED_ENABLED
        if (oledManager.isAvailable()) {
            oledManager.showWiFiInfo(wifiSSID.c_str(), 0, WiFi.softAPIP().toString().c_str());
        }
        #endif

        if (lcdManager.isAvailable()) {
            lcdManager.showWiFiInfo(wifiSSID.c_str(), 0, WiFi.softAPIP().toString().c_str());
        }
    } else {
        // Station mode - try multiple networks in sequence
        Serial.printf("[WiFi] %d network(s) configured\n", wifiNetworks.size());

        bool connected = false;

        // Try each configured network in sequence
        for (size_t i = 0; i < wifiNetworks.size() && !connected; i++) {
            WiFiNetwork& network = wifiNetworks[i];

            Serial.printf("[WiFi] Trying network %d/%d: %s",
                          i + 1, wifiNetworks.size(), network.ssid.c_str());

            #if OLED_ENABLED
            if (oledManager.isAvailable()) {
                char msg[64];
                snprintf(msg, sizeof(msg), "WiFi %d/%d: %s",
                         i + 1, wifiNetworks.size(), network.ssid.c_str());
                oledManager.showError(msg);
            }
            #endif

            if (lcdManager.isAvailable()) {
                char msg[17];
                snprintf(msg, sizeof(msg), "WiFi %d/%d", i + 1, wifiNetworks.size());
                lcdManager.showError(msg);
            }

            // Clean WiFi state before each attempt - critical for avoiding AUTH_FAIL
            WiFi.disconnect(true, true);  // Disconnect and erase credentials from NVS
            WiFi.mode(WIFI_OFF);
            delay(200);

            // Configure and start connection
            WiFi.mode(WIFI_STA);
            WiFi.setHostname(wifiHostname.c_str());
            WiFi.setSleep(false);
            delay(100);

            // Start connection
            WiFi.begin(network.ssid.c_str(), network.password.c_str());

            int attempts = 0;
            const int maxAttempts = 30;  // 15 seconds per network

            while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
                delay(500);
                Serial.print(".");
                attempts++;

                // Check for definitive failure states
                wl_status_t status = WiFi.status();
                if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED) {
                    break;
                }
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println();
                Serial.printf("[WiFi] Connected to %s\n", network.ssid.c_str());
                Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
                Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());

                // Update current SSID/password for web interface
                wifiSSID = network.ssid;
                wifiPassword = network.password;
                wifiConnectedToInternet = true;
                connected = true;

                // Initialize NTP time synchronization
                ntpManager.begin();

                // Sync RTC with NTP time after connection
                #if RTC_ENABLED
                if (rtcManager.isAvailable() && rtcManager.getConfig().syncWithNTP) {
                    delay(2000);  // Wait for NTP to sync
                    if (rtcManager.setTimeFromNTP()) {
                        Serial.println("[WiFi] RTC synchronized with NTP");
                    }
                }
                #endif

                // Initialize mDNS for .local domain resolution
                if (MDNS.begin(wifiHostname.c_str())) {
                    MDNS.addService("http", "tcp", 80);
                    Serial.printf("[WiFi] mDNS started: %s.local\n", wifiHostname.c_str());
                } else {
                    Serial.println("[WiFi] mDNS failed to start");
                }

                #if OLED_ENABLED
                if (oledManager.isAvailable()) {
                    oledManager.showWiFiInfo(network.ssid.c_str(), WiFi.RSSI(),
                                              WiFi.localIP().toString().c_str());
                }
                #endif

                if (lcdManager.isAvailable()) {
                    lcdManager.showWiFiInfo(network.ssid.c_str(), WiFi.RSSI(),
                                             WiFi.localIP().toString().c_str());
                }
            } else {
                Serial.printf(" Failed (status: %d)\n", WiFi.status());
            }
        }

        // If no network connected, switch to AP mode
        if (!connected) {
            Serial.println("[WiFi] All networks failed, switching to AP mode");

            WiFi.disconnect(true);
            delay(100);

            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
            Serial.printf("[WiFi] Fallback AP SSID: %s\n", WIFI_SSID_DEFAULT);
            Serial.printf("[WiFi] Fallback AP IP: %s\n", WiFi.softAPIP().toString().c_str());
            wifiConnectedToInternet = false;
            wifiAPMode = true;
            wifiSSID = WIFI_SSID_DEFAULT;
            wifiPassword = WIFI_PASS_DEFAULT;

            // Initialize mDNS for .local domain resolution (fallback AP)
            if (MDNS.begin(wifiHostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[WiFi] mDNS started: %s.local\n", wifiHostname.c_str());
            }

            #if OLED_ENABLED
            if (oledManager.isAvailable()) {
                oledManager.showError("WiFi Failed - AP Mode");
                delay(2000);
                oledManager.showWiFiInfo(WIFI_SSID_DEFAULT, 0,
                                          WiFi.softAPIP().toString().c_str());
            }
            #endif

            if (lcdManager.isAvailable()) {
                lcdManager.showError("WiFi Failed");
                delay(2000);
                lcdManager.showWiFiInfo(WIFI_SSID_DEFAULT, 0,
                                         WiFi.softAPIP().toString().c_str());
            }
        }
    }
}

bool loadConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[Config] Config file not found");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[Config] Parse error: %s\n", error.c_str());
        return false;
    }

    // Load WiFi configuration
    if (doc.containsKey("wifi")) {
        wifiAPMode = doc["wifi"]["ap_mode"] | WIFI_AP_MODE_DEFAULT;
        wifiHostname = doc["wifi"]["hostname"] | WIFI_HOSTNAME_DEFAULT;

        // Clear existing networks
        wifiNetworks.clear();

        // Load networks array
        if (doc["wifi"].containsKey("networks") && doc["wifi"]["networks"].is<JsonArray>()) {
            JsonArray networks = doc["wifi"]["networks"].as<JsonArray>();
            for (JsonObject network : networks) {
                if (wifiNetworks.size() >= WIFI_MAX_NETWORKS) break;

                WiFiNetwork wn;
                wn.ssid = network["ssid"] | "";
                wn.password = network["password"] | "";

                if (wn.ssid.length() > 0) {
                    wifiNetworks.push_back(wn);
                    Serial.printf("[Config] Loaded network: %s\n", wn.ssid.c_str());
                }
            }
        }

        // Set current SSID/password from first network
        if (!wifiNetworks.empty()) {
            wifiSSID = wifiNetworks[0].ssid;
            wifiPassword = wifiNetworks[0].password;
        } else {
            wifiSSID = WIFI_SSID_DEFAULT;
            wifiPassword = WIFI_PASS_DEFAULT;
        }

        Serial.printf("[Config] Total networks loaded: %d\n", wifiNetworks.size());
    }

    // Load LoRa configuration
    loraGateway.loadConfig(doc);

    // Load server configuration
    udpForwarder.loadConfig(doc);

    // Load NTP configuration
    ntpManager.loadConfig(doc);

    // Load LCD configuration
    lcdManager.loadConfig(doc);

    // Load buzzer configuration
    buzzer.loadConfig(doc);

    // Load GPS configuration
    gpsManager.loadConfig(doc);

    // Load RTC configuration
    rtcManager.loadConfig(doc);

    // Load Network Manager configuration
    if (networkManager) {
        networkManager->loadConfig(doc);
    }

    Serial.println("[Config] Configuration loaded");
    return true;
}

void setDefaultConfig() {
    wifiSSID = WIFI_SSID_DEFAULT;
    wifiPassword = WIFI_PASS_DEFAULT;
    wifiAPMode = WIFI_AP_MODE_DEFAULT;

    // Clear and set default network
    wifiNetworks.clear();
    WiFiNetwork defaultNetwork;
    defaultNetwork.ssid = WIFI_SSID_DEFAULT;
    defaultNetwork.password = WIFI_PASS_DEFAULT;
    wifiNetworks.push_back(defaultNetwork);

    // Create default config file
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    // WiFi defaults with networks array
    doc["wifi"]["hostname"] = WIFI_HOSTNAME_DEFAULT;
    doc["wifi"]["ap_mode"] = WIFI_AP_MODE_DEFAULT;
    JsonArray networks = doc["wifi"].createNestedArray("networks");
    JsonObject network = networks.createNestedObject();
    network["ssid"] = WIFI_SSID_DEFAULT;
    network["password"] = WIFI_PASS_DEFAULT;

    // LoRa defaults
    doc["lora"]["enabled"] = true;
    doc["lora"]["frequency"] = LORA_FREQUENCY_DEFAULT;
    doc["lora"]["spreading_factor"] = LORA_SF_DEFAULT;
    doc["lora"]["bandwidth"] = LORA_BW_DEFAULT;
    doc["lora"]["coding_rate"] = LORA_CR_DEFAULT;
    doc["lora"]["tx_power"] = LORA_POWER_DEFAULT;
    doc["lora"]["sync_word"] = LORA_SYNC_WORD_DEFAULT;

    // Server defaults
    doc["server"]["enabled"] = true;
    doc["server"]["host"] = NS_HOST_DEFAULT;
    doc["server"]["port_up"] = NS_PORT_UP_DEFAULT;
    doc["server"]["port_down"] = NS_PORT_DOWN_DEFAULT;
    doc["server"]["description"] = "ESP32 1ch Gateway";

    // NTP defaults
    doc["ntp"]["enabled"] = true;
    doc["ntp"]["server1"] = NTP_SERVER1_DEFAULT;
    doc["ntp"]["server2"] = NTP_SERVER2_DEFAULT;
    doc["ntp"]["timezone_offset"] = NTP_TIMEZONE_DEFAULT;
    doc["ntp"]["daylight_offset"] = NTP_DAYLIGHT_DEFAULT;
    doc["ntp"]["sync_interval"] = NTP_SYNC_INTERVAL_DEFAULT;

    // LCD defaults
    doc["lcd"]["enabled"] = LCD_ENABLED;
    doc["lcd"]["address"] = LCD_ADDRESS;
    doc["lcd"]["cols"] = LCD_COLS;
    doc["lcd"]["rows"] = LCD_ROWS;
    doc["lcd"]["sda"] = LCD_SDA;
    doc["lcd"]["scl"] = LCD_SCL;
    doc["lcd"]["backlight"] = true;
    doc["lcd"]["rotation_interval"] = 5;

    // RTC defaults
    doc["rtc"]["enabled"] = RTC_ENABLED;
    doc["rtc"]["i2cAddress"] = RTC_ADDRESS;
    doc["rtc"]["sdaPin"] = RTC_SDA;
    doc["rtc"]["sclPin"] = RTC_SCL;
    doc["rtc"]["syncWithNTP"] = RTC_SYNC_WITH_NTP_DEFAULT;
    doc["rtc"]["syncInterval"] = RTC_SYNC_INTERVAL_DEFAULT;
    doc["rtc"]["squareWaveMode"] = 0;
    doc["rtc"]["timezoneOffset"] = RTC_TIMEZONE_OFFSET_DEFAULT;

    // Network Manager defaults
    doc["network"]["wifi_enabled"] = NET_WIFI_ENABLED_DEFAULT;
    doc["network"]["ethernet_enabled"] = NET_ETHERNET_ENABLED_DEFAULT;
    doc["network"]["primary"] = NET_PRIMARY_WIFI_DEFAULT ? "wifi" : "ethernet";
    doc["network"]["failover_enabled"] = NET_FAILOVER_ENABLED_DEFAULT;
    doc["network"]["failover_timeout"] = NET_FAILOVER_TIMEOUT_DEFAULT;
    doc["network"]["reconnect_interval"] = NET_RECONNECT_INTERVAL_DEFAULT;

    // Ethernet defaults
    doc["network"]["ethernet"]["enabled"] = true;
    doc["network"]["ethernet"]["dhcp"] = ETH_DHCP_DEFAULT;
    doc["network"]["ethernet"]["static_ip"] = ETH_STATIC_IP_DEFAULT;
    doc["network"]["ethernet"]["gateway"] = ETH_GATEWAY_DEFAULT;
    doc["network"]["ethernet"]["subnet"] = ETH_SUBNET_DEFAULT;
    doc["network"]["ethernet"]["dns"] = ETH_DNS_DEFAULT;
    doc["network"]["ethernet"]["dhcp_timeout"] = ETH_DHCP_TIMEOUT_DEFAULT;

    // Save default config
    File file = LittleFS.open("/config.json", "w");
    if (file) {
        serializeJsonPretty(doc, file);
        file.close();
        Serial.println("[Config] Default configuration saved");
    }
}

void log(const String& msg) {
    Serial.println(msg);
    webServer.broadcastLog(msg);
}
