/**
 * @file main.cpp
 * @brief Firmware principal do ATmega328P - Bridge para W5500 Ethernet
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Este firmware atua como ponte entre o ESP32 (processador principal)
 * e o chip Ethernet W5500 conectado ao ATmega328P via SPI.
 *
 * Perifericos gerenciados pelo ATmega:
 * - W5500 Ethernet (SPI)
 * - LED Debug
 *
 * Nota: RTC DS1307 esta conectado diretamente ao ESP32 (I2C)
 *
 * Comunicacao Serial:
 * - Hardware UART (PD0/PD1): Debug via USB-Serial adapter
 * - SoftwareSerial (PD2/PD3): Comunicacao com ESP32 via shift level
 *
 * Wiring:
 * - PD2 (Arduino pin 2) = RX from ESP32 GPIO17 (TX2)
 * - PD3 (Arduino pin 3) = TX to ESP32 GPIO16 (RX2)
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#include "protocol.h"
#include "w5500_driver.h"

// ============================================================
// SoftwareSerial for ESP32 communication
// ============================================================
// PD2 (pin 2) = RX from ESP32
// PD3 (pin 3) = TX to ESP32
#define ESP_RX_PIN 2  // ATmega RX from ESP32 TX (GPIO17)
#define ESP_TX_PIN 3  // ATmega TX to ESP32 RX (GPIO16)

SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);

// ============================================================
// Configuracoes
// ============================================================

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION_PATCH  0

// Debug serial baud rate (Hardware UART - PD0/PD1)
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

// ESP32 communication baud rate (SoftwareSerial - PD2/PD3)
// IMPORTANT: SoftwareSerial is unreliable at high baud rates!
// 9600 baud is recommended for stability
#ifndef ESP_SERIAL_BAUD
#define ESP_SERIAL_BAUD 9600
#endif

#ifndef ETH_CS_PIN
#define ETH_CS_PIN 10   // PB2
#endif

#ifndef LED_DEBUG_PIN
#define LED_DEBUG_PIN 4 // PD4 (PCINT20/XCK/T0)
#endif

// Keep-alive LED timing
#define LED_KEEPALIVE_INTERVAL 3000  // Blink every 3 seconds
#define LED_KEEPALIVE_ON_TIME  50    // LED on for 50ms

// ============================================================
// Variaveis globais
// ============================================================

// Buffer de recepcao
static uint8_t rxBuffer[PROTO_MAX_DATA_SIZE + PROTO_HEADER_SIZE + PROTO_FOOTER_SIZE];
static uint16_t rxIndex = 0;
static uint32_t rxStartTime = 0;
static bool rxInProgress = false;

// Buffer de transmissao
static uint8_t txBuffer[PROTO_MAX_DATA_SIZE + PROTO_HEADER_SIZE + PROTO_FOOTER_SIZE];

// Estado do sistema
static uint32_t uptimeSeconds = 0;
static uint32_t lastSecondMillis = 0;

// Keep-alive LED state
static uint32_t lastLedBlink = 0;
static bool ledState = false;

// W5500 Ethernet Driver
static W5500Driver w5500(ETH_CS_PIN);
static bool ethInitialized = false;
static uint8_t udpSocket = 0;  // Socket usado para UDP
static bool udpSocketOpen = false;

// DNS server IP (obtained from network config)
static uint8_t dnsServerIP[4] = {8, 8, 8, 8};  // Default: Google DNS

// ============================================================
// Prototipos
// ============================================================

void processPacket(const uint8_t* data, uint16_t length);
void sendResponse(uint8_t cmd, uint8_t status, const uint8_t* data, uint16_t length);
void handleSystemCommand(uint8_t cmd, const uint8_t* data, uint16_t length);
void handleSPICommand(uint8_t cmd, const uint8_t* data, uint16_t length);
void handleEthernetCommand(uint8_t cmd, const uint8_t* data, uint16_t length);
void handleUDPCommand(uint8_t cmd, const uint8_t* data, uint16_t length);
void handleTCPCommand(uint8_t cmd, const uint8_t* data, uint16_t length);
uint16_t getFreeRAM();

// DNS functions
bool dnsResolve(const char* hostname, uint8_t* resultIP);
uint16_t buildDnsQuery(uint8_t* buffer, const char* hostname, uint16_t transactionId);
bool parseDnsResponse(const uint8_t* response, uint16_t length, uint16_t expectedTxId, uint8_t* resultIP);

// ============================================================
// Setup
// ============================================================

void setup() {
    // Inicializar pinos
    pinMode(LED_DEBUG_PIN, OUTPUT);
    pinMode(ETH_CS_PIN, OUTPUT);

    digitalWrite(LED_DEBUG_PIN, LOW);
    digitalWrite(ETH_CS_PIN, HIGH);  // W5500 CS desativado

    // Inicializar Hardware Serial para debug (PD0/PD1 -> USB)
    Serial.begin(SERIAL_BAUD);
    delay(100);

    // Inicializar SoftwareSerial para comunicacao com ESP32 (PD2/PD3)
    // Using lower baud rate for SoftwareSerial stability
    espSerial.begin(ESP_SERIAL_BAUD);
    delay(100);

    DBG_INFO(PSTR("ATmega v%d.%d.%d"),
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    DBG_INFO(PSTR("UART:%lu ESP:%lu"), (unsigned long)SERIAL_BAUD, (unsigned long)ESP_SERIAL_BAUD);

    // Inicializar SPI
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);  // 4MHz para W5500
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    DBG_INFO(PSTR("SPI 4MHz OK"));

    // Inicializar W5500 (verificar se presente)
    if (w5500.begin()) {
        ethInitialized = true;
        DBG_INFO(PSTR("W5500 link=%s"), w5500.getLinkStatus() ? "UP" : "DN");
    } else {
        DBG_ERROR(PSTR("W5500 FAIL"));
    }

    // LED de indicacao de inicializacao
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_DEBUG_PIN, HIGH);
        delay(100);
        digitalWrite(LED_DEBUG_PIN, LOW);
        delay(100);
    }

    DBG_INFO(PSTR("RAM:%u"), getFreeRAM());
    lastSecondMillis = millis();
}

// ============================================================
// Loop principal
// ============================================================

void loop() {
    // Atualizar uptime
    if (millis() - lastSecondMillis >= 1000) {
        lastSecondMillis += 1000;
        uptimeSeconds++;
    }

    // Keep-alive LED blink
    uint32_t now = millis();
    if (ledState) {
        // LED is ON, check if it's time to turn it off
        if (now - lastLedBlink >= LED_KEEPALIVE_ON_TIME) {
            digitalWrite(LED_DEBUG_PIN, LOW);
            ledState = false;
        }
    } else {
        // LED is OFF, check if it's time to blink
        if (now - lastLedBlink >= LED_KEEPALIVE_INTERVAL) {
            digitalWrite(LED_DEBUG_PIN, HIGH);
            ledState = true;
            lastLedBlink = now;
        }
    }

    // Processar dados seriais recebidos do ESP32 (via SoftwareSerial)
    while (espSerial.available()) {
        uint8_t byte = espSerial.read();

        // Detectar inicio de pacote
        if (!rxInProgress && byte == PROTO_START_BYTE) {
            rxInProgress = true;
            rxIndex = 0;
            rxStartTime = millis();
        }

        if (rxInProgress) {
            rxBuffer[rxIndex++] = byte;

            // Verificar se recebemos o cabecalho completo
            if (rxIndex >= PROTO_HEADER_SIZE) {
                uint16_t expectedLength = (rxBuffer[2] << 8) | rxBuffer[3];
                uint16_t totalLength = PROTO_HEADER_SIZE + expectedLength + PROTO_FOOTER_SIZE;

                // Verificar tamanho maximo
                if (totalLength > sizeof(rxBuffer)) {
                    rxInProgress = false;
                    rxIndex = 0;
                    continue;
                }

                // Verificar se recebemos o pacote completo
                if (rxIndex >= totalLength) {
                    // Verificar byte final
                    if (rxBuffer[totalLength - 1] == PROTO_END_BYTE) {
                        // Verificar CRC
                        uint8_t receivedCRC = rxBuffer[totalLength - 2];
                        uint8_t calculatedCRC = calculateCRC8(&rxBuffer[PROTO_HEADER_SIZE], expectedLength);

                        if (receivedCRC == calculatedCRC) {
                            // Pacote valido - processar
                            processPacket(rxBuffer, totalLength);
                        } else {
                            // Erro de CRC
                            sendResponse(rxBuffer[1], RSP_CRC_ERROR, nullptr, 0);
                        }
                    }

                    rxInProgress = false;
                    rxIndex = 0;
                }
            }

            // Timeout
            if (millis() - rxStartTime > PROTO_TIMEOUT_MS) {
                rxInProgress = false;
                rxIndex = 0;
            }
        }
    }
}

// ============================================================
// Processamento de pacotes
// ============================================================

void processPacket(const uint8_t* data, uint16_t length) {
    uint8_t cmd = data[1];
    uint16_t dataLength = (data[2] << 8) | data[3];
    const uint8_t* payload = &data[PROTO_HEADER_SIZE];

    DBG_VERBOSE(PSTR("RX %02X len=%u"), cmd, dataLength);

    // LED para indicar atividade
    digitalWrite(LED_DEBUG_PIN, HIGH);

    // Rotear comando
    if (cmd <= 0x0F) {
        handleSystemCommand(cmd, payload, dataLength);
    } else if (cmd >= 0x10 && cmd <= 0x17) {
        // Comandos Ethernet (init, status, MAC, IP)
        handleEthernetCommand(cmd, payload, dataLength);
    } else if (cmd >= 0x18 && cmd <= 0x1F) {
        // Comandos SPI raw para W5500 (acesso direto)
        handleSPICommand(cmd, payload, dataLength);
    } else if (cmd >= 0x20 && cmd <= 0x2F) {
        // Comandos UDP (inclui CMD_DNS_RESOLVE = 0x25)
        handleUDPCommand(cmd, payload, dataLength);
    } else if (cmd >= 0x30 && cmd <= 0x3F) {
        // Comandos TCP
        handleTCPCommand(cmd, payload, dataLength);
    } else {
        DBG_WARN(PSTR("Bad cmd %02X"), cmd);
        sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }

    digitalWrite(LED_DEBUG_PIN, LOW);
}

void sendResponse(uint8_t cmd, uint8_t status, const uint8_t* data, uint16_t length) {
    uint16_t totalDataLen = length + 1;  // status + data

    txBuffer[0] = PROTO_START_BYTE;
    txBuffer[1] = cmd | 0x80;  // Resposta = comando com bit 7 set
    txBuffer[2] = (totalDataLen >> 8) & 0xFF;
    txBuffer[3] = totalDataLen & 0xFF;
    txBuffer[4] = status;

    if (data && length > 0) {
        memcpy(&txBuffer[5], data, length);
    }

    // Calcular CRC (inclui status + data)
    txBuffer[PROTO_HEADER_SIZE + totalDataLen] = calculateCRC8(&txBuffer[PROTO_HEADER_SIZE], totalDataLen);
    txBuffer[PROTO_HEADER_SIZE + totalDataLen + 1] = PROTO_END_BYTE;

    // Enviar para ESP32 via SoftwareSerial
    espSerial.write(txBuffer, PROTO_HEADER_SIZE + totalDataLen + PROTO_FOOTER_SIZE);
}

// ============================================================
// Handlers de comandos
// ============================================================

void handleSystemCommand(uint8_t cmd, const uint8_t* data, uint16_t length) {
    switch (cmd) {
        case CMD_PING: {
            // Responder com "PONG"
            sendResponse(cmd, RSP_OK, (const uint8_t*)"PONG", 4);
            break;
        }

        case CMD_GET_VERSION: {
            uint8_t version[3] = {
                FIRMWARE_VERSION_MAJOR,
                FIRMWARE_VERSION_MINOR,
                FIRMWARE_VERSION_PATCH
            };
            sendResponse(cmd, RSP_OK, version, 3);
            break;
        }

        case CMD_RESET: {
            sendResponse(cmd, RSP_OK, nullptr, 0);
            delay(100);
            // Soft reset via jump para endereco 0
            asm volatile ("jmp 0");
            break;
        }

        case CMD_GET_STATUS: {
            // Build response buffer
            // Format: [ethInit][linkUp][reserved][uptimeH][uptimeM][uptimeS][freeRamH][freeRamL]
            uint8_t statusData[8];
            statusData[0] = ethInitialized ? 1 : 0;
            statusData[1] = (ethInitialized && w5500.getLinkStatus()) ? 1 : 0;
            statusData[2] = 0;  // Reserved (was rtcInitialized)
            statusData[3] = (uptimeSeconds / 3600) & 0xFF;
            statusData[4] = ((uptimeSeconds % 3600) / 60) & 0xFF;
            statusData[5] = (uptimeSeconds % 60) & 0xFF;
            uint16_t freeRam = getFreeRAM();
            statusData[6] = (freeRam >> 8) & 0xFF;
            statusData[7] = freeRam & 0xFF;
            sendResponse(cmd, RSP_OK, statusData, 8);
            break;
        }

        case CMD_SET_LED: {
            if (length >= 1) {
                digitalWrite(LED_DEBUG_PIN, data[0] ? HIGH : LOW);
                sendResponse(cmd, RSP_OK, nullptr, 0);
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        default:
            sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }
}

// ============================================================
// Ethernet Handlers
// Comandos de alto nivel para W5500
// ============================================================

void handleEthernetCommand(uint8_t cmd, const uint8_t* data, uint16_t length) {
    switch (cmd) {
        case CMD_ETH_INIT: {
            DBG_INFO(PSTR("ETH_INIT %u"), length);
            // Inicializar com IP estatico se dados fornecidos
            if (!ethInitialized) {
                if (w5500.begin()) {
                    ethInitialized = true;
                    DBG_INFO(PSTR("W5500 OK"));
                } else {
                    DBG_ERROR(PSTR("W5500 FAIL"));
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                    break;
                }
            }

            if (length >= sizeof(IPConfig)) {
                // IP estatico fornecido
                const IPConfig* config = (const IPConfig*)data;
                w5500.setIP(config->ip);
                w5500.setGateway(config->gateway);
                w5500.setSubnet(config->subnet);
                // Store DNS server for DNS resolution
                memcpy(dnsServerIP, config->dns, 4);
                DBG_INFO(PSTR("IP %d.%d.%d.%d"),
                         config->ip[0], config->ip[1], config->ip[2], config->ip[3]);
            }

            sendResponse(cmd, RSP_OK, nullptr, 0);
            break;
        }

        case CMD_ETH_STATUS: {
            uint8_t status = ethInitialized ? 1 : 0;
            sendResponse(cmd, RSP_OK, &status, 1);
            break;
        }

        case CMD_ETH_GET_MAC: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            uint8_t mac[6];
            w5500.getMAC(mac);
            sendResponse(cmd, RSP_OK, mac, 6);
            break;
        }

        case CMD_ETH_SET_MAC: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= 6) {
                w5500.setMAC(data);
                sendResponse(cmd, RSP_OK, nullptr, 0);
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_ETH_GET_IP: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            IPConfig config;
            w5500.getIP(config.ip);
            w5500.getGateway(config.gateway);
            w5500.getSubnet(config.subnet);
            memcpy(config.dns, dnsServerIP, 4);  // Return stored DNS
            sendResponse(cmd, RSP_OK, (uint8_t*)&config, sizeof(config));
            break;
        }

        case CMD_ETH_SET_IP: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= sizeof(IPConfig)) {
                const IPConfig* config = (const IPConfig*)data;
                w5500.setIP(config->ip);
                w5500.setGateway(config->gateway);
                w5500.setSubnet(config->subnet);
                // Store DNS server for DNS resolution
                memcpy(dnsServerIP, config->dns, 4);
                sendResponse(cmd, RSP_OK, nullptr, 0);
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_ETH_LINK_STATUS: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            uint8_t linkUp = w5500.getLinkStatus() ? 1 : 0;
            sendResponse(cmd, RSP_OK, &linkUp, 1);
            break;
        }

        default:
            sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }
}

// ============================================================
// UDP Handlers (includes DNS resolution)
// ============================================================

void handleUDPCommand(uint8_t cmd, const uint8_t* data, uint16_t length) {
    switch (cmd) {
        case CMD_UDP_BEGIN: {
            if (!ethInitialized) {
                DBG_WARN(PSTR("UDP:no ETH"));
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= 2) {
                uint16_t port = ((uint16_t)data[0] << 8) | data[1];
                DBG_INFO(PSTR("UDP port %u"), port);
                if (w5500.socketOpenUDP(udpSocket, port)) {
                    udpSocketOpen = true;
                    DBG_INFO(PSTR("UDP OK"));
                    sendResponse(cmd, RSP_OK, nullptr, 0);
                } else {
                    DBG_ERROR(PSTR("UDP FAIL"));
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                }
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_UDP_CLOSE: {
            if (udpSocketOpen) {
                w5500.socketClose(udpSocket);
                udpSocketOpen = false;
            }
            sendResponse(cmd, RSP_OK, nullptr, 0);
            break;
        }

        case CMD_UDP_SEND: {
            if (!ethInitialized || !udpSocketOpen) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= sizeof(NetAddress)) {
                const NetAddress* addr = (const NetAddress*)data;
                const uint8_t* payload = data + sizeof(NetAddress);
                uint16_t payloadLen = length - sizeof(NetAddress);

                uint16_t sent = w5500.udpSend(udpSocket, addr->ip, addr->port,
                                              payload, payloadLen);
                if (sent > 0) {
                    uint8_t response[2] = {(uint8_t)(sent >> 8), (uint8_t)(sent & 0xFF)};
                    sendResponse(cmd, RSP_OK, response, 2);
                } else {
                    DBG_ERROR(PSTR("UDP TX err"));
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                }
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_UDP_RECV: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (!udpSocketOpen) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }

            uint16_t available = w5500.socketAvailable(udpSocket);
            if (available == 0) {
                sendResponse(cmd, RSP_NO_DATA, nullptr, 0);
                break;
            }

            DBG_VERBOSE(PSTR("UDP RX %u"), available);

            // Reuse rxBuffer for response (safe - we're done receiving)
            // Format: [NetAddress][data]
            NetAddress* srcAddr = (NetAddress*)rxBuffer;
            uint8_t* recvData = rxBuffer + sizeof(NetAddress);
            uint16_t maxData = PROTO_MAX_DATA_SIZE - sizeof(NetAddress);

            uint16_t received = w5500.udpReceive(udpSocket, srcAddr->ip, &srcAddr->port,
                                                 recvData, maxData);
            if (received > 0) {
                sendResponse(cmd, RSP_OK, rxBuffer, sizeof(NetAddress) + received);
            } else {
                sendResponse(cmd, RSP_NO_DATA, nullptr, 0);
            }
            break;
        }

        case CMD_UDP_AVAILABLE: {
            if (!ethInitialized || !udpSocketOpen) {
                uint8_t response[2] = {0, 0};
                sendResponse(cmd, RSP_OK, response, 2);
                break;
            }
            uint16_t available = w5500.socketAvailable(udpSocket);
            uint8_t response[2] = {(uint8_t)(available >> 8), (uint8_t)(available & 0xFF)};
            sendResponse(cmd, RSP_OK, response, 2);
            break;
        }

        case CMD_DNS_RESOLVE: {
            // DNS resolution command
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (!w5500.getLinkStatus()) {
                sendResponse(cmd, RSP_NO_LINK, nullptr, 0);
                break;
            }
            if (length == 0 || length > DNS_MAX_HOSTNAME + 1) {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
                break;
            }

            // Ensure hostname is null-terminated
            char hostname[DNS_MAX_HOSTNAME + 1];
            uint16_t hostnameLen = (length < DNS_MAX_HOSTNAME) ? length : DNS_MAX_HOSTNAME;
            memcpy(hostname, data, hostnameLen);
            hostname[hostnameLen] = '\0';

            // Remove trailing null if present in data
            size_t actualLen = strlen(hostname);
            if (actualLen == 0) {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
                break;
            }

            // Perform DNS resolution
            uint8_t resultIP[4];
            if (dnsResolve(hostname, resultIP)) {
                DBG_INFO(PSTR("DNS %d.%d.%d.%d"), resultIP[0], resultIP[1], resultIP[2], resultIP[3]);
                sendResponse(cmd, RSP_OK, resultIP, 4);
            } else {
                DBG_ERROR(PSTR("DNS FAIL"));
                sendResponse(cmd, RSP_ERROR, nullptr, 0);
            }
            break;
        }

        default:
            sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }
}

// ============================================================
// DNS Resolution Implementation
// Lightweight DNS client for ATmega328P
// ============================================================

/**
 * @brief Build DNS query packet
 * @param buffer Output buffer (min 64 bytes)
 * @param hostname Hostname to resolve
 * @param transactionId Transaction ID for matching response
 * @return Length of query packet
 */
