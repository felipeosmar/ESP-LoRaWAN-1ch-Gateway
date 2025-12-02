#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Forward declaration
class NetworkManager;

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

// Failover notification duration (2 seconds)
#define OLED_FAILOVER_NOTIFICATION_DURATION_MS 2000

class OLEDManager {
public:
    OLEDManager();

    bool begin();
    bool isAvailable() const { return available; }

    // Display modes
    void showLogo();
    void showStatus(const char* gatewayEui, bool serverConnected, bool loraActive);
    void showPacketInfo(int rssi, float snr, int size, uint32_t freq);
    void showStats(uint32_t rxPackets, uint32_t txPackets, uint32_t errors);
    void showWiFiInfo(const char* ssid, int rssi, const char* ip);
    void showError(const char* message);

    // Failover notification (displays for 2 seconds then returns to normal)
    void showFailoverNotification(const char* fromIface, const char* toIface);

    // Update display
    void update();

    // Display control
    void setBrightness(uint8_t brightness);
    void displayOn();
    void displayOff();
    void clear();

    // Network manager reference for querying active interface
    void setNetworkManager(NetworkManager* nm) { _networkManager = nm; }

private:
    Adafruit_SSD1306* display;
    TwoWire* wire;
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

    // Animation
    uint8_t animFrame;

    // Helper methods
    void drawHeader(const char* title);
    void drawHeaderWithNetwork(const char* title);
    void drawProgressBar(int x, int y, int width, int value, int maxValue);
    void drawSignalStrength(int x, int y, int rssi);
    void drawNetworkIndicator(int x, int y);
    char getNetworkIndicator();
    int8_t getWiFiRSSI();
};

// Global instance
extern OLEDManager oledManager;

#endif // OLED_MANAGER_H
