/**
 * @file protocol.h
 * @brief Protocolo de comunicacao serial entre ESP32 e ATmega328P
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Formato do pacote:
 * [START][CMD][LEN_H][LEN_L][DATA...][CRC][END]
 *
 * START: 0xAA
 * CMD:   Comando (1 byte)
 * LEN:   Tamanho dos dados (2 bytes, big-endian)
 * DATA:  Dados do comando (0-1024 bytes)
 * CRC:   CRC8 dos dados
 * END:   0x55
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// ============================================================
// Debug Configuration
// ============================================================

// Uncomment to enable debug output via Serial
#define DEBUG_ENABLED 1

// Debug levels
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

// Current debug level
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// Debug macros
#if DEBUG_ENABLED
    #define DBG_PRINT(x)      Serial.print(x)
    #define DBG_PRINTLN(x)    Serial.println(x)
    #define DBG_PRINTF(...)   do { char _dbg_buf[80]; snprintf(_dbg_buf, sizeof(_dbg_buf), __VA_ARGS__); Serial.print(_dbg_buf); } while(0)

    #define DBG_ERROR(...)    do { if (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)   { Serial.print(F("[ERR] ")); DBG_PRINTF(__VA_ARGS__); Serial.println(); } } while(0)
    #define DBG_WARN(...)     do { if (DEBUG_LEVEL >= DEBUG_LEVEL_WARN)    { Serial.print(F("[WRN] ")); DBG_PRINTF(__VA_ARGS__); Serial.println(); } } while(0)
    #define DBG_INFO(...)     do { if (DEBUG_LEVEL >= DEBUG_LEVEL_INFO)    { Serial.print(F("[INF] ")); DBG_PRINTF(__VA_ARGS__); Serial.println(); } } while(0)
    #define DBG_VERBOSE(...)  do { if (DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) { Serial.print(F("[VRB] ")); DBG_PRINTF(__VA_ARGS__); Serial.println(); } } while(0)
#else
    #define DBG_PRINT(x)
    #define DBG_PRINTLN(x)
    #define DBG_PRINTF(...)
    #define DBG_ERROR(...)
    #define DBG_WARN(...)
    #define DBG_INFO(...)
    #define DBG_VERBOSE(...)
#endif

// ============================================================

// Marcadores de pacote
#define PROTO_START_BYTE    0xAA
#define PROTO_END_BYTE      0x55

// Tamanho maximo de dados
// ATmega328P tem apenas 2KB RAM - usar buffer menor
#if defined(__AVR_ATmega328P__) || defined(ATMEGA328P_BRIDGE)
#define PROTO_MAX_DATA_SIZE 128     // Menor para ATmega328P
#else
#define PROTO_MAX_DATA_SIZE 512     // ESP32 pode usar mais
#endif

#define PROTO_HEADER_SIZE   4   // START + CMD + LEN_H + LEN_L
#define PROTO_FOOTER_SIZE   2   // CRC + END

// Timeout para receber pacote completo (ms)
#define PROTO_TIMEOUT_MS    1000

// ============================================================
// Comandos do protocolo
// ============================================================

// --- Comandos de Sistema (0x00 - 0x0F) ---
#define CMD_PING            0x00    // Teste de comunicacao
#define CMD_GET_VERSION     0x01    // Obter versao do firmware
#define CMD_RESET           0x02    // Reset do ATmega
#define CMD_GET_STATUS      0x03    // Status geral
#define CMD_SET_LED         0x04    // Controlar LED debug

// --- Comandos Ethernet (0x10 - 0x3F) ---
#define CMD_ETH_INIT        0x10    // Inicializar Ethernet
#define CMD_ETH_STATUS      0x11    // Status da conexao
#define CMD_ETH_GET_MAC     0x12    // Obter MAC address
#define CMD_ETH_SET_MAC     0x13    // Definir MAC address
#define CMD_ETH_GET_IP      0x14    // Obter configuracao IP
#define CMD_ETH_SET_IP      0x15    // Definir IP estatico
#define CMD_ETH_DHCP        0x16    // Iniciar DHCP
#define CMD_ETH_LINK_STATUS 0x17    // Status do link fisico

// Comandos de Socket UDP
#define CMD_UDP_BEGIN       0x20    // Abrir socket UDP
#define CMD_UDP_CLOSE       0x21    // Fechar socket UDP
#define CMD_UDP_SEND        0x22    // Enviar pacote UDP
#define CMD_UDP_RECV        0x23    // Receber pacote UDP (poll)
#define CMD_UDP_AVAILABLE   0x24    // Verificar dados disponiveis

/**
 * @brief CMD_DNS_RESOLVE (0x25) - Resolver hostname para IP via DNS
 *
 * Request format:
 *   [hostname] - Null-terminated hostname string (max 63 chars + null)
 *   Example: "chirpstack.example.com\0"
 *
 * Response format on success (RSP_OK):
 *   [IP0][IP1][IP2][IP3] - 4-byte IPv4 address
 *   Example: [192][168][1][100] for 192.168.1.100
 *
 * Response format on failure:
 *   RSP_ERROR   - DNS resolution failed (hostname not found, server error)
 *   RSP_TIMEOUT - DNS query timed out (5 second timeout)
 *   RSP_NOT_INIT - Ethernet not initialized
 *   RSP_NO_LINK - No Ethernet link
 *
 * Notes:
 *   - Uses W5500 DNS client via UDP socket 2
 *   - DNS server IP must be configured (via DHCP or static config)
 *   - Hostname max length: 63 characters (DNS label limit)
 *   - Timeout: 5 seconds
 */