uint16_t buildDnsQuery(uint8_t* buffer, const char* hostname, uint16_t transactionId) {
    uint16_t pos = 0;

    // DNS Header (12 bytes)
    buffer[pos++] = (transactionId >> 8) & 0xFF;  // Transaction ID (high)
    buffer[pos++] = transactionId & 0xFF;          // Transaction ID (low)
    buffer[pos++] = 0x01;  // Flags: Standard query, recursion desired
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;  // Questions: 1
    buffer[pos++] = 0x01;
    buffer[pos++] = 0x00;  // Answer RRs: 0
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;  // Authority RRs: 0
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x00;  // Additional RRs: 0
    buffer[pos++] = 0x00;

    // Question section: hostname in DNS format
    // DNS format: [len]label[len]label...[0]
    const char* label = hostname;
    while (*label) {
        // Find end of current label
        const char* dot = label;
        while (*dot && *dot != '.') dot++;

        uint8_t labelLen = dot - label;
        if (labelLen == 0 || labelLen > 63) {
            return 0;  // Invalid hostname
        }

        buffer[pos++] = labelLen;
        memcpy(&buffer[pos], label, labelLen);
        pos += labelLen;

        label = dot;
        if (*label == '.') label++;
    }
    buffer[pos++] = 0x00;  // End of hostname

    // Query type: A (IPv4 address)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;

    // Query class: IN (Internet)
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;

    return pos;
}

