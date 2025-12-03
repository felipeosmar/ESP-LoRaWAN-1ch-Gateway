/**
 * @file ethernet_adapter.cpp
 * @brief Implementacao do adaptador Ethernet via ATmega Bridge
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "ethernet_adapter.h"
#include <WiFi.h>  // Para obter MAC base do ESP32

EthernetAdapter::EthernetAdapter(ATmegaBridge& bridge)
    : _bridge(bridge)
    , _status(NetworkStatus::DISCONNECTED)
    , _udpStarted(false)
    , _udpLocalPort(0)
    , _txBufferLen(0)
    , _txDestPort(0)
    , _rxBufferLen(0)
    , _rxBufferPos(0)
    , _rxRemotePort(0)
    , _rxPacketAvailable(false)
    , _connectedTime(0)
    , _lastLinkCheck(0)
    , _lastLinkStatus(false)
    , _dnsCache_time(0)
{
    // Configuracao padrao: DHCP
    _config.enabled = true;
    _config.useDHCP = true;
    _config.staticIP = IPAddress(0, 0, 0, 0);
    _config.gateway = IPAddress(0, 0, 0, 0);
    _config.subnet = IPAddress(255, 255, 255, 0);
    _config.dns = IPAddress(8, 8, 8, 8);
    _config.dhcpTimeout = ETH_DHCP_TIMEOUT_DEFAULT;

    memset(_mac, 0, sizeof(_mac));
    memset(_dnsCache_host, 0, sizeof(_dnsCache_host));
}

EthernetAdapter::~EthernetAdapter() {
    end();
}

// ================== Inicializacao ==================

bool EthernetAdapter::begin() {
    if (!_config.enabled) {
        Serial.println("[ETH] Ethernet disabled");
        _status = NetworkStatus::DISCONNECTED;
        return false;
    }

    Serial.println("[ETH] Initializing Ethernet via ATmega...");

    // Verificar comunicacao com ATmega
    if (!_bridge.ping()) {
        Serial.println("[ETH] ATmega not responding");
        _status = NetworkStatus::ERROR;
        return false;
    }

    // Verificar link fisico
    if (!_bridge.ethLinkStatus()) {
        Serial.println("[ETH] No Ethernet cable connected");
        _status = NetworkStatus::LINK_DOWN;
        _lastLinkStatus = false;
        return false;
    }
    _lastLinkStatus = true;

    // Inicializar Ethernet
    return initEthernet();
}

bool EthernetAdapter::initEthernet() {
    _status = NetworkStatus::CONNECTING;
    bool success = false;

    // Step 1: Generate and set MAC address based on ESP32 WiFi MAC
    // This ensures a unique MAC for Ethernet derived from device identity
    generateMAC();
    if (!_bridge.ethSetMAC(_mac)) {
        Serial.println("[ETH] Warning: Failed to set MAC address");
    } else {
        Serial.printf("[ETH] MAC set: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
    }

    // Step 2: Initialize with IP configuration
    // NOTE: DHCP is NOT implemented in W5500 driver - using static IP
    if (_config.useDHCP) {
        Serial.println("[ETH] WARNING: DHCP not implemented in W5500 driver!");
        Serial.println("[ETH] Please configure static IP in web interface.");
        Serial.println("[ETH] Attempting initialization anyway...");

        // Even without DHCP, we need to initialize the chip
        // Try with static IP if configured, otherwise fail gracefully
        if (_config.staticIP != IPAddress(0, 0, 0, 0)) {
            Serial.printf("[ETH] Falling back to static IP: %s\n", _config.staticIP.toString().c_str());
            success = _bridge.ethInit(_config.staticIP, _config.gateway,
                                       _config.subnet, _config.dns);
        } else {
            // Just initialize W5500 without IP config
            success = _bridge.ethInit(_config.dhcpTimeout);
            if (success) {
                Serial.println("[ETH] W5500 initialized but no IP configured!");
                Serial.println("[ETH] Set static IP in config or web interface");
            }
        }
    } else {
        // Static IP configured - use it
        if (_config.staticIP == IPAddress(0, 0, 0, 0)) {
            Serial.println("[ETH] ERROR: Static IP mode but IP is 0.0.0.0!");
            Serial.println("[ETH] Please configure IP in web interface.");
            _status = NetworkStatus::ERROR;
            return false;
        }

        Serial.printf("[ETH] Using static IP: %s\n", _config.staticIP.toString().c_str());
        Serial.printf("[ETH] Gateway: %s, DNS: %s\n",
                      _config.gateway.toString().c_str(),
                      _config.dns.toString().c_str());
        success = _bridge.ethInit(_config.staticIP, _config.gateway,
                                   _config.subnet, _config.dns);
    }

    if (success) {
        // Obter configuracao IP
        updateIPConfig();

        if (_localIP != IPAddress(0, 0, 0, 0)) {
            _status = NetworkStatus::CONNECTED;
            _connectedTime = millis();
            Serial.printf("[ETH] Connected! IP: %s\n", _localIP.toString().c_str());
            return true;
        } else {
            Serial.println("[ETH] W5500 initialized but IP is 0.0.0.0");
            Serial.println("[ETH] Configure static IP for Ethernet to work");
        }
    }

    Serial.println("[ETH] Failed to initialize Ethernet");
    _status = NetworkStatus::ERROR;
    return false;
}

void EthernetAdapter::end() {
    if (_udpStarted) {
        udpStop();
    }
    _status = NetworkStatus::DISCONNECTED;
    _connectedTime = 0;
    _localIP = IPAddress(0, 0, 0, 0);
    Serial.println("[ETH] Ethernet stopped");
}

void EthernetAdapter::update() {
    if (!_config.enabled) return;

    // Verificar link periodicamente
    uint32_t now = millis();
    if (now - _lastLinkCheck >= ETH_LINK_CHECK_INTERVAL) {
        checkLink();
        _lastLinkCheck = now;
    }
}

void EthernetAdapter::checkLink() {
    bool linkUp = _bridge.ethLinkStatus();

    if (linkUp != _lastLinkStatus) {
        _lastLinkStatus = linkUp;

        if (linkUp) {
            Serial.println("[ETH] Link UP - cable connected");
            // Tentar reconectar se estava desconectado
            if (_status == NetworkStatus::LINK_DOWN ||
                _status == NetworkStatus::DISCONNECTED) {
                initEthernet();
            }
        } else {
            Serial.println("[ETH] Link DOWN - cable disconnected");
            _status = NetworkStatus::LINK_DOWN;
            _connectedTime = 0;
        }
    }
}

void EthernetAdapter::updateIPConfig() {
    _bridge.ethGetIP(_localIP, _gatewayIP, _subnetMask, _dnsIP);
    _bridge.ethGetMAC(_mac);
}

// ================== Status ==================

bool EthernetAdapter::isConnected() {
    return _status == NetworkStatus::CONNECTED &&
           _localIP != IPAddress(0, 0, 0, 0);
}

bool EthernetAdapter::isLinkUp() {
    return _lastLinkStatus;
}

NetworkStatus EthernetAdapter::getStatus() {
    return _status;
}

NetworkType EthernetAdapter::getType() {
    return NetworkType::ETHERNET;
}

const char* EthernetAdapter::getName() {
    return "Ethernet";
}

void EthernetAdapter::getInfo(NetworkInfo& info) {
    info.type = NetworkType::ETHERNET;
    info.status = _status;
    info.ip = _localIP;
    info.gateway = _gatewayIP;
    info.subnet = _subnetMask;
    info.dns = _dnsIP;
    memcpy(info.mac, _mac, 6);
    info.rssi = 0;  // N/A para Ethernet
    info.linkUp = _lastLinkStatus;
    info.connectedTime = _connectedTime > 0 ? millis() - _connectedTime : 0;
}

// ================== Configuracao IP ==================

IPAddress EthernetAdapter::localIP() {
    return _localIP;
}

IPAddress EthernetAdapter::gatewayIP() {
    return _gatewayIP;
}

void EthernetAdapter::macAddress(uint8_t* mac) {
    memcpy(mac, _mac, 6);
}

// ================== UDP ==================

bool EthernetAdapter::udpBegin(uint16_t port) {
    if (_status != NetworkStatus::CONNECTED) {
        Serial.println("[ETH] Cannot start UDP - not connected");
        return false;
    }

    if (_udpStarted) {
        _bridge.udpClose();
    }

    if (_bridge.udpBegin(port)) {
        _udpStarted = true;
        _udpLocalPort = port;
        Serial.printf("[ETH] UDP started on port %d\n", port);
        return true;
    }

    Serial.println("[ETH] Failed to start UDP");
    return false;
}

void EthernetAdapter::udpStop() {
    if (_udpStarted) {
        _bridge.udpClose();
        _udpStarted = false;
        _udpLocalPort = 0;
        Serial.println("[ETH] UDP stopped");
    }
}

bool EthernetAdapter::udpBeginPacket(IPAddress ip, uint16_t port) {
    if (!_udpStarted) return false;

    _txDestIP = ip;
    _txDestPort = port;
    _txBufferLen = 0;
    return true;
}

bool EthernetAdapter::udpBeginPacket(const char* host, uint16_t port) {
    IPAddress ip;
    if (!hostByName(host, ip)) {
        Serial.printf("[ETH] DNS resolution failed for: %s\n", host);
        return false;
    }
    return udpBeginPacket(ip, port);
}

size_t EthernetAdapter::udpWrite(const uint8_t* buffer, size_t size) {
    if (!_udpStarted) return 0;

    size_t remaining = sizeof(_txBuffer) - _txBufferLen;
    size_t toWrite = min(size, remaining);

    if (toWrite > 0) {
        memcpy(_txBuffer + _txBufferLen, buffer, toWrite);
        _txBufferLen += toWrite;
    }

    return toWrite;
}

bool EthernetAdapter::udpEndPacket() {
    if (!_udpStarted || _txBufferLen == 0) return false;

    bool result = _bridge.udpSend(_txDestIP, _txDestPort, _txBuffer, _txBufferLen);

    if (!result) {
        Serial.println("[ETH] Failed to send UDP packet");
    }

    _txBufferLen = 0;
    return result;
}

int EthernetAdapter::udpParsePacket() {
    if (!_udpStarted) return 0;

    // Se ja tem pacote pendente, retornar o tamanho
    if (_rxPacketAvailable && _rxBufferPos < _rxBufferLen) {
        return _rxBufferLen - _rxBufferPos;
    }

    // Verificar se ha dados disponiveis
    uint16_t available = _bridge.udpAvailable();
    if (available == 0) {
        _rxPacketAvailable = false;
        return 0;
    }

    // Receber pacote
    uint16_t received = 0;
    if (_bridge.udpReceive(_rxRemoteIP, _rxRemotePort, _rxBuffer, sizeof(_rxBuffer), received)) {
        _rxBufferLen = received;
        _rxBufferPos = 0;
        _rxPacketAvailable = true;
        return received;
    }

    _rxPacketAvailable = false;
    return 0;
}

int EthernetAdapter::udpRead(uint8_t* buffer, size_t maxSize) {
    if (!_rxPacketAvailable || _rxBufferPos >= _rxBufferLen) {
        return 0;
    }

    size_t remaining = _rxBufferLen - _rxBufferPos;
    size_t toRead = min(maxSize, remaining);

    memcpy(buffer, _rxBuffer + _rxBufferPos, toRead);
    _rxBufferPos += toRead;

    // Se leu tudo, limpar o pacote
    if (_rxBufferPos >= _rxBufferLen) {
        _rxPacketAvailable = false;
    }

    return toRead;
}

IPAddress EthernetAdapter::udpRemoteIP() {
    return _rxRemoteIP;
}

uint16_t EthernetAdapter::udpRemotePort() {
    return _rxRemotePort;
}

// ================== DNS ==================

bool EthernetAdapter::hostByName(const char* host, IPAddress& result) {
    // Verificar se eh IP literal
    if (result.fromString(host)) {
        return true;
    }

    // Verificar cache (valido por 5 minutos)
    if (strlen(_dnsCache_host) > 0 &&
        strcmp(_dnsCache_host, host) == 0 &&
        millis() - _dnsCache_time < 300000) {
        result = _dnsCache_ip;
        Serial.printf("[ETH] DNS cache hit: %s -> %s\n", host, result.toString().c_str());
        return true;
    }

    // Resolve via ATmega/W5500 native DNS
    Serial.printf("[ETH] Resolving DNS: %s\n", host);

    if (_bridge.dnsResolve(host, result)) {
        // Cache result (5-minute TTL)
        strlcpy(_dnsCache_host, host, sizeof(_dnsCache_host));
        _dnsCache_ip = result;
        _dnsCache_time = millis();

        Serial.printf("[ETH] DNS resolved: %s -> %s\n", host, result.toString().c_str());
        return true;
    }

    Serial.printf("[ETH] DNS resolution failed for: %s\n", host);
    return false;
}

// ================== Configuracao ==================

void EthernetAdapter::setDHCP(uint16_t timeout) {
    _config.useDHCP = true;
    _config.dhcpTimeout = timeout;
}

void EthernetAdapter::setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns) {
    _config.useDHCP = false;
    _config.staticIP = ip;
    _config.gateway = gateway;
    _config.subnet = subnet;
    _config.dns = dns;
}

bool EthernetAdapter::reconnect() {
    Serial.println("[ETH] Reconnecting...");
    end();
    delay(100);
    return begin();
}

// ================== MAC Address Generation ==================

void EthernetAdapter::generateMAC() {
    // Generate unique MAC address based on ESP32 WiFi MAC
    // This ensures each device has a unique Ethernet MAC
    //
    // Strategy: Use WiFi MAC as base, modify for Ethernet:
    // - WiFi MAC:     AA:BB:CC:DD:EE:FF
    // - Ethernet MAC: AA:BB:CC:DD:EE:FE (last byte XOR 0x01)
    //
    // This creates a locally administered unicast MAC address
    // Bit 1 of first byte = 0 (unicast)
    // Bit 2 of first byte = 1 (locally administered)

    uint8_t wifiMac[6];
    WiFi.macAddress(wifiMac);

    // Copy WiFi MAC as base
    memcpy(_mac, wifiMac, 6);

    // Set locally administered bit (bit 1 of first byte)
    // This indicates it's not a globally unique OUI
    _mac[0] |= 0x02;  // Set locally administered bit

    // Ensure unicast (clear multicast bit)
    _mac[0] &= 0xFE;  // Clear multicast bit

    // Differentiate from WiFi by modifying last byte
    _mac[5] ^= 0x01;  // XOR last byte to make it different

    Serial.printf("[ETH] Generated MAC from WiFi base: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
}