#define CMD_DNS_RESOLVE     0x25    // Resolver hostname para IPv4

// Comandos de Socket TCP
#define CMD_TCP_CONNECT     0x30    // Conectar TCP cliente
#define CMD_TCP_LISTEN      0x31    // Iniciar TCP servidor
#define CMD_TCP_CLOSE       0x32    // Fechar conexao TCP
#define CMD_TCP_SEND        0x33    // Enviar dados TCP
#define CMD_TCP_RECV        0x34    // Receber dados TCP
#define CMD_TCP_AVAILABLE   0x35    // Verificar dados disponiveis
#define CMD_TCP_STATUS      0x36    // Status da conexao

// --- Comandos RTC (0x40 - 0x4F) ---
#define CMD_RTC_GET_TIME    0x40    // Obter hora atual
#define CMD_RTC_SET_TIME    0x41    // Definir hora
#define CMD_RTC_GET_DATE    0x42    // Obter data atual
#define CMD_RTC_SET_DATE    0x43    // Definir data
#define CMD_RTC_GET_DATETIME 0x44   // Obter data e hora
#define CMD_RTC_SET_DATETIME 0x45   // Definir data e hora
#define CMD_RTC_GET_TEMP    0x46    // Obter temperatura (se disponivel)

// --- Comandos I2C Raw (0x50 - 0x5F) ---
#define CMD_I2C_SCAN        0x50    // Escanear barramento I2C
#define CMD_I2C_WRITE       0x51    // Escrever dados I2C
#define CMD_I2C_READ        0x52    // Ler dados I2C
#define CMD_I2C_WRITE_READ  0x53    // Escrever e ler I2C

// ============================================================
// Codigos de resposta
// ============================================================
#define RSP_OK              0x00    // Sucesso
#define RSP_ERROR           0x01    // Erro generico
#define RSP_INVALID_CMD     0x02    // Comando invalido
#define RSP_INVALID_PARAM   0x03    // Parametro invalido
#define RSP_TIMEOUT         0x04    // Timeout
#define RSP_BUSY            0x05    // Ocupado
#define RSP_NOT_INIT        0x06    // Nao inicializado
#define RSP_NO_LINK         0x07    // Sem link Ethernet
#define RSP_NO_DATA         0x08    // Sem dados disponiveis
#define RSP_BUFFER_FULL     0x09    // Buffer cheio
#define RSP_CRC_ERROR       0x0A    // Erro de CRC

// ============================================================
// DNS Constants
// ============================================================
#define DNS_TIMEOUT_MS      5000    // 5 second DNS timeout
#define DNS_MAX_HOSTNAME    63      // Max hostname length (DNS label limit)
#define DNS_SERVER_PORT     53      // Standard DNS port
#define DNS_SOCKET          2       // Socket reserved for DNS queries

// ============================================================
// Estruturas de dados
// ============================================================

// Cabecalho do pacote (apenas para referencia - nao usar em ATmega328P)
typedef struct {
    uint8_t start;
    uint8_t cmd;
    uint16_t length;
} __attribute__((packed)) PacketHeader;

// NOTA: Struct Packet removida para economizar RAM no ATmega328P
// Use buffers separados para rx/tx em vez de struct estatica

// Configuracao IP
typedef struct {
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t subnet[4];
    uint8_t dns[4];
} __attribute__((packed)) IPConfig;

// Endereco UDP/TCP
typedef struct {
    uint8_t ip[4];
    uint16_t port;
} __attribute__((packed)) NetAddress;

// Data e hora
typedef struct {
    uint8_t year;       // Anos desde 2000
    uint8_t month;      // 1-12
    uint8_t day;        // 1-31
    uint8_t dayOfWeek;  // 1-7 (1=Domingo)
    uint8_t hour;       // 0-23
    uint8_t minute;     // 0-59
    uint8_t second;     // 0-59
} __attribute__((packed)) DateTime;

// Status do sistema
typedef struct {
    uint8_t eth_initialized;
    uint8_t eth_link_up;
    uint8_t rtc_initialized;
    uint8_t uptime_hours;
    uint8_t uptime_minutes;
    uint8_t uptime_seconds;
    uint16_t free_ram;
} __attribute__((packed)) SystemStatus;

// ============================================================
// Funcoes utilitarias
// ============================================================

/**
 * Calcula CRC8 dos dados
 */
inline uint8_t calculateCRC8(const uint8_t* data, uint16_t length) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;  // Polynomial x^8 + x^5 + x^4 + 1
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#endif // PROTOCOL_H