/**
 * @brief Parse DNS response and extract IPv4 address
 * @param response Response buffer
 * @param length Response length
 * @param expectedTxId Expected transaction ID
 * @param resultIP Output buffer for IP (4 bytes)
 * @return true if valid A record found
 */
bool parseDnsResponse(const uint8_t* response, uint16_t length, uint16_t expectedTxId, uint8_t* resultIP) {
    if (length < 12) return false;  // Too short for header

    // Check transaction ID
    uint16_t txId = ((uint16_t)response[0] << 8) | response[1];
    if (txId != expectedTxId) return false;

    // Check flags - response bit should be set (0x80), no error (lower 4 bits = 0)
    if (!(response[2] & 0x80)) return false;  // Not a response
    if ((response[3] & 0x0F) != 0) return false;  // Error code present

    // Get counts
    uint16_t questions = ((uint16_t)response[4] << 8) | response[5];
    uint16_t answers = ((uint16_t)response[6] << 8) | response[7];

    if (answers == 0) return false;  // No answers

    // Skip header
    uint16_t pos = 12;

    // Skip question section
    for (uint16_t i = 0; i < questions && pos < length; i++) {
        // Skip hostname (labels)
        while (pos < length && response[pos] != 0) {
            if ((response[pos] & 0xC0) == 0xC0) {
                // Compression pointer
                pos += 2;
                break;
            } else {
                pos += response[pos] + 1;
            }
        }
        if (pos < length && response[pos] == 0) pos++;  // Skip null terminator
        pos += 4;  // Skip QTYPE and QCLASS
    }

    // Parse answer section
    for (uint16_t i = 0; i < answers && pos < length; i++) {
        // Skip name (may be compressed)
        if ((response[pos] & 0xC0) == 0xC0) {
            pos += 2;  // Compression pointer
        } else {
            while (pos < length && response[pos] != 0) {
                pos += response[pos] + 1;
            }
            if (pos < length) pos++;  // Skip null terminator
        }

        if (pos + 10 > length) return false;  // Not enough data

        uint16_t rtype = ((uint16_t)response[pos] << 8) | response[pos + 1];
        // uint16_t rclass = ((uint16_t)response[pos + 2] << 8) | response[pos + 3];
        // uint32_t ttl = ((uint32_t)response[pos + 4] << 24) | ...
        uint16_t rdlength = ((uint16_t)response[pos + 8] << 8) | response[pos + 9];

        pos += 10;  // Skip to RDATA

        if (pos + rdlength > length) return false;

        // Type A (IPv4) = 1, Class IN = 1
        if (rtype == 1 && rdlength == 4) {
            memcpy(resultIP, &response[pos], 4);
            return true;
        }

        pos += rdlength;  // Skip this record's data
    }

    return false;  // No A record found
}

