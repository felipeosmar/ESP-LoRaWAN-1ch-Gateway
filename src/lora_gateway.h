#ifndef LORA_GATEWAY_H
#define LORA_GATEWAY_H

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "config.h"

// Maximum packets to queue
#define MAX_PACKET_QUEUE 8
#define MAX_PACKET_SIZE 256

// LoRa packet structure
struct LoRaPacket {
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t length;
    float rssi;
    float snr;
    uint32_t frequency;
    uint8_t spreadingFactor;
    float bandwidth;
    uint8_t codingRate;
    uint32_t timestamp;  // Internal timestamp (microseconds)
    bool valid;
};

// Gateway statistics
struct GatewayStats {
    uint32_t rxPacketsReceived;
    uint32_t rxPacketsForwarded;
    uint32_t rxPacketsCrcError;
    uint32_t txPacketsSent;
    uint32_t txPacketsAcked;
    uint32_t txPacketsFailed;
    unsigned long lastPacketTime;
    float lastRssi;
    float lastSnr;
};

// Gateway configuration
struct GatewayConfig {
    bool enabled;
    uint32_t frequency;        // Frequency in Hz
    uint8_t spreadingFactor;   // 7-12
    float bandwidth;           // 125, 250, 500 kHz
    uint8_t codingRate;        // 5-8 (4/5 to 4/8)
    int8_t txPower;            // TX power in dBm
    uint8_t syncWord;          // LoRaWAN sync word (0x34)

    // Pin configuration
    int8_t pinMiso;
    int8_t pinMosi;
    int8_t pinSck;
    int8_t pinNss;
    int8_t pinRst;
    int8_t pinDio0;
};

class LoRaGateway {
public:
    LoRaGateway();

    // Initialization
    bool begin();
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();

    // Operation
    void update();
    bool startReceive();
    bool hasPacket();
    LoRaPacket getPacket();

    // Transmission (for downlinks)
    bool transmit(const uint8_t* data, size_t length, uint32_t frequency = 0,
                  uint8_t sf = 0, float bw = 0, uint8_t cr = 0);

    // Configuration
    GatewayConfig& getConfig() { return config; }
    GatewayStats& getStats() { return stats; }
    bool isAvailable() const { return available; }
    bool isReceiving() const { return receiving; }

    // Status
    String getStatusJson();
    void resetStats();

    // Interrupt handler
    static void onDio0Rise();

private:
    SX1276* radio;
    SPIClass* spi;
    GatewayConfig config;
    GatewayStats stats;

    bool available;
    bool receiving;

    // Packet queue (circular buffer)
    LoRaPacket packetQueue[MAX_PACKET_QUEUE];
    volatile uint8_t queueHead;
    volatile uint8_t queueTail;

    // Interrupt flag
    static volatile bool dio0Flag;

    // Internal methods
    bool initRadio();
    void processReceivedPacket();
    bool queuePacket(const LoRaPacket& packet);

    // Configuration helpers
    void setDefaultConfig();
    bool applyConfig();
};

// Global instance
extern LoRaGateway loraGateway;

#endif // LORA_GATEWAY_H
