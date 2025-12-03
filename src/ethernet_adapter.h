/**
 * @file ethernet_adapter.h
 * @brief Adaptador Ethernet via ATmega Bridge que implementa NetworkInterface
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Implementacao da interface de rede usando Ethernet W5500 via ATmega328P.
 * Toda comunicacao Ethernet passa pelo protocolo serial do ATmega Bridge.
 */

#ifndef ETHERNET_ADAPTER_H
#define ETHERNET_ADAPTER_H

#include "network_interface.h"
#include "atmega_bridge.h"

// Configuracao padrao Ethernet
#define ETH_DHCP_TIMEOUT_DEFAULT 10000   // 10 segundos para DHCP
#define ETH_LINK_CHECK_INTERVAL  2000    // Verificar link a cada 2s
#define ETH_DNS_TIMEOUT          5000    // Timeout DNS

/**
 * @struct EthernetConfig
 * @brief Configuracao da interface Ethernet
 */
struct EthernetConfig {
    bool enabled;
    bool useDHCP;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
    uint16_t dhcpTimeout;
};

/**
 * @class EthernetAdapter
 * @brief Implementacao de NetworkInterface usando Ethernet via ATmega
 */
class EthernetAdapter : public NetworkInterface {
public:
    /**
     * @brief Construtor
     * @param bridge Referencia para o ATmega Bridge
     */
    EthernetAdapter(ATmegaBridge& bridge);
    ~EthernetAdapter();

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
     * @brief Configurar para usar DHCP
     * @param timeout Timeout em ms
     */
    void setDHCP(uint16_t timeout = ETH_DHCP_TIMEOUT_DEFAULT);

    /**
     * @brief Configurar IP estatico
     * @param ip IP
     * @param gateway Gateway
     * @param subnet Mascara
     * @param dns DNS
     */
    void setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);

    /**
     * @brief Obter configuracao atual
     * @return Configuracao
     */
    EthernetConfig& getConfig() { return _config; }

    /**
     * @brief Forcar reconexao
     * @return true se sucesso
     */
    bool reconnect();

private:
    ATmegaBridge& _bridge;
    EthernetConfig _config;
    NetworkStatus _status;

    // Cache de estado
    IPAddress _localIP;
    IPAddress _gatewayIP;
    IPAddress _subnetMask;
    IPAddress _dnsIP;
    uint8_t _mac[6];

    // Estado UDP
    bool _udpStarted;
    uint16_t _udpLocalPort;

    // Buffer para pacote sendo construido
    uint8_t _txBuffer[512];
    size_t _txBufferLen;
    IPAddress _txDestIP;
    uint16_t _txDestPort;

    // Buffer para pacote recebido
    uint8_t _rxBuffer[512];
    size_t _rxBufferLen;
    size_t _rxBufferPos;
    IPAddress _rxRemoteIP;
    uint16_t _rxRemotePort;
    bool _rxPacketAvailable;

    // Timing
    uint32_t _connectedTime;
    uint32_t _lastLinkCheck;
    bool _lastLinkStatus;

    // Cache DNS simples
    char _dnsCache_host[64];
    IPAddress _dnsCache_ip;
    uint32_t _dnsCache_time;

    // Metodos internos
    bool initEthernet();
    void updateIPConfig();
    void checkLink();
    void generateMAC();  // Generate unique MAC from ESP32 WiFi MAC
};

#endif // ETHERNET_ADAPTER_H