/**
 * @brief Resolve hostname to IPv4 address via DNS
 * @param hostname Hostname to resolve
 * @param resultIP Output buffer for IP (4 bytes)
 * @return true if resolved successfully
 */
bool dnsResolve(const char* hostname, uint8_t* resultIP) {
    // Use socket 2 for DNS (reserved for DNS queries)
    const uint8_t dnsSocket = DNS_SOCKET;

    // Check if DNS server is configured
    if (dnsServerIP[0] == 0 && dnsServerIP[1] == 0 &&
        dnsServerIP[2] == 0 && dnsServerIP[3] == 0) {
        return false;  // No DNS server configured
    }

    // Open UDP socket on random local port
    uint16_t localPort = 10000 + (millis() & 0x3FFF);  // Semi-random port
    if (!w5500.socketOpenUDP(dnsSocket, localPort)) {
        return false;
    }

    // Build DNS query - reuse rxBuffer to save stack
    uint16_t transactionId = (uint16_t)(millis() ^ 0xA5A5);
    uint16_t queryLen = buildDnsQuery(rxBuffer, hostname, transactionId);

    if (queryLen == 0) {
        w5500.socketClose(dnsSocket);
        return false;
    }

    // Send DNS query
    uint16_t sent = w5500.udpSend(dnsSocket, dnsServerIP, DNS_SERVER_PORT,
                                   rxBuffer, queryLen);
    if (sent == 0) {
        w5500.socketClose(dnsSocket);
        return false;
    }

    // Wait for response with timeout
    uint32_t startTime = millis();
    bool resolved = false;

    while (millis() - startTime < DNS_TIMEOUT_MS) {
        uint16_t available = w5500.socketAvailable(dnsSocket);
        if (available > 0) {
            uint8_t srcIP[4];
            uint16_t srcPort;

            // Reuse rxBuffer for response
            uint16_t received = w5500.udpReceive(dnsSocket, srcIP, &srcPort,
                                                  rxBuffer, PROTO_MAX_DATA_SIZE);
            if (received > 0) {
                // Parse DNS response
                if (parseDnsResponse(rxBuffer, received, transactionId, resultIP)) {
                    resolved = true;
                    break;
                }
            }
        }
        delay(10);  // Small delay to avoid busy loop
    }

    // Close DNS socket
    w5500.socketClose(dnsSocket);

    return resolved;
}

