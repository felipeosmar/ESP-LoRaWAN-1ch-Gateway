#include "oled_manager.h"
#include "network_manager.h"
#include <WiFi.h>

// Global instance
OLEDManager oledManager;

OLEDManager::OLEDManager()
    : display(nullptr)
    , wire(nullptr)
    , available(false)
    , _networkManager(nullptr)
    , currentMode(MODE_LOGO)
    , previousMode(MODE_STATUS)
    , lastUpdate(0)
    , modeStartTime(0)
    , animFrame(0) {

    memset(&displayData, 0, sizeof(displayData));
    memset(failoverFromIface, 0, sizeof(failoverFromIface));
    memset(failoverToIface, 0, sizeof(failoverToIface));
}

bool OLEDManager::begin() {
#if OLED_ENABLED == 0
    Serial.println("[OLED] Display disabled in config");
    return false;
#else
    Serial.println("[OLED] Initializing display...");

    // Handle OLED reset pin if defined
    #if OLED_RST >= 0
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);
    delay(20);
    #endif

    // Initialize I2C
    wire = &Wire;
    wire->begin(OLED_SDA, OLED_SCL);

    // Create display instance
    display = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, wire, OLED_RST);

    // Initialize display
    if (!display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("[OLED] SSD1306 allocation failed");
        delete display;
        display = nullptr;
        return false;
    }

    available = true;
    Serial.println("[OLED] Display initialized");

    // Clear and show logo
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    showLogo();

    return true;
#endif
}

void OLEDManager::showLogo() {
    if (!available) return;

    currentMode = MODE_LOGO;
    modeStartTime = millis();

    display->clearDisplay();

    // Draw logo text
    display->setTextSize(2);
    display->setCursor(10, 8);
    display->print("LoRaWAN");

    display->setTextSize(1);
    display->setCursor(20, 30);
    display->print("1ch Gateway");

    display->setCursor(25, 45);
    display->print("ESP32 + SX1276");

    display->display();
}

void OLEDManager::showStatus(const char* gatewayEui, bool serverConnected, bool loraActive) {
    if (!available) return;

    currentMode = MODE_STATUS;
    strlcpy(displayData.gatewayEui, gatewayEui, sizeof(displayData.gatewayEui));
    displayData.serverConnected = serverConnected;
    displayData.loraActive = loraActive;

    display->clearDisplay();

    // Header with network indicator
    drawHeaderWithNetwork("Gateway Status");

    // EUI
    display->setTextSize(1);
    display->setCursor(0, 16);
    display->print("EUI:");
    display->setCursor(0, 26);
    display->setTextSize(1);

    // Show EUI in two parts for readability
    char euiPart[9];
    strncpy(euiPart, gatewayEui, 8);
    euiPart[8] = '\0';
    display->print(euiPart);
    display->print(" ");
    display->print(gatewayEui + 8);

    // Status indicators
    display->setCursor(0, 42);
    display->print("Server: ");
    display->print(serverConnected ? "Connected" : "Disconnected");

    display->setCursor(0, 54);
    display->print("LoRa: ");
    display->print(loraActive ? "Active" : "Inactive");

    // Status icons
    if (serverConnected) {
        display->fillCircle(120, 45, 3, SSD1306_WHITE);
    } else {
        display->drawCircle(120, 45, 3, SSD1306_WHITE);
    }

    if (loraActive) {
        display->fillCircle(120, 57, 3, SSD1306_WHITE);
    } else {
        display->drawCircle(120, 57, 3, SSD1306_WHITE);
    }

    display->display();
}

void OLEDManager::showFailoverNotification(const char* fromIface, const char* toIface) {
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

    display->clearDisplay();

    // Large "FAILOVER" text at top
    display->setTextSize(2);
    display->setCursor(10, 8);
    display->print("FAILOVER");

    // From -> To in the middle
    display->setTextSize(1);
    display->setCursor(20, 32);
    display->print(fromIface);
    display->print(" -> ");
    display->print(toIface);

    // Status line at bottom
    display->setCursor(10, 50);
    display->print("Switching network...");

    display->display();

    Serial.printf("[OLED] Failover notification: %s -> %s\n", fromIface, toIface);
}

