#ifndef UDP_FORWARDER_H
#define UDP_FORWARDER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "config.h"
#include "lora_gateway.h"

// LoRaWAN Region IDs
#define REGION_EU868    "EU868"
#define REGION_US915    "US915"
#define REGION_AU915    "AU915"
#define REGION_AS923    "AS923"
#define REGION_KR920    "KR920"
#define REGION_IN865    "IN865"
#define REGION_RU864    "RU864"
#define REGION_DEFAULT  REGION_US915

// Forwarder configuration
struct ForwarderConfig {
    bool enabled;
    char serverHost[64];
    uint16_t serverPortUp;
    uint16_t serverPortDown;
    uint8_t gatewayEui[8];
    char description[64];
    char region[16];
    float latitude;
    float longitude;
    int16_t altitude;
};

// Forwarder statistics
struct ForwarderStats {
    uint32_t pushDataSent;
    uint32_t pushAckReceived;
    uint32_t pullDataSent;
    uint32_t pullAckReceived;
    uint32_t pullRespReceived;
    uint32_t txAckSent;
    uint32_t downlinksReceived;
    uint32_t downlinksSent;
    unsigned long lastPushTime;
    unsigned long lastPullTime;
    unsigned long lastAckTime;
};

class UDPForwarder {
public:
    UDPForwarder();

    // Initialization
    bool begin();
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();

    // Operation
    void update();
    bool forwardPacket(const LoRaPacket& packet);

    // Configuration
    ForwarderConfig& getConfig() { return config; }
    ForwarderStats& getStats() { return stats; }
    bool isConnected() const { return connected; }

    // Status
    String getStatusJson();
    void resetStats();
    String getGatewayEuiString();

private:
    WiFiUDP udp;
    ForwarderConfig config;
    ForwarderStats stats;

    bool connected;
    uint16_t tokenCounter;

    // Timing
    unsigned long lastStatTime;
    unsigned long lastPullTime;

    // Buffer for UDP packets
    uint8_t udpBuffer[UDP_BUFFER_SIZE];

    // Internal methods
    void setDefaultConfig();
    void generateGatewayEui();

    // Semtech protocol methods
    bool sendPushData(const char* jsonData, size_t length);
    bool sendPullData();
    bool sendTxAck(uint16_t token, const char* error = nullptr);
    void sendStatistics();

    void receivePackets();
    void handlePullResp(const uint8_t* data, size_t length, uint16_t token);
    void processTxPacket(const JsonDocument& txpk);

    // Helper methods
    String buildRxpkJson(const LoRaPacket& packet);
    String buildStatJson();
    uint16_t getNextToken();

    // Base64 encoding for payload
    static const char base64Chars[];
    String base64Encode(const uint8_t* data, size_t length);
    size_t base64Decode(const char* encoded, uint8_t* output, size_t maxLen);

    // Timestamp helpers
    uint32_t getCompactTimestamp();
    String getIsoTimestamp();
};

// Global instance
extern UDPForwarder udpForwarder;

#endif // UDP_FORWARDER_H
