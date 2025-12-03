#include "lcd_manager.h"
#include "network_manager.h"
#include "i2c_bus.h"
#include <WiFi.h>
#include <LittleFS.h>

// Global instance
LCDManager lcdManager;

LCDManager::LCDManager()
    : lcd(nullptr)
    , available(false)
    , _networkManager(nullptr)
    , currentMode(MODE_LOGO)
    , previousMode(MODE_STATUS)
    , lastUpdate(0)
    , modeStartTime(0) {

    // Initialize config with defaults
    config.enabled = LCD_ENABLED;
    config.address = LCD_ADDRESS;
    config.cols = LCD_COLS;
    config.rows = LCD_ROWS;
    config.sda = LCD_SDA;
    config.scl = LCD_SCL;
    config.backlightOn = true;
    config.rotationInterval = 5;  // 5 seconds default

    memset(&displayData, 0, sizeof(displayData));
    memset(failoverFromIface, 0, sizeof(failoverFromIface));
    memset(failoverToIface, 0, sizeof(failoverToIface));
}

bool LCDManager::begin() {
    if (!config.enabled) {
        Serial.println("[LCD] Display disabled in config");
        return false;
    }

    Serial.println("[LCD] Initializing display...");

    // Check if I2C bus is initialized (should be done by main)
    if (!i2cBus.isInitialized()) {
        Serial.println("[LCD] Error: I2C bus not initialized!");
        return false;
    }

    // Check if LCD device is present on bus
    if (!i2cBus.devicePresent(config.address)) {
        Serial.printf("[LCD] Warning: Device not found at 0x%02X\n", config.address);
    }

    // Create LCD instance with configured parameters
    lcd = new LiquidCrystal_I2C(config.address, config.cols, config.rows);

    // Initialize LCD
    lcd->init();

    // Apply backlight setting
    if (config.backlightOn) {
        lcd->backlight();
    } else {
        lcd->noBacklight();
    }

    available = true;
    Serial.printf("[LCD] Display initialized (Address: 0x%02X, %dx%d, SDA=%d, SCL=%d)\n",
                  config.address, config.cols, config.rows, config.sda, config.scl);

    // Show logo
    showLogo();

    return true;
}

void LCDManager::showLogo() {
    if (!available) return;

    currentMode = MODE_LOGO;
    modeStartTime = millis();

    lcd->clear();
    printCentered(0, "LoRaWAN Gateway");
    printCentered(1, "ESP32 + SX1276");
}

void LCDManager::showStatus(const char* gatewayEui, bool serverConnected, bool loraActive) {
    if (!available) return;

    currentMode = MODE_STATUS;
    strlcpy(displayData.gatewayEui, gatewayEui, sizeof(displayData.gatewayEui));
    displayData.serverConnected = serverConnected;
    displayData.loraActive = loraActive;

    lcd->clear();

    // Line 1: Gateway title with network indicator and time
    // Format: "LORA GW  E 12:34" or "LORA GW  W 12:34"
    char line1[17];
    char indicator = getNetworkIndicator();

    // Get current time (simplified - just use uptime hours:minutes)
    unsigned long uptime = millis() / 1000;
    uint8_t hours = (uptime / 3600) % 24;
    uint8_t minutes = (uptime % 3600) / 60;

    snprintf(line1, sizeof(line1), "LORA GW  %c %02d:%02d", indicator, hours, minutes);
    lcd->setCursor(0, 0);
    lcd->print(line1);

    // Line 2: Server and LoRa status
    lcd->setCursor(0, 1);
    char line2[17];
    snprintf(line2, sizeof(line2), "S:%s L:%s",
             serverConnected ? "OK" : "--",
             loraActive ? "OK" : "--");
    lcd->print(line2);
}

void LCDManager::showStatusWithNetwork(const char* gatewayEui, bool serverConnected, bool loraActive,
                                        char networkIndicator, uint8_t hours, uint8_t minutes) {
    if (!available) return;

    currentMode = MODE_STATUS;
    strlcpy(displayData.gatewayEui, gatewayEui, sizeof(displayData.gatewayEui));
    displayData.serverConnected = serverConnected;
    displayData.loraActive = loraActive;

    lcd->clear();

    // Line 1: Gateway title with network indicator and time
    // Format: "LORA GW  E 12:34" or "LORA GW  W 12:34"
    char line1[17];
    snprintf(line1, sizeof(line1), "LORA GW  %c %02d:%02d", networkIndicator, hours, minutes);
    lcd->setCursor(0, 0);
    lcd->print(line1);

    // Line 2: Server and LoRa status
    lcd->setCursor(0, 1);
    char line2[17];
    snprintf(line2, sizeof(line2), "S:%s L:%s",
             serverConnected ? "OK" : "--",
             loraActive ? "OK" : "--");
    lcd->print(line2);
}

