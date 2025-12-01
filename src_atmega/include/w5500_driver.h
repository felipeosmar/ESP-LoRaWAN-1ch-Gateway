/**
 * @file w5500_driver.h
 * @brief Driver leve para W5500 via SPI direto
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Este driver fornece funcoes de baixo nivel para acessar o W5500
 * sem usar a biblioteca Ethernet completa do Arduino.
 *
 * Otimizado para ATmega328P com memoria limitada.
 */

#ifndef W5500_DRIVER_H
#define W5500_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include "w5500_raw.h"

// ============================================================
// Classe W5500Driver
// ============================================================

class W5500Driver {
public:
    /**
     * @brief Construtor
     * @param csPin Pino CS do W5500
     */
    W5500Driver(uint8_t csPin);

    /**
     * @brief Inicializar o W5500
     * @return true se chip detectado corretamente
     */
    bool begin();

    /**
     * @brief Verificar se W5500 esta presente
     * @return true se versao do chip esta correta
     */
    bool isPresent();

    /**
     * @brief Soft reset do W5500
     */
    void softReset();

    // ================== Registradores Comuns ==================

    /**
     * @brief Definir endereco MAC
     * @param mac Buffer de 6 bytes
     */
    void setMAC(const uint8_t* mac);

    /**
     * @brief Obter endereco MAC
     * @param mac Buffer de 6 bytes
     */
    void getMAC(uint8_t* mac);

    /**
     * @brief Definir endereco IP
     * @param ip Buffer de 4 bytes
     */
    void setIP(const uint8_t* ip);

    /**
     * @brief Obter endereco IP
     * @param ip Buffer de 4 bytes
     */
    void getIP(uint8_t* ip);

    /**
     * @brief Definir mascara de sub-rede
     * @param subnet Buffer de 4 bytes
     */
    void setSubnet(const uint8_t* subnet);

    /**
     * @brief Obter mascara de sub-rede
     * @param subnet Buffer de 4 bytes
     */
    void getSubnet(uint8_t* subnet);

    /**
     * @brief Definir gateway
     * @param gateway Buffer de 4 bytes
     */
    void setGateway(const uint8_t* gateway);

    /**
     * @brief Obter gateway
     * @param gateway Buffer de 4 bytes
     */
    void getGateway(uint8_t* gateway);

    /**
     * @brief Verificar status do link PHY
     * @return true se link esta ativo
     */
    bool getLinkStatus();

    /**
     * @brief Obter configuracao PHY
     * @return Valor do registrador PHYCFGR
     */
    uint8_t getPhyConfig();

    // ================== Operacoes de Socket ==================

    /**
     * @brief Abrir socket UDP
     * @param socket Numero do socket (0-7)
     * @param port Porta local
     * @return true se sucesso
     */
    bool socketOpenUDP(uint8_t socket, uint16_t port);

    /**
     * @brief Abrir socket TCP (modo cliente ou servidor)
     * @param socket Numero do socket (0-7)
     * @param port Porta local
     * @return true se sucesso
     */
    bool socketOpenTCP(uint8_t socket, uint16_t port);

    /**
     * @brief Fechar socket
     * @param socket Numero do socket
     */
    void socketClose(uint8_t socket);

    /**
     * @brief Obter status do socket
     * @param socket Numero do socket
     * @return Valor do registrador Sn_SR
     */
    uint8_t socketStatus(uint8_t socket);

    /**
     * @brief Verificar bytes disponiveis para leitura
     * @param socket Numero do socket
     * @return Numero de bytes disponiveis
     */
    uint16_t socketAvailable(uint8_t socket);

    /**
     * @brief Verificar espaco livre no TX buffer
     * @param socket Numero do socket
     * @return Bytes livres no buffer TX
     */
    uint16_t socketTxFree(uint8_t socket);

    // ================== UDP ==================

    /**
     * @brief Enviar dados UDP
     * @param socket Numero do socket
     * @param destIP IP destino (4 bytes)
     * @param destPort Porta destino
     * @param data Dados a enviar
     * @param length Tamanho dos dados
     * @return Bytes enviados ou 0 se erro
     */
    uint16_t udpSend(uint8_t socket, const uint8_t* destIP, uint16_t destPort,
                     const uint8_t* data, uint16_t length);