// ============================================================
// TCP Handlers
// ============================================================

// Socket usado para TCP (diferente do UDP)
static uint8_t tcpSocket = 1;
static bool tcpSocketOpen = false;

void handleTCPCommand(uint8_t cmd, const uint8_t* data, uint16_t length) {
    switch (cmd) {
        case CMD_TCP_CONNECT: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= sizeof(NetAddress) + 2) {  // NetAddress + local port
                const NetAddress* destAddr = (const NetAddress*)data;
                uint16_t localPort = ((uint16_t)data[6] << 8) | data[7];

                // Fechar socket se aberto
                if (tcpSocketOpen) {
                    w5500.socketClose(tcpSocket);
                }

                // Abrir socket TCP
                if (!w5500.socketOpenTCP(tcpSocket, localPort)) {
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                    break;
                }

                // Conectar
                if (w5500.tcpConnect(tcpSocket, destAddr->ip, destAddr->port, 5000)) {
                    tcpSocketOpen = true;
                    sendResponse(cmd, RSP_OK, nullptr, 0);
                } else {
                    w5500.socketClose(tcpSocket);
                    sendResponse(cmd, RSP_TIMEOUT, nullptr, 0);
                }
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_TCP_LISTEN: {
            if (!ethInitialized) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (length >= 2) {
                uint16_t port = ((uint16_t)data[0] << 8) | data[1];

                // Fechar socket se aberto
                if (tcpSocketOpen) {
                    w5500.socketClose(tcpSocket);
                }

                // Abrir e escutar
                if (w5500.socketOpenTCP(tcpSocket, port)) {
                    if (w5500.tcpListen(tcpSocket)) {
                        tcpSocketOpen = true;
                        sendResponse(cmd, RSP_OK, nullptr, 0);
                    } else {
                        w5500.socketClose(tcpSocket);
                        sendResponse(cmd, RSP_ERROR, nullptr, 0);
                    }
                } else {
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                }
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_TCP_CLOSE: {
            if (tcpSocketOpen) {
                w5500.tcpDisconnect(tcpSocket);
                w5500.socketClose(tcpSocket);
                tcpSocketOpen = false;
            }
            sendResponse(cmd, RSP_OK, nullptr, 0);
            break;
        }

        case CMD_TCP_SEND: {
            if (!ethInitialized || !tcpSocketOpen) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }
            if (!w5500.tcpConnected(tcpSocket)) {
                sendResponse(cmd, RSP_NO_LINK, nullptr, 0);
                break;
            }
            if (length > 0) {
                uint16_t sent = w5500.tcpSend(tcpSocket, data, length);
                if (sent > 0) {
                    uint8_t response[2] = {(uint8_t)(sent >> 8), (uint8_t)(sent & 0xFF)};
                    sendResponse(cmd, RSP_OK, response, 2);
                } else {
                    sendResponse(cmd, RSP_ERROR, nullptr, 0);
                }
            } else {
                sendResponse(cmd, RSP_OK, nullptr, 0);
            }
            break;
        }

        case CMD_TCP_RECV: {
            if (!ethInitialized || !tcpSocketOpen) {
                sendResponse(cmd, RSP_NOT_INIT, nullptr, 0);
                break;
            }

            uint16_t available = w5500.socketAvailable(tcpSocket);
            if (available == 0) {
                sendResponse(cmd, RSP_NO_DATA, nullptr, 0);
                break;
            }

            // Reuse rxBuffer to save stack space
            uint16_t received = w5500.tcpReceive(tcpSocket, rxBuffer, PROTO_MAX_DATA_SIZE);
            if (received > 0) {
                sendResponse(cmd, RSP_OK, rxBuffer, received);
            } else {
                sendResponse(cmd, RSP_NO_DATA, nullptr, 0);
            }
            break;
        }

        case CMD_TCP_AVAILABLE: {
            if (!ethInitialized || !tcpSocketOpen) {
                uint8_t response[2] = {0, 0};
                sendResponse(cmd, RSP_OK, response, 2);
                break;
            }
            uint16_t available = w5500.socketAvailable(tcpSocket);
            uint8_t response[2] = {(uint8_t)(available >> 8), (uint8_t)(available & 0xFF)};
            sendResponse(cmd, RSP_OK, response, 2);
            break;
        }

        case CMD_TCP_STATUS: {
            if (!ethInitialized || !tcpSocketOpen) {
                uint8_t status = 0;  // Closed
                sendResponse(cmd, RSP_OK, &status, 1);
                break;
            }
            uint8_t status = w5500.socketStatus(tcpSocket);
            sendResponse(cmd, RSP_OK, &status, 1);
            break;
        }

        default:
            sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }
}

// ============================================================
// SPI Bridge para W5500 (acesso raw)
// Comandos SPI raw para acesso direto ao W5500
// Nota: Comandos movidos para 0x18-0x1F
// ============================================================

// Definicoes para comandos SPI raw
#define CMD_SPI_RAW_BEGIN     0x18  // Iniciar transacao SPI (CS LOW)
#define CMD_SPI_RAW_END       0x19  // Finalizar transacao SPI (CS HIGH)
#define CMD_SPI_RAW_TRANSFER  0x1A  // Transferir dados SPI
#define CMD_SPI_RAW_TRANSFER16 0x1B // Transferir 16 bits

void handleSPICommand(uint8_t cmd, const uint8_t* data, uint16_t length) {
    switch (cmd) {
        case CMD_SPI_RAW_BEGIN: {
            // Iniciar transacao SPI - CS LOW
            digitalWrite(ETH_CS_PIN, LOW);
            sendResponse(cmd, RSP_OK, nullptr, 0);
            break;
        }

        case CMD_SPI_RAW_END: {
            // Finalizar transacao SPI - CS HIGH
            digitalWrite(ETH_CS_PIN, HIGH);
            sendResponse(cmd, RSP_OK, nullptr, 0);
            break;
        }

        case CMD_SPI_RAW_TRANSFER: {
            // Transferir dados SPI (full-duplex)
            // Entrada: bytes a enviar
            // Saida: bytes recebidos
            if (length > 0 && length <= PROTO_MAX_DATA_SIZE) {
                // Reuse rxBuffer for response
                for (uint16_t i = 0; i < length; i++) {
                    rxBuffer[i] = SPI.transfer(data[i]);
                }
                sendResponse(cmd, RSP_OK, rxBuffer, length);
            } else if (length == 0) {
                sendResponse(cmd, RSP_OK, nullptr, 0);
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        case CMD_SPI_RAW_TRANSFER16: {
            // Transferir 16 bits SPI
            if (length >= 2) {
                uint16_t dataOut = (data[0] << 8) | data[1];
                uint16_t dataIn = SPI.transfer16(dataOut);
                uint8_t response[2] = {(uint8_t)(dataIn >> 8), (uint8_t)(dataIn & 0xFF)};
                sendResponse(cmd, RSP_OK, response, 2);
            } else {
                sendResponse(cmd, RSP_INVALID_PARAM, nullptr, 0);
            }
            break;
        }

        default:
            sendResponse(cmd, RSP_INVALID_CMD, nullptr, 0);
    }
}

// ============================================================
// Utilitarios
// ============================================================

uint16_t getFreeRAM() {
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