void LCDManager::showFailoverNotification(const char* fromIface, const char* toIface) {
    if (!available) return;

    // Save current mode to return to after notification
    if (currentMode != MODE_FAILOVER_NOTIFICATION) {
        previousMode = currentMode;
    }

    currentMode = MODE_FAILOVER_NOTIFICATION;
    modeStartTime = millis();

    // Store interface names
    strlcpy(failoverFromIface, fromIface, sizeof(failoverFromIface));
    strlcpy(failoverToIface, toIface, sizeof(failoverToIface));

    lcd->clear();

    // Line 1: "NET FAILOVER"
    printCentered(0, "NET FAILOVER");

    // Line 2: "WiFi -> Eth" or "Eth -> WiFi"
    char line2[17];
    snprintf(line2, sizeof(line2), "%s->%s", fromIface, toIface);
    printCentered(1, line2);

    Serial.printf("[LCD] Failover notification: %s -> %s\n", fromIface, toIface);
}

void LCDManager::showPacketInfo(int rssi, float snr, int size, uint32_t freq) {
    if (!available) return;

    currentMode = MODE_PACKET;
    modeStartTime = millis();
    displayData.lastRssi = rssi;
    displayData.lastSnr = snr;
    displayData.lastPacketSize = size;
    displayData.lastFreq = freq;

    lcd->clear();

    // Line 1: RSSI and SNR
    lcd->setCursor(0, 0);
    char line1[17];
    snprintf(line1, sizeof(line1), "RSSI:%d SNR:%.1f", rssi, snr);
    lcd->print(line1);

    // Line 2: Frequency and size
    lcd->setCursor(0, 1);
    char line2[17];
    snprintf(line2, sizeof(line2), "%.2fMHz %dB", freq / 1000000.0, size);
    lcd->print(line2);
}

void LCDManager::showStats(uint32_t rxPackets, uint32_t txPackets, uint32_t errors) {
    if (!available) return;

    currentMode = MODE_STATS;
    displayData.rxPackets = rxPackets;
    displayData.txPackets = txPackets;
    displayData.errors = errors;

    lcd->clear();

    // Line 1: RX and TX counts
    lcd->setCursor(0, 0);
    char line1[17];
    snprintf(line1, sizeof(line1), "RX:%-5lu TX:%-4lu", (unsigned long)rxPackets, (unsigned long)txPackets);
    lcd->print(line1);

    // Line 2: Errors and uptime
    lcd->setCursor(0, 1);
    String uptime = formatUptime(millis());
    char line2[17];
    snprintf(line2, sizeof(line2), "Err:%-3lu %s", (unsigned long)errors, uptime.c_str());
    lcd->print(line2);
}

void LCDManager::showWiFiInfo(const char* ssid, int rssi, const char* ip) {
    if (!available) return;

    currentMode = MODE_WIFI;
    strlcpy(displayData.ssid, ssid, sizeof(displayData.ssid));
    displayData.wifiRssi = rssi;
    strlcpy(displayData.ip, ip, sizeof(displayData.ip));

    lcd->clear();

    // Line 1: SSID (truncated if needed)
    lcd->setCursor(0, 0);
    char line1[17];
    if (strlen(ssid) > 16) {
        strncpy(line1, ssid, 15);
        line1[15] = '.';
        line1[16] = '\0';
    } else {
        strncpy(line1, ssid, 16);
        line1[16] = '\0';
    }
    lcd->print(line1);

    // Line 2: IP address
    lcd->setCursor(0, 1);
    lcd->print(ip);
}

void LCDManager::showError(const char* message) {
    if (!available) return;

    currentMode = MODE_ERROR;
    modeStartTime = millis();
    strlcpy(displayData.errorMsg, message, sizeof(displayData.errorMsg));

    lcd->clear();

    // Line 1: ERROR label
    printCentered(0, "! ERROR !");

    // Line 2: Error message (truncated if needed)
    lcd->setCursor(0, 1);
    char line2[17];
    strncpy(line2, message, 16);
    line2[16] = '\0';
    lcd->print(line2);
}