void OLEDManager::showPacketInfo(int rssi, float snr, int size, uint32_t freq) {
    if (!available) return;

    currentMode = MODE_PACKET;
    modeStartTime = millis();
    displayData.lastRssi = rssi;
    displayData.lastSnr = snr;
    displayData.lastPacketSize = size;
    displayData.lastFreq = freq;

    display->clearDisplay();

    // Header with network indicator
    drawHeaderWithNetwork("Packet Received");

    display->setTextSize(1);

    // Frequency
    display->setCursor(0, 16);
    display->printf("Freq: %.2f MHz", freq / 1000000.0);

    // RSSI with bar
    display->setCursor(0, 28);
    display->printf("RSSI: %d dBm", rssi);
    drawSignalStrength(100, 28, rssi);

    // SNR
    display->setCursor(0, 40);
    display->printf("SNR: %.1f dB", snr);

    // Size
    display->setCursor(0, 52);
    display->printf("Size: %d bytes", size);

    display->display();
}

void OLEDManager::showStats(uint32_t rxPackets, uint32_t txPackets, uint32_t errors) {
    if (!available) return;

    currentMode = MODE_STATS;
    displayData.rxPackets = rxPackets;
    displayData.txPackets = txPackets;
    displayData.errors = errors;

    display->clearDisplay();

    // Header with network indicator
    drawHeaderWithNetwork("Statistics");

    display->setTextSize(1);

    // RX packets
    display->setCursor(0, 18);
    display->print("RX Packets: ");
    display->print(rxPackets);

    // TX packets
    display->setCursor(0, 30);
    display->print("TX Packets: ");
    display->print(txPackets);

    // Errors
    display->setCursor(0, 42);
    display->print("CRC Errors: ");
    display->print(errors);

    // Uptime
    unsigned long uptime = millis() / 1000;
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;

    display->setCursor(0, 54);
    display->printf("Uptime: %02lu:%02lu:%02lu", hours, minutes, seconds);

    display->display();
}

void OLEDManager::showWiFiInfo(const char* ssid, int rssi, const char* ip) {
    if (!available) return;

    currentMode = MODE_WIFI;
    strlcpy(displayData.ssid, ssid, sizeof(displayData.ssid));
    displayData.wifiRssi = rssi;
    strlcpy(displayData.ip, ip, sizeof(displayData.ip));

    display->clearDisplay();

    // Header with network indicator
    drawHeaderWithNetwork("WiFi Status");

    display->setTextSize(1);

    // SSID
    display->setCursor(0, 18);
    display->print("SSID: ");
    display->print(ssid);

    // Signal strength
    display->setCursor(0, 30);
    display->print("Signal: ");
    display->print(rssi);
    display->print(" dBm");
    drawSignalStrength(100, 30, rssi);

    // IP Address
    display->setCursor(0, 42);
    display->print("IP: ");
    display->print(ip);

    // MAC Address
    display->setCursor(0, 54);
    display->print("MAC: ");
    display->print(WiFi.macAddress().substring(0, 14));

    display->display();
}

void OLEDManager::showError(const char* message) {
    if (!available) return;

    currentMode = MODE_ERROR;
    modeStartTime = millis();
    strlcpy(displayData.errorMsg, message, sizeof(displayData.errorMsg));

    display->clearDisplay();

    // Error header
    display->setTextSize(2);
    display->setCursor(20, 10);
    display->print("ERROR!");

    // Error message
    display->setTextSize(1);
    display->setCursor(0, 35);

    // Word wrap for long messages
    String msg = message;
    int lineStart = 0;
    int y = 35;

    while (lineStart < (int)msg.length() && y < 60) {
        int lineEnd = min(lineStart + 21, (int)msg.length());
        display->setCursor(0, y);
        display->print(msg.substring(lineStart, lineEnd));
        lineStart = lineEnd;
        y += 10;
    }

    display->display();
}

