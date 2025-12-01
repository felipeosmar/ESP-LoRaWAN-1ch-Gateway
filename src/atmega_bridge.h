/**
 * @file atmega_bridge.h
 * @brief Biblioteca cliente para comunicacao ESP32 <-> ATmega328P
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Esta biblioteca permite ao ESP32 enviar comandos ao ATmega328P
 * para acessar os perifericos conectados a ele:
 * - Ethernet W5500
 * - RTC DS1307Z
 *
 * Nota: O buzzer esta conectado diretamente ao ESP32 GPIO26
 */

#ifndef ATMEGA_BRIDGE_H
#define ATMEGA_BRIDGE_H

#include <Arduino.h>
#include <HardwareSerial.h>

// Importar definicoes do protocolo
// Nota: este arquivo eh compartilhado entre ESP32 e ATmega
#include "../src_atmega/include/protocol.h"

/**
 * @class ATmegaBridge
 * @brief Classe para comunicacao com o ATmega328P
 */
class ATmegaBridge {
public:
    /**
     * @brief Construtor
     * @param serial Referencia para HardwareSerial (Serial1 ou Serial2)
     * @param rxPin Pino RX do ESP32
     * @param txPin Pino TX do ESP32
     */
    ATmegaBridge(HardwareSerial& serial, int rxPin = -1, int txPin = -1);

    /**
     * @brief Inicializar comunicacao
     * @param baudRate Velocidade da serial (default 115200)
     * @return true se ATmega respondeu ao ping
     */
    bool begin(unsigned long baudRate = 115200);

    /**
     * @brief Verificar se ATmega esta respondendo
     * @return true se respondeu ao ping
     */
    bool ping();

    /**
     * @brief Obter versao do firmware do ATmega
     * @param major Versao major
     * @param minor Versao minor
     * @param patch Versao patch
     * @return true se sucesso
     */
    bool getVersion(uint8_t& major, uint8_t& minor, uint8_t& patch);

    /**
     * @brief Obter status do sistema
     * @param status Estrutura para receber o status
     * @return true se sucesso
     */
    bool getStatus(SystemStatus& status);

    /**
     * @brief Resetar ATmega
     * @return true se comando enviado com sucesso
     */
    bool reset();

    /**
     * @brief Controlar LED de debug
     * @param on true para ligar, false para desligar
     * @return true se sucesso
     */
    bool setLED(bool on);

    // ================== Ethernet ==================

    /**
     * @brief Inicializar Ethernet com DHCP
     * @param timeoutMs Timeout para DHCP
     * @return true se sucesso
     */
    bool ethInit(uint16_t timeoutMs = 5000);

    /**
     * @brief Inicializar Ethernet com IP estatico
     * @param ip Endereco IP
     * @param gateway Gateway
     * @param subnet Mascara de sub-rede
     * @param dns Servidor DNS
     * @return true se sucesso
     */
    bool ethInit(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);

    /**
     * @brief Verificar status da Ethernet
     * @return true se inicializada
     */
    bool ethStatus();

    /**
     * @brief Verificar link fisico
     * @return true se link esta ativo
     */
    bool ethLinkStatus();

    /**
     * @brief Obter endereco MAC
     * @param mac Buffer de 6 bytes para o MAC
     * @return true se sucesso
     */
    bool ethGetMAC(uint8_t* mac);

    /**
     * @brief Definir endereco MAC
     * @param mac Buffer de 6 bytes com o MAC
     * @return true se sucesso
     */
    bool ethSetMAC(const uint8_t* mac);

    /**
     * @brief Obter configuracao IP atual
     * @param ip Endereco IP
     * @param gateway Gateway
     * @param subnet Mascara
     * @param dns DNS
     * @return true se sucesso
     */
    bool ethGetIP(IPAddress& ip, IPAddress& gateway, IPAddress& subnet, IPAddress& dns);

    // ================== UDP ==================

    /**
     * @brief Abrir socket UDP
     * @param localPort Porta local
     * @return true se sucesso
     */
    bool udpBegin(uint16_t localPort);

    /**
     * @brief Fechar socket UDP
     * @return true se sucesso
     */
    bool udpClose();