    /**
     * @brief Receber dados UDP
     * @param socket Numero do socket
     * @param srcIP Buffer para IP origem (4 bytes)
     * @param srcPort Ponteiro para porta origem
     * @param buffer Buffer para dados
     * @param maxLength Tamanho maximo do buffer
     * @return Bytes recebidos ou 0 se nada disponivel
     */
    uint16_t udpReceive(uint8_t socket, uint8_t* srcIP, uint16_t* srcPort,
                        uint8_t* buffer, uint16_t maxLength);

    // ================== TCP ==================

    /**
     * @brief Conectar a servidor TCP
     * @param socket Numero do socket
     * @param destIP IP destino (4 bytes)
     * @param destPort Porta destino
     * @param timeoutMs Timeout em ms
     * @return true se conectado
     */
    bool tcpConnect(uint8_t socket, const uint8_t* destIP, uint16_t destPort,
                    uint16_t timeoutMs = 5000);

    /**
     * @brief Iniciar servidor TCP (listen)
     * @param socket Numero do socket
     * @return true se sucesso
     */
    bool tcpListen(uint8_t socket);

    /**
     * @brief Verificar se ha conexao pendente (servidor)
     * @param socket Numero do socket
     * @return true se cliente conectou
     */
    bool tcpAccepted(uint8_t socket);

    /**
     * @brief Desconectar TCP graciosamente
     * @param socket Numero do socket
     */
    void tcpDisconnect(uint8_t socket);

    /**
     * @brief Enviar dados TCP
     * @param socket Numero do socket
     * @param data Dados a enviar
     * @param length Tamanho dos dados
     * @return Bytes enviados ou 0 se erro
     */
    uint16_t tcpSend(uint8_t socket, const uint8_t* data, uint16_t length);

    /**
     * @brief Receber dados TCP
     * @param socket Numero do socket
     * @param buffer Buffer para dados
     * @param maxLength Tamanho maximo do buffer
     * @return Bytes recebidos ou 0 se nada disponivel
     */
    uint16_t tcpReceive(uint8_t socket, uint8_t* buffer, uint16_t maxLength);

    /**
     * @brief Verificar se conexao TCP esta estabelecida
     * @param socket Numero do socket
     * @return true se conectado
     */
    bool tcpConnected(uint8_t socket);

    // ================== Acesso SPI de baixo nivel ==================

    /**
     * @brief Escrever um byte em registrador
     * @param block Bloco (comum ou socket)
     * @param addr Endereco
     * @param data Byte a escrever
     */
    void write8(uint8_t block, uint16_t addr, uint8_t data);

    /**
     * @brief Ler um byte de registrador
     * @param block Bloco
     * @param addr Endereco
     * @return Byte lido
     */
    uint8_t read8(uint8_t block, uint16_t addr);

    /**
     * @brief Escrever 16 bits em registrador
     * @param block Bloco
     * @param addr Endereco
     * @param data Valor 16-bit
     */
    void write16(uint8_t block, uint16_t addr, uint16_t data);

    /**
     * @brief Ler 16 bits de registrador
     * @param block Bloco
     * @param addr Endereco
     * @return Valor 16-bit
     */
    uint16_t read16(uint8_t block, uint16_t addr);

    /**
     * @brief Escrever buffer em registrador/buffer
     * @param block Bloco
     * @param addr Endereco
     * @param data Buffer
     * @param length Tamanho
     */
    void writeBuffer(uint8_t block, uint16_t addr, const uint8_t* data, uint16_t length);

    /**
     * @brief Ler buffer de registrador/buffer
     * @param block Bloco
     * @param addr Endereco
     * @param buffer Buffer
     * @param length Tamanho
     */
    void readBuffer(uint8_t block, uint16_t addr, uint8_t* buffer, uint16_t length);

private:
    uint8_t _csPin;
    bool _initialized;

    /**
     * @brief Executar comando no socket e aguardar conclusao
     * @param socket Numero do socket
     * @param cmd Comando
     * @return true se comando executado
     */
    bool execSocketCmd(uint8_t socket, uint8_t cmd);

    /**
     * @brief Escrever dados no TX buffer do socket
     * @param socket Numero do socket
     * @param data Dados
     * @param length Tamanho
     * @return Bytes escritos
     */
    uint16_t writeSocketTx(uint8_t socket, const uint8_t* data, uint16_t length);

    /**
     * @brief Ler dados do RX buffer do socket
     * @param socket Numero do socket
     * @param buffer Buffer
     * @param length Tamanho
     * @return Bytes lidos
     */
    uint16_t readSocketRx(uint8_t socket, uint8_t* buffer, uint16_t length);
};

#endif // W5500_DRIVER_H
