#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "config.h"

// LCD Configuration structure
struct LCDConfig {
    bool enabled;
    uint8_t address;
    uint8_t cols;
    uint8_t rows;
    uint8_t sda;
    uint8_t scl;
    bool backlightOn;
    uint8_t rotationInterval;  // seconds between display mode changes
};

class LCDManager {
public:
    LCDManager();

    bool begin();
    bool isAvailable() const { return available; }

    // Display modes
    void showLogo();
    void showStatus(const char* gatewayEui, bool serverConnected, bool loraActive);
    void showPacketInfo(int rssi, float snr, int size, uint32_t freq);
    void showStats(uint32_t rxPackets, uint32_t txPackets, uint32_t errors);
    void showWiFiInfo(const char* ssid, int rssi, const char* ip);
    void showError(const char* message);

    // Update display (handles auto-rotation of display modes)
    void update();

    // Display control
    void backlight(bool on);
    void clear();

    // Configuration
    LCDConfig& getConfig() { return config; }
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();

private:
    LCDConfig config;
    LiquidCrystal_I2C* lcd;
    bool available;

    // Current display state
    enum DisplayMode {
        MODE_LOGO,
        MODE_STATUS,
        MODE_PACKET,
        MODE_STATS,
        MODE_WIFI,
        MODE_ERROR
    };

    DisplayMode currentMode;
    unsigned long lastUpdate;
    unsigned long modeStartTime;

    // Cached data for display
    struct {
        char gatewayEui[24];
        bool serverConnected;
        bool loraActive;
        int lastRssi;
        float lastSnr;
        int lastPacketSize;
        uint32_t lastFreq;
        uint32_t rxPackets;
        uint32_t txPackets;
        uint32_t errors;
        char ssid[32];
        int wifiRssi;
        char ip[16];
        char errorMsg[32];
    } displayData;

    // Helper methods
    void printCentered(uint8_t row, const char* text);
    String formatUptime(unsigned long millis);
};

// Global instance
extern LCDManager lcdManager;

#endif // LCD_MANAGER_H
