/**
 * @file network_manager.cpp
 * @brief Implementacao do gerenciador de rede com failover
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "network_manager.h"
#include "udp_forwarder.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Instancia global
NetworkManager* networkManager = nullptr;

NetworkManager::NetworkManager(ATmegaBridge& bridge)
    : _bridge(bridge)
    , _wifi()
    , _ethernet(bridge)
    , _udpForwarder(nullptr)
    , _failoverCallback(nullptr)
    , _activeInterface(nullptr)
    , _manualMode(false)
    , _manualType(NetworkType::NONE)
    , _wifiWasConnected(false)
    , _ethernetWasConnected(false)
    , _lastStatusCheck(0)
    , _primaryDownSince(0)
    , _lastReconnectAttempt(0)
    , _failoverActive(false)
    , _primaryStableStart(0)
    , _udpPort(0)
    , _udpStarted(false)
{
    // Configuracao padrao - Ethernet como interface primaria
    _config.wifiEnabled = true;
    _config.ethernetEnabled = true;
    _config.primary = PrimaryInterface::ETHERNET;
    _config.failoverEnabled = true;
    _config.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;
    _config.reconnectInterval = NET_RECONNECT_INTERVAL_DEFAULT;
    _config.healthCheckEnabled = true;
    _config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;

    // Zerar estatisticas
    memset(&_stats, 0, sizeof(_stats));
}

NetworkManager::~NetworkManager() {
    udpStop();
}

// ================== Inicializacao ==================

bool NetworkManager::begin() {
    Serial.println("[NET] Initializing Network Manager...");
    Serial.printf("[NET] Primary: %s, Failover: %s, Timeout: %dms\n",
                  _config.primary == PrimaryInterface::WIFI ? "WiFi" : "Ethernet",
                  _config.failoverEnabled ? "ON" : "OFF",
                  _config.failoverTimeout);
    Serial.printf("[NET] Health check: %s, Stability period: %dms\n",
                  _config.healthCheckEnabled ? "ON" : "OFF",
                  _config.stabilityPeriod);

    bool anyConnected = false;

    // Inicializar WiFi adapter (WiFi ja deve estar conectado pelo main.cpp)
    if (_config.wifiEnabled) {
        if (_wifi.begin()) {
            Serial.println("[NET] WiFi adapter ready");
            anyConnected = true;
        }
    }

    // Inicializar Ethernet adapter
    if (_config.ethernetEnabled) {
        if (_ethernet.begin()) {
            Serial.println("[NET] Ethernet adapter ready");
            anyConnected = true;
        } else {
            Serial.println("[NET] Ethernet not available");
        }
    }

    // Selecionar interface ativa inicial
    NetworkInterface* primary = getPrimaryInterface();
    if (primary && primary->isConnected()) {
        _activeInterface = primary;
        Serial.printf("[NET] Active interface: %s\n", primary->getName());
    } else {
        // Tentar secundaria
        NetworkInterface* secondary = getSecondaryInterface();
        if (secondary && secondary->isConnected()) {
            _activeInterface = secondary;
            _failoverActive = true;
            Serial.printf("[NET] Failover active, using: %s\n", secondary->getName());
        }
    }

    if (_activeInterface) {
        Serial.printf("[NET] Connected via %s, IP: %s\n",
                      _activeInterface->getName(),
                      _activeInterface->localIP().toString().c_str());
    } else {
        Serial.println("[NET] No network connection available");
    }

    return anyConnected;
}

void NetworkManager::update() {
    uint32_t now = millis();

    // Verificar estado periodicamente
    if (now - _lastStatusCheck >= NET_STATUS_CHECK_INTERVAL) {
        _lastStatusCheck = now;

        updateInterfaces();
        updateStats();

        if (!_manualMode) {
            checkFailover();
        }
    }
}

void NetworkManager::updateInterfaces() {
    // Atualizar estado das interfaces
    if (_config.wifiEnabled) {
        _wifi.update();

        bool wifiConnected = _wifi.isConnected();
        if (wifiConnected && !_wifiWasConnected) {
            _stats.wifiConnections++;
            Serial.println("[NET] WiFi connected");
        } else if (!wifiConnected && _wifiWasConnected) {
            _stats.wifiDisconnections++;
            Serial.println("[NET] WiFi disconnected");
        }
        _wifiWasConnected = wifiConnected;
    }

    if (_config.ethernetEnabled) {
        _ethernet.update();

        bool ethConnected = _ethernet.isConnected();
        if (ethConnected && !_ethernetWasConnected) {
            _stats.ethernetConnections++;
            Serial.println("[NET] Ethernet connected");
        } else if (!ethConnected && _ethernetWasConnected) {
            _stats.ethernetDisconnections++;
            Serial.println("[NET] Ethernet disconnected");
        }
        _ethernetWasConnected = ethConnected;
    }
}

bool NetworkManager::isApplicationHealthy() {
    // If health check is disabled, always return true (rely on IP connectivity)
    if (!_config.healthCheckEnabled) {
        return true;
    }

    // If no UDPForwarder reference, fall back to IP connectivity
    if (!_udpForwarder) {
        return true;
    }

    // Query UDPForwarder for health status
    return _udpForwarder->isHealthy(_config.failoverTimeout);
}

void NetworkManager::checkFailover() {
    if (!_config.failoverEnabled) return;

    NetworkInterface* primary = getPrimaryInterface();
    NetworkInterface* secondary = getSecondaryInterface();
    uint32_t now = millis();

    bool primaryConnected = primary && primary->isConnected();
    bool secondaryConnected = secondary && secondary->isConnected();

    // Check application-layer health when health check is enabled
    bool applicationHealthy = isApplicationHealthy();

    // Se interface ativa caiu (link down)
    if (_activeInterface && !_activeInterface->isConnected()) {
        const char* fromName = _activeInterface->getName();
        Serial.printf("[NET] Active interface %s lost connection\n", fromName);

        // Tentar outra interface imediatamente
        if (_activeInterface == primary && secondaryConnected) {
            const char* toName = secondary->getName();
            notifyFailover(fromName, toName);
            switchToInterface(secondary);
            _failoverActive = true;
            _stats.failoverCount++;
            _stats.lastFailoverTime = now;
            _primaryStableStart = 0;
            Serial.printf("[NET] Failover to %s (link down)\n", toName);
        } else if (_activeInterface == secondary && primaryConnected) {
            const char* toName = primary->getName();
            notifyFailover(fromName, toName);
            switchToInterface(primary);
            _failoverActive = false;
            _primaryStableStart = 0;
            Serial.printf("[NET] Restored to primary %s\n", toName);
        } else {
            // Nenhuma interface disponivel
            _activeInterface = nullptr;
            _primaryStableStart = 0;
            Serial.println("[NET] No network available");
        }
        return;
    }

    // Check application-layer health for failover trigger
    if (_config.healthCheckEnabled && _activeInterface == primary && !applicationHealthy) {
        // Primary interface link is up but ChirpStack is not responding
        Serial.printf("[NET] Health check failed on %s (no ACK within %dms)\n",
                      primary->getName(), _config.failoverTimeout);

        if (secondaryConnected) {
            notifyFailover(primary->getName(), secondary->getName());
            switchToInterface(secondary);
            _failoverActive = true;
            _stats.failoverCount++;
            _stats.lastFailoverTime = now;
            _primaryStableStart = 0;
            Serial.printf("[NET] Failover to %s (health check failed)\n", secondary->getName());
        }
        return;
    }

    // Se esta em failover e primaria voltou
    if (_failoverActive && primaryConnected) {
        // Use configurable stability period instead of hardcoded 5s
        // Check if primary is healthy (both link and application layer)
        bool primaryHealthy = primaryConnected;

        // For return-to-primary, we could optionally check application health
        // but for now, rely on link status since we are on secondary
        // and cannot easily test primary's application health without switching

        if (primaryHealthy) {
            if (_primaryStableStart == 0) {
                _primaryStableStart = now;
                Serial.printf("[NET] Primary %s connected, waiting stability period (%dms)\n",
                              primary->getName(), _config.stabilityPeriod);
            } else if (now - _primaryStableStart >= _config.stabilityPeriod) {
                // Voltar para primaria after stability period
                notifyFailover(secondary->getName(), primary->getName());
                switchToInterface(primary);
                _failoverActive = false;
                _primaryStableStart = 0;
                Serial.printf("[NET] Restored to primary %s after %dms stability period\n",
                              primary->getName(), _config.stabilityPeriod);
            }
        } else {
            // Primary became unhealthy during stability period, reset timer
            if (_primaryStableStart != 0) {
                Serial.printf("[NET] Primary %s unstable, resetting stability timer\n",
                              primary->getName());
                _primaryStableStart = 0;
            }
        }
    } else {
        // Reset stability timer when not in failover or primary not connected
        _primaryStableStart = 0;
    }

    // Se nao tem interface ativa, tentar conectar
    if (!_activeInterface) {
        if (now - _lastReconnectAttempt >= _config.reconnectInterval) {
            _lastReconnectAttempt = now;

            if (primaryConnected) {
                switchToInterface(primary);
            } else if (secondaryConnected) {
                switchToInterface(secondary);
                _failoverActive = true;
            }
        }
    }
}

void NetworkManager::switchToInterface(NetworkInterface* iface) {
    if (!iface) return;

    bool needRestartUDP = _udpStarted;

    // Parar UDP na interface antiga
    if (_activeInterface && _udpStarted) {
        _activeInterface->udpStop();
    }

    _activeInterface = iface;

    // Reiniciar UDP na nova interface
    if (needRestartUDP) {
        startUDP();
    }

    Serial.printf("[NET] Switched to %s, IP: %s\n",
                  iface->getName(), iface->localIP().toString().c_str());
}

void NetworkManager::notifyFailover(const char* fromIface, const char* toIface) {
    // Call the registered callback to notify displays of the failover
    if (_failoverCallback) {
        _failoverCallback(fromIface, toIface);
    }
}

NetworkInterface* NetworkManager::getPrimaryInterface() {
    if (_config.primary == PrimaryInterface::WIFI) {
        return _config.wifiEnabled ? &_wifi : nullptr;
    } else {
        return _config.ethernetEnabled ? &_ethernet : nullptr;
    }
}

NetworkInterface* NetworkManager::getSecondaryInterface() {
    if (_config.primary == PrimaryInterface::WIFI) {
        return _config.ethernetEnabled ? &_ethernet : nullptr;
    } else {
        return _config.wifiEnabled ? &_wifi : nullptr;
    }
}

void NetworkManager::updateStats() {
    // Atualizar tempo de uptime por interface
    if (_activeInterface) {
        if (_activeInterface->getType() == NetworkType::WIFI) {
            _stats.totalUptimeWifi += NET_STATUS_CHECK_INTERVAL;
        } else if (_activeInterface->getType() == NetworkType::ETHERNET) {
            _stats.totalUptimeEthernet += NET_STATUS_CHECK_INTERVAL;
        }
    }
}

// ================== Configuracao ==================

void NetworkManager::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("network")) {
        Serial.println("[NET] No network config in JSON, using defaults");
        return;
    }

    JsonObjectConst network = doc["network"];

    _config.wifiEnabled = network["wifi_enabled"] | true;
    _config.ethernetEnabled = network["ethernet_enabled"] | true;

    // Default to "ethernet" when primary field is missing (backwards compatibility)
    const char* primary = network["primary"] | "ethernet";
    if (strcmp(primary, "wifi") == 0) {
        _config.primary = PrimaryInterface::WIFI;
    } else {
        _config.primary = PrimaryInterface::ETHERNET;
    }

    _config.failoverEnabled = network["failover_enabled"] | true;
    _config.failoverTimeout = network["failover_timeout"] | NET_FAILOVER_TIMEOUT_DEFAULT;
    _config.reconnectInterval = network["reconnect_interval"] | NET_RECONNECT_INTERVAL_DEFAULT;
    _config.healthCheckEnabled = network["health_check_enabled"] | true;
    _config.stabilityPeriod = network["stability_period"] | NET_STABILITY_PERIOD_DEFAULT;

    // Configuracao Ethernet
    if (network.containsKey("ethernet")) {
        JsonObjectConst eth = network["ethernet"];

        EthernetConfig& ethConfig = _ethernet.getConfig();
        ethConfig.enabled = eth["enabled"] | true;
        ethConfig.useDHCP = eth["dhcp"] | true;

        if (eth.containsKey("static_ip")) {
            ethConfig.staticIP.fromString(eth["static_ip"].as<const char*>());
        }
        if (eth.containsKey("gateway")) {
            ethConfig.gateway.fromString(eth["gateway"].as<const char*>());
        }
        if (eth.containsKey("subnet")) {
            ethConfig.subnet.fromString(eth["subnet"].as<const char*>());
        }
        if (eth.containsKey("dns")) {
            ethConfig.dns.fromString(eth["dns"].as<const char*>());
        }
        ethConfig.dhcpTimeout = eth["dhcp_timeout"] | ETH_DHCP_TIMEOUT_DEFAULT;
    }

    // Configuracao WiFi (static IP settings)
    if (network.containsKey("wifi")) {
        JsonObjectConst wifi = network["wifi"];

        WiFiConfig& wifiConfig = _wifi.getConfig();
        wifiConfig.useDHCP = wifi["dhcp"] | true;

        if (wifi.containsKey("static_ip")) {
            wifiConfig.staticIP.fromString(wifi["static_ip"].as<const char*>());
        }
        if (wifi.containsKey("gateway")) {
            wifiConfig.gateway.fromString(wifi["gateway"].as<const char*>());
        }
        if (wifi.containsKey("subnet")) {
            wifiConfig.subnet.fromString(wifi["subnet"].as<const char*>());
        }
        if (wifi.containsKey("dns")) {
            wifiConfig.dns.fromString(wifi["dns"].as<const char*>());
        }

        // Apply static IP configuration if DHCP is disabled
        if (!wifiConfig.useDHCP) {
            _wifi.applyStaticIPConfig();
        }
    }

    Serial.printf("[NET] Config loaded: primary=%s, failover=%s, health_check=%s\n",
                  _config.primary == PrimaryInterface::WIFI ? "WiFi" : "Ethernet",
                  _config.failoverEnabled ? "ON" : "OFF",
                  _config.healthCheckEnabled ? "ON" : "OFF");
}

bool NetworkManager::saveConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[NET] Cannot open config for reading");
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("[NET] Failed to parse config");
        return false;
    }

    // Atualizar secao network
    JsonObject network = doc.createNestedObject("network");
    network["wifi_enabled"] = _config.wifiEnabled;
    network["ethernet_enabled"] = _config.ethernetEnabled;
    network["primary"] = _config.primary == PrimaryInterface::WIFI ? "wifi" : "ethernet";
    network["failover_enabled"] = _config.failoverEnabled;
    network["failover_timeout"] = _config.failoverTimeout;
    network["reconnect_interval"] = _config.reconnectInterval;
    network["health_check_enabled"] = _config.healthCheckEnabled;
    network["stability_period"] = _config.stabilityPeriod;

    // Configuracao Ethernet
    EthernetConfig& ethConfig = _ethernet.getConfig();
    JsonObject eth = network.createNestedObject("ethernet");
    eth["enabled"] = ethConfig.enabled;
    eth["dhcp"] = ethConfig.useDHCP;
    eth["static_ip"] = ethConfig.staticIP.toString();
    eth["gateway"] = ethConfig.gateway.toString();
    eth["subnet"] = ethConfig.subnet.toString();
    eth["dns"] = ethConfig.dns.toString();
    eth["dhcp_timeout"] = ethConfig.dhcpTimeout;

    // Configuracao WiFi (static IP settings)
    WiFiConfig& wifiConfig = _wifi.getConfig();
    JsonObject wifi = network.createNestedObject("wifi");
    wifi["dhcp"] = wifiConfig.useDHCP;
    wifi["static_ip"] = wifiConfig.staticIP.toString();
    wifi["gateway"] = wifiConfig.gateway.toString();
    wifi["subnet"] = wifiConfig.subnet.toString();
    wifi["dns"] = wifiConfig.dns.toString();

    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[NET] Cannot open config for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println("[NET] Config saved");
    return true;
}

void NetworkManager::setPrimary(PrimaryInterface primary) {
    _config.primary = primary;
    Serial.printf("[NET] Primary set to: %s\n",
                  primary == PrimaryInterface::WIFI ? "WiFi" : "Ethernet");
}

void NetworkManager::setFailoverEnabled(bool enabled) {
    _config.failoverEnabled = enabled;
    Serial.printf("[NET] Failover: %s\n", enabled ? "enabled" : "disabled");
}

void NetworkManager::setFailoverTimeout(uint32_t timeoutMs) {
    _config.failoverTimeout = timeoutMs;
    Serial.printf("[NET] Failover timeout: %d ms\n", timeoutMs);
}

// ================== Status ==================

bool NetworkManager::isConnected() {
    return _activeInterface && _activeInterface->isConnected();
}

NetworkInterface* NetworkManager::getActiveInterface() {
    return _activeInterface;
}

NetworkType NetworkManager::getActiveType() {
    return _activeInterface ? _activeInterface->getType() : NetworkType::NONE;
}

String NetworkManager::getStatusJson() {
    DynamicJsonDocument doc(1536);

    doc["connected"] = isConnected();
    doc["activeInterface"] = _activeInterface ? _activeInterface->getName() : "None";
    doc["failoverActive"] = _failoverActive;
    doc["manualMode"] = _manualMode;

    // Health check status
    doc["applicationHealthy"] = isApplicationHealthy();
    if (_udpForwarder) {
        doc["lastAckTime"] = _udpForwarder->getLastAckTime();
    }

    // Active interface info
    if (_activeInterface) {
        doc["ip"] = _activeInterface->localIP().toString();
        doc["gateway"] = _activeInterface->gatewayIP().toString();
    } else {
        doc["ip"] = "";
        doc["gateway"] = "";
    }

    // Config
    JsonObject config = doc.createNestedObject("config");
    config["primary"] = _config.primary == PrimaryInterface::WIFI ? "wifi" : "ethernet";
    config["failoverEnabled"] = _config.failoverEnabled;
    config["failoverTimeout"] = _config.failoverTimeout;
    config["healthCheckEnabled"] = _config.healthCheckEnabled;
    config["stabilityPeriod"] = _config.stabilityPeriod;

    // WiFi status
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["enabled"] = _config.wifiEnabled;
    wifi["connected"] = _wifi.isConnected();
    wifi["ip"] = _wifi.localIP().toString();
    wifi["rssi"] = _wifi.getRSSI();
    wifi["ssid"] = _wifi.getSSID();
    wifi["mac"] = _wifi.getMacAddress();

    // Ethernet status
    JsonObject ethernet = doc.createNestedObject("ethernet");
    ethernet["enabled"] = _config.ethernetEnabled;
    ethernet["connected"] = _ethernet.isConnected();
    ethernet["linkUp"] = _ethernet.isLinkUp();
    ethernet["ip"] = _ethernet.localIP().toString();
    ethernet["mac"] = _ethernet.getMacAddress();

    // Stats
    JsonObject stats = doc.createNestedObject("stats");
    stats["wifiConnections"] = _stats.wifiConnections;
    stats["wifiDisconnections"] = _stats.wifiDisconnections;
    stats["ethernetConnections"] = _stats.ethernetConnections;
    stats["ethernetDisconnections"] = _stats.ethernetDisconnections;
    stats["failoverCount"] = _stats.failoverCount;
    stats["totalUptimeWifi"] = _stats.totalUptimeWifi;
    stats["totalUptimeEthernet"] = _stats.totalUptimeEthernet;

    String output;
    serializeJson(doc, output);
    return output;
}

String NetworkManager::getHealthJson() {
    DynamicJsonDocument doc(512);

    // Health status - whether the connection to ChirpStack is healthy
    doc["healthy"] = isApplicationHealthy();

    // Last ACK time from UDPForwarder
    if (_udpForwarder) {
        doc["lastAckTime"] = _udpForwarder->getLastAckTime();
    } else {
        doc["lastAckTime"] = 0;
    }

    // Failover configuration
    doc["failoverTimeout"] = _config.failoverTimeout;
    doc["failoverActive"] = _failoverActive;
    doc["stabilityPeriod"] = _config.stabilityPeriod;

    // Time primary has been stable (during failover recovery)
    uint32_t primaryStableFor = 0;
    if (_failoverActive && _primaryStableStart > 0) {
        primaryStableFor = millis() - _primaryStableStart;
    }
    doc["primaryStableFor"] = primaryStableFor;

    String output;
    serializeJson(doc, output);
    return output;
}

// ================== Controle Manual ==================

bool NetworkManager::forceInterface(NetworkType type) {
    _manualMode = true;
    _manualType = type;

    NetworkInterface* target = nullptr;
    if (type == NetworkType::WIFI && _config.wifiEnabled) {
        target = &_wifi;
    } else if (type == NetworkType::ETHERNET && _config.ethernetEnabled) {
        target = &_ethernet;
    }

    if (target && target->isConnected()) {
        switchToInterface(target);
        Serial.printf("[NET] Forced to %s\n", target->getName());
        return true;
    }

    Serial.printf("[NET] Cannot force to %s - not available\n",
                  type == NetworkType::WIFI ? "WiFi" : "Ethernet");
    return false;
}

void NetworkManager::setAutoMode() {
    _manualMode = false;
    _manualType = NetworkType::NONE;
    Serial.println("[NET] Auto mode enabled");
}

void NetworkManager::reconnect() {
    Serial.println("[NET] Reconnecting all interfaces...");

    if (_config.wifiEnabled) {
        // WiFi reconexao eh gerenciada pelo ESP32
    }

    if (_config.ethernetEnabled) {
        _ethernet.reconnect();
    }

    _activeInterface = nullptr;
    _failoverActive = false;
    _primaryStableStart = 0;
}

// ================== UDP ==================

bool NetworkManager::udpBegin(uint16_t port) {
    _udpPort = port;
    _udpStarted = true;
    return startUDP();
}

bool NetworkManager::startUDP() {
    if (!_activeInterface || _udpPort == 0) return false;
    return _activeInterface->udpBegin(_udpPort);
}

void NetworkManager::udpStop() {
    if (_activeInterface) {
        _activeInterface->udpStop();
    }
    _udpStarted = false;
    _udpPort = 0;
}

bool NetworkManager::udpBeginPacket(const char* host, uint16_t port) {
    return _activeInterface ? _activeInterface->udpBeginPacket(host, port) : false;
}

bool NetworkManager::udpBeginPacket(IPAddress ip, uint16_t port) {
    return _activeInterface ? _activeInterface->udpBeginPacket(ip, port) : false;
}

size_t NetworkManager::udpWrite(const uint8_t* buffer, size_t size) {
    return _activeInterface ? _activeInterface->udpWrite(buffer, size) : 0;
}

bool NetworkManager::udpEndPacket() {
    return _activeInterface ? _activeInterface->udpEndPacket() : false;
}

int NetworkManager::udpParsePacket() {
    return _activeInterface ? _activeInterface->udpParsePacket() : 0;
}

int NetworkManager::udpRead(uint8_t* buffer, size_t maxSize) {
    return _activeInterface ? _activeInterface->udpRead(buffer, maxSize) : 0;
}

IPAddress NetworkManager::udpRemoteIP() {
    return _activeInterface ? _activeInterface->udpRemoteIP() : IPAddress();
}

uint16_t NetworkManager::udpRemotePort() {
    return _activeInterface ? _activeInterface->udpRemotePort() : 0;
}