    /**
     * @brief Enviar pacote UDP
     * @param destIP IP destino
     * @param destPort Porta destino
     * @param data Dados
     * @param length Tamanho dos dados
     * @return true se sucesso
     */
    bool udpSend(IPAddress destIP, uint16_t destPort, const uint8_t* data, uint16_t length);

    /**
     * @brief Receber pacote UDP (poll)
     * @param srcIP IP de origem (preenchido se sucesso)
     * @param srcPort Porta de origem (preenchida se sucesso)
     * @param buffer Buffer para dados
     * @param maxLength Tamanho maximo do buffer
     * @param receivedLength Bytes recebidos
     * @return true se recebeu dados
     */
    bool udpReceive(IPAddress& srcIP, uint16_t& srcPort, uint8_t* buffer, uint16_t maxLength, uint16_t& receivedLength);

    /**
     * @brief Verificar dados UDP disponiveis
     * @return Numero de bytes disponiveis
     */
    uint16_t udpAvailable();

    // ================== RTC ==================

    /**
     * @brief Obter data e hora do RTC
     * @param dt Estrutura DateTime
     * @return true se sucesso
     */
    bool rtcGetDateTime(DateTime& dt);

    /**
     * @brief Definir data e hora do RTC
     * @param dt Estrutura DateTime
     * @return true se sucesso
     */
    bool rtcSetDateTime(const DateTime& dt);

    /**
     * @brief Obter apenas hora do RTC
     * @param hour Hora (0-23)
     * @param minute Minuto (0-59)
     * @param second Segundo (0-59)
     * @return true se sucesso
     */
    bool rtcGetTime(uint8_t& hour, uint8_t& minute, uint8_t& second);

    /**
     * @brief Obter apenas data do RTC
     * @param year Ano (0-99, desde 2000)
     * @param month Mes (1-12)
     * @param day Dia (1-31)
     * @param dayOfWeek Dia da semana (1-7, 1=Dom)
     * @return true se sucesso
     */
    bool rtcGetDate(uint8_t& year, uint8_t& month, uint8_t& day, uint8_t& dayOfWeek);

    // ================== I2C Raw ==================

    /**
     * @brief Escanear barramento I2C
     * @param addresses Buffer para enderecos encontrados
     * @param maxCount Tamanho maximo do buffer
     * @param foundCount Quantidade encontrada
     * @return true se sucesso
     */
    bool i2cScan(uint8_t* addresses, uint8_t maxCount, uint8_t& foundCount);

    /**
     * @brief Escrever dados I2C
     * @param address Endereco I2C
     * @param data Dados
     * @param length Tamanho
     * @return true se sucesso
     */
    bool i2cWrite(uint8_t address, const uint8_t* data, uint8_t length);

    /**
     * @brief Ler dados I2C
     * @param address Endereco I2C
     * @param buffer Buffer para dados
     * @param length Quantidade a ler
     * @return true se sucesso
     */
    bool i2cRead(uint8_t address, uint8_t* buffer, uint8_t length);

    // Note: Buzzer is connected directly to ESP32 GPIO26, not via ATmega bridge

    // ================== Configuracao ==================

    /**
     * @brief Definir timeout para comandos
     * @param timeoutMs Timeout em ms
     */
    void setTimeout(uint16_t timeoutMs);

    /**
     * @brief Obter ultimo codigo de erro
     * @return Codigo de erro (RSP_xxx)
     */
    uint8_t getLastError() const;

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    uint16_t _timeout;
    uint8_t _lastError;

    // Buffers
    uint8_t _txBuffer[PROTO_MAX_DATA_SIZE + PROTO_HEADER_SIZE + PROTO_FOOTER_SIZE];
    uint8_t _rxBuffer[PROTO_MAX_DATA_SIZE + PROTO_HEADER_SIZE + PROTO_FOOTER_SIZE];

    /**
     * @brief Enviar comando e aguardar resposta
     * @param cmd Comando
     * @param data Dados do comando
     * @param dataLength Tamanho dos dados
     * @param response Buffer para resposta
     * @param responseLength Tamanho da resposta
     * @return true se sucesso
     */
    bool sendCommand(uint8_t cmd, const uint8_t* data, uint16_t dataLength,
                     uint8_t* response, uint16_t& responseLength);

    /**
     * @brief Calcular CRC8
     */
    uint8_t calcCRC8(const uint8_t* data, uint16_t length);
};

#endif // ATMEGA_BRIDGE_H
