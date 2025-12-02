/**
 * @file atmega_bridge.cpp
 * @brief Implementacao da biblioteca cliente ATmega328P Bridge
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "atmega_bridge.h"

ATmegaBridge::ATmegaBridge(HardwareSerial& serial, int rxPin, int txPin)
    : _serial(serial)
    , _rxPin(rxPin)
    , _txPin(txPin)
    , _timeout(1000)
    , _lastError(RSP_OK)
{
}

bool ATmegaBridge::begin(unsigned long baudRate) {
    if (_rxPin >= 0 && _txPin >= 0) {
        _serial.begin(baudRate, SERIAL_8N1, _rxPin, _txPin);
    } else {
        _serial.begin(baudRate);
    }

    delay(100);  // Aguardar estabilizacao

    // Testar comunicacao com ping
    return ping();
}

void ATmegaBridge::setTimeout(uint16_t timeoutMs) {
    _timeout = timeoutMs;
}

uint8_t ATmegaBridge::getLastError() const {
    return _lastError;
}

uint8_t ATmegaBridge::calcCRC8(const uint8_t* data, uint16_t length) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool ATmegaBridge::sendCommand(uint8_t cmd, const uint8_t* data, uint16_t dataLength,
                                uint8_t* response, uint16_t& responseLength) {
    // Montar pacote
    _txBuffer[0] = PROTO_START_BYTE;
    _txBuffer[1] = cmd;
    _txBuffer[2] = (dataLength >> 8) & 0xFF;
    _txBuffer[3] = dataLength & 0xFF;

    if (data && dataLength > 0) {
        memcpy(&_txBuffer[PROTO_HEADER_SIZE], data, dataLength);
    }

    // CRC dos dados
    _txBuffer[PROTO_HEADER_SIZE + dataLength] = calcCRC8(&_txBuffer[PROTO_HEADER_SIZE], dataLength);
    _txBuffer[PROTO_HEADER_SIZE + dataLength + 1] = PROTO_END_BYTE;

    uint16_t packetLength = PROTO_HEADER_SIZE + dataLength + PROTO_FOOTER_SIZE;

    // Limpar buffer de recepcao
    while (_serial.available()) {
        _serial.read();
    }

    // Enviar pacote
    _serial.write(_txBuffer, packetLength);
    _serial.flush();

    // Aguardar resposta
    uint32_t startTime = millis();
    uint16_t rxIndex = 0;
    bool rxInProgress = false;
    uint16_t expectedLength = 0;

    while (millis() - startTime < _timeout) {
        if (_serial.available()) {
            uint8_t byte = _serial.read();

            if (!rxInProgress && byte == PROTO_START_BYTE) {
                rxInProgress = true;
                rxIndex = 0;
            }

            if (rxInProgress) {
                _rxBuffer[rxIndex++] = byte;

                // Verificar cabecalho
                if (rxIndex >= PROTO_HEADER_SIZE && expectedLength == 0) {
                    expectedLength = (_rxBuffer[2] << 8) | _rxBuffer[3];
                    expectedLength += PROTO_HEADER_SIZE + PROTO_FOOTER_SIZE;

                    if (expectedLength > sizeof(_rxBuffer)) {
                        rxInProgress = false;
                        rxIndex = 0;
                        expectedLength = 0;
                        continue;
                    }
                }

                // Verificar se pacote completo
                if (expectedLength > 0 && rxIndex >= expectedLength) {
                    // Verificar byte final
                    if (_rxBuffer[expectedLength - 1] != PROTO_END_BYTE) {
                        _lastError = RSP_ERROR;
                        return false;
                    }

                    // Verificar se eh resposta ao comando enviado
                    if (_rxBuffer[1] != (cmd | 0x80)) {
                        _lastError = RSP_ERROR;
                        return false;
                    }

                    // Verificar CRC
                    uint16_t respDataLen = (_rxBuffer[2] << 8) | _rxBuffer[3];
                    uint8_t receivedCRC = _rxBuffer[PROTO_HEADER_SIZE + respDataLen];
                    uint8_t calculatedCRC = calcCRC8(&_rxBuffer[PROTO_HEADER_SIZE], respDataLen);

                    if (receivedCRC != calculatedCRC) {
                        _lastError = RSP_CRC_ERROR;
                        return false;
                    }

                    // Extrair status
                    _lastError = _rxBuffer[PROTO_HEADER_SIZE];

                    // Copiar dados da resposta (sem status)
                    if (response && responseLength > 0 && respDataLen > 1) {
                        uint16_t copyLen = min((uint16_t)(respDataLen - 1), responseLength);
                        memcpy(response, &_rxBuffer[PROTO_HEADER_SIZE + 1], copyLen);
                        responseLength = copyLen;
                    } else {
                        responseLength = 0;
                    }

                    return (_lastError == RSP_OK);
                }
            }
        }

        yield();  // Permitir outras tarefas
    }

    _lastError = RSP_TIMEOUT;
    return false;
}

// ================== Sistema ==================

bool ATmegaBridge::ping() {
    uint8_t response[8];
    uint16_t respLen = sizeof(response);

    if (sendCommand(CMD_PING, nullptr, 0, response, respLen)) {
        // Verificar se recebeu "PONG"
        return (respLen >= 4 && memcmp(response, "PONG", 4) == 0);
    }
    return false;
}

bool ATmegaBridge::getVersion(uint8_t& major, uint8_t& minor, uint8_t& patch) {
    uint8_t response[3];
    uint16_t respLen = sizeof(response);

    if (sendCommand(CMD_GET_VERSION, nullptr, 0, response, respLen) && respLen >= 3) {
        major = response[0];
        minor = response[1];
        patch = response[2];
        return true;
    }
    return false;
}

bool ATmegaBridge::getStatus(SystemStatus& status) {
    uint16_t respLen = sizeof(SystemStatus);
    return sendCommand(CMD_GET_STATUS, nullptr, 0, (uint8_t*)&status, respLen);
}

bool ATmegaBridge::reset() {
    uint16_t respLen = 0;
    return sendCommand(CMD_RESET, nullptr, 0, nullptr, respLen);
}

bool ATmegaBridge::setLED(bool on) {
    uint8_t data = on ? 1 : 0;
    uint16_t respLen = 0;
    return sendCommand(CMD_SET_LED, &data, 1, nullptr, respLen);
}

// ================== Ethernet ==================

bool ATmegaBridge::ethInit(uint16_t timeoutMs) {
    uint16_t respLen = 0;
    uint16_t oldTimeout = _timeout;
    _timeout = timeoutMs + 1000;
    bool result = sendCommand(CMD_ETH_INIT, nullptr, 0, nullptr, respLen);
    _timeout = oldTimeout;
    return result;
}

bool ATmegaBridge::ethInit(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns) {
    IPConfig config;
    config.ip[0] = ip[0]; config.ip[1] = ip[1]; config.ip[2] = ip[2]; config.ip[3] = ip[3];
    config.gateway[0] = gateway[0]; config.gateway[1] = gateway[1]; config.gateway[2] = gateway[2]; config.gateway[3] = gateway[3];
    config.subnet[0] = subnet[0]; config.subnet[1] = subnet[1]; config.subnet[2] = subnet[2]; config.subnet[3] = subnet[3];
    config.dns[0] = dns[0]; config.dns[1] = dns[1]; config.dns[2] = dns[2]; config.dns[3] = dns[3];

    uint16_t respLen = 0;
    return sendCommand(CMD_ETH_INIT, (uint8_t*)&config, sizeof(config), nullptr, respLen);
}

bool ATmegaBridge::ethStatus() {
    uint8_t response;
    uint16_t respLen = 1;
    if (sendCommand(CMD_ETH_STATUS, nullptr, 0, &response, respLen)) {
        return response == 1;
    }
    return false;
}

bool ATmegaBridge::ethLinkStatus() {
    uint8_t response;
    uint16_t respLen = 1;
    if (sendCommand(CMD_ETH_LINK_STATUS, nullptr, 0, &response, respLen)) {
        return response == 1;
    }
    return false;
}

bool ATmegaBridge::ethGetMAC(uint8_t* mac) {
    uint16_t respLen = 6;
    return sendCommand(CMD_ETH_GET_MAC, nullptr, 0, mac, respLen) && respLen == 6;
}

bool ATmegaBridge::ethSetMAC(const uint8_t* mac) {
    uint16_t respLen = 0;
    return sendCommand(CMD_ETH_SET_MAC, mac, 6, nullptr, respLen);
}

bool ATmegaBridge::ethGetIP(IPAddress& ip, IPAddress& gateway, IPAddress& subnet, IPAddress& dns) {
    IPConfig config;
    uint16_t respLen = sizeof(config);

    if (sendCommand(CMD_ETH_GET_IP, nullptr, 0, (uint8_t*)&config, respLen)) {
        ip = IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]);
        gateway = IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
        subnet = IPAddress(config.subnet[0], config.subnet[1], config.subnet[2], config.subnet[3]);
        dns = IPAddress(config.dns[0], config.dns[1], config.dns[2], config.dns[3]);
        return true;
    }
    return false;
}

// ================== UDP ==================

bool ATmegaBridge::udpBegin(uint16_t localPort) {
    uint8_t data[2] = {(uint8_t)(localPort >> 8), (uint8_t)(localPort & 0xFF)};
    uint16_t respLen = 0;
    return sendCommand(CMD_UDP_BEGIN, data, 2, nullptr, respLen);
}

bool ATmegaBridge::udpClose() {
    uint16_t respLen = 0;
    return sendCommand(CMD_UDP_CLOSE, nullptr, 0, nullptr, respLen);
}

bool ATmegaBridge::udpSend(IPAddress destIP, uint16_t destPort, const uint8_t* data, uint16_t length) {
    if (length > PROTO_MAX_DATA_SIZE - sizeof(NetAddress)) {
        _lastError = RSP_BUFFER_FULL;
        return false;
    }

    uint8_t buffer[PROTO_MAX_DATA_SIZE];
    NetAddress* addr = (NetAddress*)buffer;
    addr->ip[0] = destIP[0];
    addr->ip[1] = destIP[1];
    addr->ip[2] = destIP[2];
    addr->ip[3] = destIP[3];
    addr->port = destPort;

    memcpy(buffer + sizeof(NetAddress), data, length);

    uint16_t respLen = 0;
    return sendCommand(CMD_UDP_SEND, buffer, sizeof(NetAddress) + length, nullptr, respLen);
}

bool ATmegaBridge::udpReceive(IPAddress& srcIP, uint16_t& srcPort, uint8_t* buffer, uint16_t maxLength, uint16_t& receivedLength) {
    uint8_t response[PROTO_MAX_DATA_SIZE];
    uint16_t respLen = sizeof(response);

    if (sendCommand(CMD_UDP_RECV, nullptr, 0, response, respLen)) {
        if (respLen >= sizeof(NetAddress)) {
            NetAddress* addr = (NetAddress*)response;
            srcIP = IPAddress(addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3]);
            srcPort = addr->port;

            uint16_t dataLen = respLen - sizeof(NetAddress);
            receivedLength = min(dataLen, maxLength);
            memcpy(buffer, response + sizeof(NetAddress), receivedLength);
            return true;
        }
    }

    receivedLength = 0;
    return false;
}

uint16_t ATmegaBridge::udpAvailable() {
    uint8_t response[2];
    uint16_t respLen = 2;

    if (sendCommand(CMD_UDP_AVAILABLE, nullptr, 0, response, respLen) && respLen >= 2) {
        return (response[0] << 8) | response[1];
    }
    return 0;
}

// ================== DNS ==================

bool ATmegaBridge::dnsResolve(const char* hostname, IPAddress& result) {
    if (!hostname || strlen(hostname) == 0) {
        _lastError = RSP_INVALID_PARAM;
        return false;
    }

    // Check hostname length (DNS label limit is 63 chars)
    size_t hostnameLen = strlen(hostname);
    if (hostnameLen > DNS_MAX_HOSTNAME) {
        _lastError = RSP_INVALID_PARAM;
        return false;
    }

    // Send hostname as null-terminated string
    uint8_t response[4];
    uint16_t respLen = 4;

    // Use longer timeout for DNS (5 seconds + protocol overhead)
    uint16_t oldTimeout = _timeout;
    _timeout = DNS_TIMEOUT_MS + 1000;

    bool success = sendCommand(CMD_DNS_RESOLVE, (const uint8_t*)hostname,
                               hostnameLen + 1, response, respLen);

    _timeout = oldTimeout;

    if (success && respLen >= 4) {
        result = IPAddress(response[0], response[1], response[2], response[3]);
        return true;
    }

    return false;
}

// ================== RTC ==================

bool ATmegaBridge::rtcGetDateTime(DateTime& dt) {
    uint16_t respLen = sizeof(DateTime);
    return sendCommand(CMD_RTC_GET_DATETIME, nullptr, 0, (uint8_t*)&dt, respLen);
}

bool ATmegaBridge::rtcSetDateTime(const DateTime& dt) {
    uint16_t respLen = 0;
    return sendCommand(CMD_RTC_SET_DATETIME, (const uint8_t*)&dt, sizeof(DateTime), nullptr, respLen);
}

bool ATmegaBridge::rtcGetTime(uint8_t& hour, uint8_t& minute, uint8_t& second) {
    uint8_t response[3];
    uint16_t respLen = 3;

    if (sendCommand(CMD_RTC_GET_TIME, nullptr, 0, response, respLen) && respLen >= 3) {
        hour = response[0];
        minute = response[1];
        second = response[2];
        return true;
    }
    return false;
}

bool ATmegaBridge::rtcGetDate(uint8_t& year, uint8_t& month, uint8_t& day, uint8_t& dayOfWeek) {
    uint8_t response[4];
    uint16_t respLen = 4;

    if (sendCommand(CMD_RTC_GET_DATE, nullptr, 0, response, respLen) && respLen >= 4) {
        year = response[0];
        month = response[1];
        day = response[2];
        dayOfWeek = response[3];
        return true;
    }
    return false;
}

// ================== I2C ==================

bool ATmegaBridge::i2cScan(uint8_t* addresses, uint8_t maxCount, uint8_t& foundCount) {
    uint16_t respLen = maxCount;

    if (sendCommand(CMD_I2C_SCAN, nullptr, 0, addresses, respLen)) {
        foundCount = respLen;
        return true;
    }

    foundCount = 0;
    return false;
}

bool ATmegaBridge::i2cWrite(uint8_t address, const uint8_t* data, uint8_t length) {
    if (length > 30) {
        _lastError = RSP_BUFFER_FULL;
        return false;
    }

    uint8_t buffer[32];
    buffer[0] = address;
    buffer[1] = length;
    memcpy(&buffer[2], data, length);

    uint16_t respLen = 0;
    return sendCommand(CMD_I2C_WRITE, buffer, 2 + length, nullptr, respLen);
}

bool ATmegaBridge::i2cRead(uint8_t address, uint8_t* buffer, uint8_t length) {
    if (length > 32) {
        _lastError = RSP_BUFFER_FULL;
        return false;
    }

    uint8_t cmd[2] = {address, length};
    uint16_t respLen = length;

    return sendCommand(CMD_I2C_READ, cmd, 2, buffer, respLen);
}

// Note: Buzzer is connected directly to ESP32 GPIO26, not via ATmega bridge
