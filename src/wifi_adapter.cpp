/**
 * @file wifi_adapter.cpp
 * @brief Implementacao do adaptador WiFi
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "wifi_adapter.h"

WiFiAdapter::WiFiAdapter()
    : _status(NetworkStatus::DISCONNECTED)
    , _connectedTime(0)
    , _udpStarted(false)
{
}

WiFiAdapter::~WiFiAdapter() {
    end();
}

// ================== Inicializacao ==================

bool WiFiAdapter::begin() {
    // WiFi ja deve estar inicializado pelo main.cpp
    // Este metodo apenas verifica o estado atual

    // Em modo AP, verificar se AP esta ativo
    if (WiFi.getMode() == WIFI_AP) {
        if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
            _status = NetworkStatus::CONNECTED;
            if (_connectedTime == 0) {
                _connectedTime = millis();
            }
            Serial.printf("[WiFi] AP mode detected, IP: %s\n", WiFi.softAPIP().toString().c_str());
            return true;
        }
    }

    // Em modo Station, verificar conexao
    if (WiFi.status() == WL_CONNECTED) {
        _status = NetworkStatus::CONNECTED;
        if (_connectedTime == 0) {
            _connectedTime = millis();
        }
        return true;
    }

    _status = NetworkStatus::DISCONNECTED;
    return false;
}

void WiFiAdapter::end() {
    udpStop();
    _status = NetworkStatus::DISCONNECTED;
    _connectedTime = 0;
}

void WiFiAdapter::update() {
    // Em modo AP, sempre considerado conectado se IP valido
    if (WiFi.getMode() == WIFI_AP) {
        if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
            if (_status != NetworkStatus::CONNECTED) {
                _status = NetworkStatus::CONNECTED;
                _connectedTime = millis();
                Serial.printf("[WiFi] AP mode active, IP: %s\n", WiFi.softAPIP().toString().c_str());
            }
        } else {
            _status = NetworkStatus::DISCONNECTED;
            _connectedTime = 0;
        }
        return;
    }

    // Modo Station
    wl_status_t wifiStatus = WiFi.status();

    switch (wifiStatus) {
        case WL_CONNECTED:
            if (_status != NetworkStatus::CONNECTED) {
                _status = NetworkStatus::CONNECTED;
                _connectedTime = millis();
                Serial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
            }
            break;

        case WL_DISCONNECTED:
        case WL_CONNECTION_LOST:
            if (_status == NetworkStatus::CONNECTED) {
                Serial.println("[WiFi] Disconnected");
            }
            _status = NetworkStatus::DISCONNECTED;
            _connectedTime = 0;
            break;

        case WL_CONNECT_FAILED:
        case WL_NO_SSID_AVAIL:
            _status = NetworkStatus::ERROR;
            _connectedTime = 0;
            break;

        default:
            _status = NetworkStatus::CONNECTING;
            break;
    }
}

// ================== Status ==================

bool WiFiAdapter::isConnected() {
    // Em modo Station, verificar WL_CONNECTED
    if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
        return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
    }
    // Em modo AP, considerar conectado se AP está ativo
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
    }
    return false;
}

bool WiFiAdapter::isLinkUp() {
    // WiFi nao tem "link" fisico como Ethernet
    // Consideramos link up se estiver conectado ou tentando conectar
    return WiFi.status() == WL_CONNECTED ||
           WiFi.status() == WL_IDLE_STATUS;
}

NetworkStatus WiFiAdapter::getStatus() {
    update();
    return _status;
}

NetworkType WiFiAdapter::getType() {
    return NetworkType::WIFI;
}

const char* WiFiAdapter::getName() {
    return "WiFi";
}

void WiFiAdapter::getInfo(NetworkInfo& info) {
    info.type = NetworkType::WIFI;
    info.status = _status;
    info.ip = WiFi.localIP();
    info.gateway = WiFi.gatewayIP();
    info.subnet = WiFi.subnetMask();
    info.dns = WiFi.dnsIP();

    WiFi.macAddress(info.mac);

    info.rssi = WiFi.RSSI();
    info.linkUp = isLinkUp();
    info.connectedTime = _connectedTime > 0 ? millis() - _connectedTime : 0;
}

// ================== Configuracao IP ==================

IPAddress WiFiAdapter::localIP() {
    // Em modo AP, retornar IP do softAP
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

IPAddress WiFiAdapter::gatewayIP() {
    return WiFi.gatewayIP();
}

void WiFiAdapter::macAddress(uint8_t* mac) {
    WiFi.macAddress(mac);
}

// ================== UDP ==================

bool WiFiAdapter::udpBegin(uint16_t port) {
    if (_udpStarted) {
        _udp.stop();
    }

    _udpStarted = _udp.begin(port);
    if (_udpStarted) {
        Serial.printf("[WiFi] UDP started on port %d\n", port);
    } else {
        Serial.println("[WiFi] Failed to start UDP");
    }
    return _udpStarted;
}

void WiFiAdapter::udpStop() {
    if (_udpStarted) {
        _udp.stop();
        _udpStarted = false;
        Serial.println("[WiFi] UDP stopped");
    }
}

bool WiFiAdapter::udpBeginPacket(IPAddress ip, uint16_t port) {
    if (!_udpStarted) return false;
    return _udp.beginPacket(ip, port);
}

bool WiFiAdapter::udpBeginPacket(const char* host, uint16_t port) {
    if (!_udpStarted) return false;
    return _udp.beginPacket(host, port);
}

size_t WiFiAdapter::udpWrite(const uint8_t* buffer, size_t size) {
    if (!_udpStarted) return 0;
    return _udp.write(buffer, size);
}

bool WiFiAdapter::udpEndPacket() {
    if (!_udpStarted) return false;
    return _udp.endPacket();
}

int WiFiAdapter::udpParsePacket() {
    if (!_udpStarted) return 0;
    return _udp.parsePacket();
}

int WiFiAdapter::udpRead(uint8_t* buffer, size_t maxSize) {
    if (!_udpStarted) return 0;
    return _udp.read(buffer, maxSize);
}

IPAddress WiFiAdapter::udpRemoteIP() {
    return _udp.remoteIP();
}

uint16_t WiFiAdapter::udpRemotePort() {
    return _udp.remotePort();
}

// ================== DNS ==================

bool WiFiAdapter::hostByName(const char* host, IPAddress& result) {
    return WiFi.hostByName(host, result);
}

// ================== WiFi Especifico ==================

int8_t WiFiAdapter::getRSSI() {
    return WiFi.RSSI();
}

String WiFiAdapter::getSSID() {
    return WiFi.SSID();
}
