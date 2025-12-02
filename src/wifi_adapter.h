/**
 * @file wifi_adapter.h
 * @brief Adaptador WiFi que implementa NetworkInterface
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Implementacao da interface de rede usando WiFi nativo do ESP32.
 */

#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H

#include "network_interface.h"
#include <WiFi.h>
#include <WiFiUdp.h>

/**
 * @struct WiFiConfig
 * @brief Configuracao da interface WiFi (similar a EthernetConfig)
 */
struct WiFiConfig {
    bool useDHCP;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
};

/**
 * @class WiFiAdapter
 * @brief Implementacao de NetworkInterface usando WiFi ESP32
 */
class WiFiAdapter : public NetworkInterface {
public:
    WiFiAdapter();
    ~WiFiAdapter();

    // ================== Inicializacao ==================
    bool begin() override;
    void end() override;
    void update() override;

    // ================== Status ==================
    bool isConnected() override;
    bool isLinkUp() override;
    NetworkStatus getStatus() override;
    NetworkType getType() override;
    const char* getName() override;
    void getInfo(NetworkInfo& info) override;

    // ================== Configuracao IP ==================
    IPAddress localIP() override;
    IPAddress gatewayIP() override;
    void macAddress(uint8_t* mac) override;
    int8_t getRSSI() override;
    String getSSID() override;

    // ================== UDP ==================
    bool udpBegin(uint16_t port) override;
    void udpStop() override;
    bool udpBeginPacket(IPAddress ip, uint16_t port) override;
    bool udpBeginPacket(const char* host, uint16_t port) override;
    size_t udpWrite(const uint8_t* buffer, size_t size) override;
    bool udpEndPacket() override;
    int udpParsePacket() override;
    int udpRead(uint8_t* buffer, size_t maxSize) override;
    IPAddress udpRemoteIP() override;
    uint16_t udpRemotePort() override;

    // ================== DNS ==================
    bool hostByName(const char* host, IPAddress& result) override;

    // ================== Configuracao ==================

    /**
     * @brief Obter configuracao atual
     * @return Referencia para configuracao
     */
    WiFiConfig& getConfig() { return _config; }

    /**
     * @brief Aplicar configuracao de IP estatico
     * Deve ser chamado antes de conectar ao WiFi
     */
    void applyStaticIPConfig();

    /**
     * @brief Obter endereco MAC como string
     * @return String com MAC address
     */
    String getMacAddress();

private:
    WiFiUDP _udp;
    WiFiConfig _config;
    NetworkStatus _status;
    uint32_t _connectedTime;
    bool _udpStarted;
};

#endif // WIFI_ADAPTER_H
