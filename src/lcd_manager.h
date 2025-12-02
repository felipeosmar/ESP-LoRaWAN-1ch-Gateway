#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declaration
class NetworkManager;

// Failover notification duration (2 seconds)
#define LCD_FAILOVER_NOTIFICATION_DURATION_MS 2000

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

    // Network status display with interface indicator
    void showStatusWithNetwork(const char* gatewayEui, bool serverConnected, bool loraActive,
                               char networkIndicator, uint8_t hours, uint8_t minutes);

    // Failover notification (displays for 2 seconds then returns to normal)
    void showFailoverNotification(const char* fromIface, const char* toIface);

    // Update display (handles auto-rotation of display modes)
    void update();

    // Display control
    void backlight(bool on);
    void clear();

    // Configuration
    LCDConfig& getConfig() { return config; }
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();

    // Network manager reference for querying active interface
    void setNetworkManager(NetworkManager* nm) { _networkManager = nm; }

private:
    LCDConfig config;
    LiquidCrystal_I2C* lcd;
    bool available;
    NetworkManager* _networkManager;

    // Current display state
    enum DisplayMode {
        MODE_LOGO,
        MODE_STATUS,
        MODE_PACKET,
        MODE_STATS,
        MODE_WIFI,
        MODE_ERROR,
        MODE_FAILOVER_NOTIFICATION
    };

    DisplayMode currentMode;
    DisplayMode previousMode;  // Mode to return to after notification
    unsigned long lastUpdate;
    unsigned long modeStartTime;

    // Failover notification state
    char failoverFromIface[16];
    char failoverToIface[16];

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
    char getNetworkIndicator();
};

// Global instance
extern LCDManager lcdManager;

#endif // LCD_MANAGER_H