void LCDManager::update() {
    if (!available) return;

    // Auto-return from failover notification after 2 seconds
    if (currentMode == MODE_FAILOVER_NOTIFICATION) {
        if (millis() - modeStartTime > LCD_FAILOVER_NOTIFICATION_DURATION_MS) {
            // Return to previous mode
            showStatus(displayData.gatewayEui,
                       displayData.serverConnected,
                       displayData.loraActive);
        }
        return;  // Don't process other mode timeouts during notification
    }

    // Auto-return from packet info after 3 seconds
    if (currentMode == MODE_PACKET) {
        if (millis() - modeStartTime > 3000) {
            showStatus(displayData.gatewayEui,
                       displayData.serverConnected,
                       displayData.loraActive);
        }
    }

    // Auto-return from error after 5 seconds
    if (currentMode == MODE_ERROR) {
        if (millis() - modeStartTime > 5000) {
            showStatus(displayData.gatewayEui,
                       displayData.serverConnected,
                       displayData.loraActive);
        }
    }
}

char LCDManager::getNetworkIndicator() {
    if (!_networkManager) {
        return '-';
    }

    NetworkType activeType = _networkManager->getActiveType();

    switch (activeType) {
        case NetworkType::ETHERNET:
            return 'E';
        case NetworkType::WIFI:
            return 'W';
        default:
            return '-';
    }
}

void LCDManager::backlight(bool on) {
    if (!available) return;

    if (on) {
        lcd->backlight();
    } else {
        lcd->noBacklight();
    }
}

void LCDManager::clear() {
    if (!available) return;
    lcd->clear();
}

void LCDManager::printCentered(uint8_t row, const char* text) {
    if (!available) return;

    int len = strlen(text);
    int col = (LCD_COLS - len) / 2;
    if (col < 0) col = 0;

    lcd->setCursor(col, row);
    lcd->print(text);
}

String LCDManager::formatUptime(unsigned long ms) {
    unsigned long secs = ms / 1000;
    unsigned long hours = secs / 3600;
    unsigned long mins = (secs % 3600) / 60;

    char buf[10];
    if (hours > 0) {
        snprintf(buf, sizeof(buf), "%luh%02lum", hours, mins);
    } else {
        unsigned long s = secs % 60;
        snprintf(buf, sizeof(buf), "%lum%02lus", mins, s);
    }
    return String(buf);
}

void LCDManager::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("lcd")) {
        Serial.println("[LCD] No LCD config in JSON, using defaults");
        return;
    }

    JsonObjectConst lcdCfg = doc["lcd"];

    config.enabled = lcdCfg["enabled"] | config.enabled;
    config.address = lcdCfg["address"] | config.address;
    config.cols = lcdCfg["cols"] | config.cols;
    config.rows = lcdCfg["rows"] | config.rows;
    config.sda = lcdCfg["sda"] | config.sda;
    config.scl = lcdCfg["scl"] | config.scl;
    config.backlightOn = lcdCfg["backlight"] | config.backlightOn;
    config.rotationInterval = lcdCfg["rotation_interval"] | config.rotationInterval;

    Serial.printf("[LCD] Config loaded: enabled=%d, addr=0x%02X, %dx%d, SDA=%d, SCL=%d\n",
                  config.enabled, config.address, config.cols, config.rows,
                  config.sda, config.scl);
}

bool LCDManager::saveConfig() {
    // Read existing config
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[LCD] Cannot read config file");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[LCD] JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Update LCD section
    JsonObject lcdCfg = doc.createNestedObject("lcd");
    lcdCfg["enabled"] = config.enabled;
    lcdCfg["address"] = config.address;
    lcdCfg["cols"] = config.cols;
    lcdCfg["rows"] = config.rows;
    lcdCfg["sda"] = config.sda;
    lcdCfg["scl"] = config.scl;
    lcdCfg["backlight"] = config.backlightOn;
    lcdCfg["rotation_interval"] = config.rotationInterval;

    // Write back
    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[LCD] Cannot write config file");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println("[LCD] Config saved");
    return true;
}
