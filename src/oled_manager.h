#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

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

    // Update display
    void update();

    // Display control
    void setBrightness(uint8_t brightness);
    void displayOn();
    void displayOff();
    void clear();

private:
    Adafruit_SSD1306* display;
    TwoWire* wire;
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

    // Animation
    uint8_t animFrame;

    void drawHeader(const char* title);
    void drawProgressBar(int x, int y, int width, int value, int maxValue);
    void drawSignalStrength(int x, int y, int rssi);
};

// Global instance
extern OLEDManager oledManager;

#endif // OLED_MANAGER_H