void OLEDManager::update() {
    if (!available) return;

    // Animation frame counter
    animFrame++;

    // Auto-return from failover notification after 2 seconds
    if (currentMode == MODE_FAILOVER_NOTIFICATION) {
        if (millis() - modeStartTime > OLED_FAILOVER_NOTIFICATION_DURATION_MS) {
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

void OLEDManager::drawHeader(const char* title) {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print(title);

    // Underline
    display->drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void OLEDManager::drawHeaderWithNetwork(const char* title) {
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print(title);

    // Draw network indicator on the right side of header
    drawNetworkIndicator(108, 0);

    // Underline
    display->drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void OLEDManager::drawNetworkIndicator(int x, int y) {
    char indicator = getNetworkIndicator();

    // Draw indicator character in a box
    display->drawRect(x, y, 18, 9, SSD1306_WHITE);
    display->setCursor(x + 2, y + 1);

    if (indicator == 'W') {
        display->print("W");
        // When WiFi active, show signal bars
        int8_t rssi = getWiFiRSSI();
        if (rssi != 0) {
            // Draw mini signal bars next to W
            int bars = 0;
            if (rssi > -50) bars = 4;
            else if (rssi > -60) bars = 3;
            else if (rssi > -70) bars = 2;
            else if (rssi > -80) bars = 1;

            // Draw 4 small bars
            for (int i = 0; i < 4; i++) {
                int barHeight = (i + 1);
                int barY = y + 7 - barHeight;
                if (i < bars) {
                    display->fillRect(x + 9 + i * 2, barY, 1, barHeight, SSD1306_WHITE);
                } else {
                    display->drawPixel(x + 9 + i * 2, y + 6, SSD1306_WHITE);
                }
            }
        }
    } else if (indicator == 'E') {
        display->print("E");
        // For Ethernet, show a filled circle (connected indicator)
        display->fillCircle(x + 13, y + 4, 2, SSD1306_WHITE);
    } else {
        display->print("-");
        // No connection - show empty circle
        display->drawCircle(x + 13, y + 4, 2, SSD1306_WHITE);
    }
}

char OLEDManager::getNetworkIndicator() {
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

int8_t OLEDManager::getWiFiRSSI() {
    if (!_networkManager) {
        return 0;
    }

    // Only return RSSI when WiFi is the active interface
    if (_networkManager->getActiveType() == NetworkType::WIFI) {
        WiFiAdapter* wifi = _networkManager->getWiFi();
        if (wifi && wifi->isConnected()) {
            return wifi->getRSSI();
        }
    }
    return 0;
}

void OLEDManager::drawProgressBar(int x, int y, int width, int value, int maxValue) {
    int fillWidth = map(constrain(value, 0, maxValue), 0, maxValue, 0, width - 2);

    display->drawRect(x, y, width, 8, SSD1306_WHITE);
    display->fillRect(x + 1, y + 1, fillWidth, 6, SSD1306_WHITE);
}

void OLEDManager::drawSignalStrength(int x, int y, int rssi) {
    // Convert RSSI to 0-4 bars
    // -50 dBm = excellent (4 bars)
    // -60 dBm = good (3 bars)
    // -70 dBm = fair (2 bars)
    // -80 dBm = weak (1 bar)
    // <-80 dBm = very weak (0 bars)

    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;

    // Draw signal bars
    for (int i = 0; i < 4; i++) {
        int barHeight = (i + 1) * 2;
        int barY = y + 8 - barHeight;

        if (i < bars) {
            display->fillRect(x + i * 5, barY, 3, barHeight, SSD1306_WHITE);
        } else {
            display->drawRect(x + i * 5, barY, 3, barHeight, SSD1306_WHITE);
        }
    }
}

void OLEDManager::setBrightness(uint8_t brightness) {
    if (!available) return;

    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(brightness);
}

void OLEDManager::displayOn() {
    if (!available) return;
    display->ssd1306_command(SSD1306_DISPLAYON);
}

void OLEDManager::displayOff() {
    if (!available) return;
    display->ssd1306_command(SSD1306_DISPLAYOFF);
}

void OLEDManager::clear() {
    if (!available) return;
    display->clearDisplay();
    display->display();
}
