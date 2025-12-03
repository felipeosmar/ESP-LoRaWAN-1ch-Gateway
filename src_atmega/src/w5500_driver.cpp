/**
 * @file w5500_driver.cpp
 * @brief Implementacao do driver leve para W5500
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "w5500_driver.h"
#include "protocol.h"  // Para macros de debug

// ============================================================
// Construtor
// ============================================================

W5500Driver::W5500Driver(uint8_t csPin)
    : _csPin(csPin)
    , _initialized(false)
{
}

// ============================================================
// Inicializacao
// ============================================================

bool W5500Driver::begin() {
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    // Configurar SPI
    SPI.begin();

    // Verificar se W5500 esta presente
    if (!isPresent()) {
        DBG_ERROR(PSTR("W5500 not found"));
        return false;
    }

    // Soft reset
    softReset();
    delay(10);

    // Verificar novamente apos reset
    if (!isPresent()) {
        DBG_ERROR(PSTR("W5500 reset fail"));
        return false;
    }

    // Configurar tamanho dos buffers (2KB por socket, padrao)
    for (uint8_t i = 0; i < W5500_SOCKET_COUNT; i++) {
        write8(W5500_SOCKET_REG(i), W5500_Sn_RXBUF_SIZE, 2);  // 2KB
        write8(W5500_SOCKET_REG(i), W5500_Sn_TXBUF_SIZE, 2);  // 2KB
    }

    _initialized = true;
    DBG_INFO(PSTR("W5500 init OK"));
    return true;
}

bool W5500Driver::isPresent() {
    uint8_t version = read8(W5500_COMMON_REG, W5500_VERSIONR);
    return (version == W5500_CHIP_VERSION);
}

void W5500Driver::softReset() {
    // Set MR register bit 7 (RST)
    write8(W5500_COMMON_REG, W5500_MR, 0x80);

    // Aguardar reset completar
    delay(1);

    // Aguardar bit RST voltar a 0
    uint8_t timeout = 100;
    while (timeout-- > 0) {
        if ((read8(W5500_COMMON_REG, W5500_MR) & 0x80) == 0) {
            break;
        }
        delay(1);
    }
}

// ============================================================
// Registradores Comuns
// ============================================================

void W5500Driver::setMAC(const uint8_t* mac) {
    writeBuffer(W5500_COMMON_REG, W5500_SHAR, mac, 6);
}

void W5500Driver::getMAC(uint8_t* mac) {
    readBuffer(W5500_COMMON_REG, W5500_SHAR, mac, 6);
}

void W5500Driver::setIP(const uint8_t* ip) {
    writeBuffer(W5500_COMMON_REG, W5500_SIPR, ip, 4);
}

void W5500Driver::getIP(uint8_t* ip) {
    readBuffer(W5500_COMMON_REG, W5500_SIPR, ip, 4);
}

void W5500Driver::setSubnet(const uint8_t* subnet) {
    writeBuffer(W5500_COMMON_REG, W5500_SUBR, subnet, 4);
}

void W5500Driver::getSubnet(uint8_t* subnet) {
    readBuffer(W5500_COMMON_REG, W5500_SUBR, subnet, 4);
}

void W5500Driver::setGateway(const uint8_t* gateway) {
    writeBuffer(W5500_COMMON_REG, W5500_GAR, gateway, 4);
}

void W5500Driver::getGateway(uint8_t* gateway) {
    readBuffer(W5500_COMMON_REG, W5500_GAR, gateway, 4);
}

bool W5500Driver::getLinkStatus() {
    uint8_t phycfg = read8(W5500_COMMON_REG, W5500_PHYCFGR);
    return (phycfg & W5500_PHYCFGR_LNK) != 0;
}

uint8_t W5500Driver::getPhyConfig() {
    return read8(W5500_COMMON_REG, W5500_PHYCFGR);
}

// ============================================================
// Operacoes de Socket
// ============================================================

bool W5500Driver::socketOpenUDP(uint8_t socket, uint16_t port) {
    if (socket >= W5500_SOCKET_COUNT) return false;

    // Fechar socket se estiver aberto
    socketClose(socket);

    // Configurar modo UDP
    write8(W5500_SOCKET_REG(socket), W5500_Sn_MR, W5500_Sn_MR_UDP);

    // Configurar porta local
    write16(W5500_SOCKET_REG(socket), W5500_Sn_PORT, port);

    // Abrir socket
    if (!execSocketCmd(socket, W5500_Sn_CR_OPEN)) {
        DBG_ERROR(PSTR("Sk%u open fail"), socket);
        return false;
    }

    // Verificar se abriu corretamente
    uint8_t status = socketStatus(socket);
    if (status == W5500_Sn_SR_UDP) {
        return true;
    }

    DBG_ERROR(PSTR("Sk%u st=%02X"), socket, status);
    return false;
}

bool W5500Driver::socketOpenTCP(uint8_t socket, uint16_t port) {
    if (socket >= W5500_SOCKET_COUNT) return false;

    // Fechar socket se estiver aberto
    socketClose(socket);

    // Configurar modo TCP
    write8(W5500_SOCKET_REG(socket), W5500_Sn_MR, W5500_Sn_MR_TCP);

    // Configurar porta local
    write16(W5500_SOCKET_REG(socket), W5500_Sn_PORT, port);

    // Abrir socket
    if (!execSocketCmd(socket, W5500_Sn_CR_OPEN)) {
        return false;
    }

    // Verificar se inicializou corretamente
    return (socketStatus(socket) == W5500_Sn_SR_INIT);
}

void W5500Driver::socketClose(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return;

    // Enviar comando CLOSE
    execSocketCmd(socket, W5500_Sn_CR_CLOSE);

    // Limpar interrupcoes
    write8(W5500_SOCKET_REG(socket), W5500_Sn_IR, 0xFF);
}

uint8_t W5500Driver::socketStatus(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    return read8(W5500_SOCKET_REG(socket), W5500_Sn_SR);
}

uint16_t W5500Driver::socketAvailable(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    return read16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RSR);
}

uint16_t W5500Driver::socketTxFree(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    return read16(W5500_SOCKET_REG(socket), W5500_Sn_TX_FSR);
}

// ============================================================
// UDP
// ============================================================

uint16_t W5500Driver::udpSend(uint8_t socket, const uint8_t* destIP, uint16_t destPort,
                              const uint8_t* data, uint16_t length) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    if (socketStatus(socket) != W5500_Sn_SR_UDP) {
        return 0;
    }

    // Verificar espaco disponivel
    uint16_t freeSize = socketTxFree(socket);
    if (freeSize < length) {
        length = freeSize;
    }

    if (length == 0) return 0;

    // Configurar destino
    writeBuffer(W5500_SOCKET_REG(socket), W5500_Sn_DIPR, destIP, 4);
    write16(W5500_SOCKET_REG(socket), W5500_Sn_DPORT, destPort);

    // Escrever dados no TX buffer
    writeSocketTx(socket, data, length);

    // Enviar
    if (!execSocketCmd(socket, W5500_Sn_CR_SEND)) {
        DBG_ERROR(PSTR("TX cmd fail"));
        return 0;
    }

    // Aguardar envio completar (com timeout)
    uint32_t start = millis();
    while (millis() - start < 1000) {
        uint8_t ir = read8(W5500_SOCKET_REG(socket), W5500_Sn_IR);
        if (ir & W5500_Sn_IR_SEND_OK) {
            // Limpar flag
            write8(W5500_SOCKET_REG(socket), W5500_Sn_IR, W5500_Sn_IR_SEND_OK);
            return length;
        }
        if (ir & W5500_Sn_IR_TIMEOUT) {
            // Timeout - limpar flag
            write8(W5500_SOCKET_REG(socket), W5500_Sn_IR, W5500_Sn_IR_TIMEOUT);
            DBG_ERROR(PSTR("ARP timeout"));
            return 0;
        }
    }

    DBG_ERROR(PSTR("TX timeout"));
    return 0;
}

uint16_t W5500Driver::udpReceive(uint8_t socket, uint8_t* srcIP, uint16_t* srcPort,
                                 uint8_t* buffer, uint16_t maxLength) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    if (socketStatus(socket) != W5500_Sn_SR_UDP) return 0;

    uint16_t available = socketAvailable(socket);
    if (available == 0) return 0;

    // Ler header UDP do buffer RX (8 bytes)
    // Formato: [4 bytes IP][2 bytes Port][2 bytes Length]
    uint8_t header[W5500_UDP_HEADER_SIZE];
    readSocketRx(socket, header, W5500_UDP_HEADER_SIZE);

    // Extrair informacoes
    if (srcIP) {
        srcIP[0] = header[0];
        srcIP[1] = header[1];
        srcIP[2] = header[2];
        srcIP[3] = header[3];
    }

    if (srcPort) {
        *srcPort = ((uint16_t)header[4] << 8) | header[5];
    }

    uint16_t dataLen = ((uint16_t)header[6] << 8) | header[7];

    // Limitar ao tamanho do buffer
    if (dataLen > maxLength) {
        dataLen = maxLength;
    }

    // Ler dados
    if (dataLen > 0 && buffer) {
        // Atualizar RX_RD pointer apos ler header
        uint16_t ptr = read16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RD);
        ptr += W5500_UDP_HEADER_SIZE;
        write16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RD, ptr);

        // Ler dados do pacote
        readBuffer(W5500_SOCKET_RX_BUF(socket), ptr & 0x07FF, buffer, dataLen);

        // Atualizar ponteiro de leitura
        ptr += dataLen;
        write16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RD, ptr);
    }

    // Comando RECV para liberar buffer
    execSocketCmd(socket, W5500_Sn_CR_RECV);

    return dataLen;
}

// ============================================================
// TCP
// ============================================================

bool W5500Driver::tcpConnect(uint8_t socket, const uint8_t* destIP, uint16_t destPort,
                             uint16_t timeoutMs) {
    if (socket >= W5500_SOCKET_COUNT) return false;
    if (socketStatus(socket) != W5500_Sn_SR_INIT) return false;

    // Configurar destino
    writeBuffer(W5500_SOCKET_REG(socket), W5500_Sn_DIPR, destIP, 4);
    write16(W5500_SOCKET_REG(socket), W5500_Sn_DPORT, destPort);

    // Conectar
    if (!execSocketCmd(socket, W5500_Sn_CR_CONNECT)) {
        return false;
    }

    // Aguardar conexao
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        uint8_t status = socketStatus(socket);
        if (status == W5500_Sn_SR_ESTABLISHED) {
            return true;
        }
        if (status == W5500_Sn_SR_CLOSED) {
            return false;
        }
        delay(1);
    }

    return false;
}

bool W5500Driver::tcpListen(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return false;
    if (socketStatus(socket) != W5500_Sn_SR_INIT) return false;

    return execSocketCmd(socket, W5500_Sn_CR_LISTEN);
}

bool W5500Driver::tcpAccepted(uint8_t socket) {
    return (socketStatus(socket) == W5500_Sn_SR_ESTABLISHED);
}

void W5500Driver::tcpDisconnect(uint8_t socket) {
    if (socket >= W5500_SOCKET_COUNT) return;

    execSocketCmd(socket, W5500_Sn_CR_DISCON);

    // Aguardar fechamento (com timeout)
    uint32_t start = millis();
    while (millis() - start < 1000) {
        uint8_t status = socketStatus(socket);
        if (status == W5500_Sn_SR_CLOSED) {
            break;
        }
        delay(1);
    }
}

uint16_t W5500Driver::tcpSend(uint8_t socket, const uint8_t* data, uint16_t length) {
    if (socket >= W5500_SOCKET_COUNT) return 0;
    if (socketStatus(socket) != W5500_Sn_SR_ESTABLISHED) return 0;

    // Verificar espaco disponivel
    uint16_t freeSize = socketTxFree(socket);
    if (freeSize < length) {
        length = freeSize;
    }

    if (length == 0) return 0;

    // Escrever dados no TX buffer
    writeSocketTx(socket, data, length);

    // Enviar
    if (!execSocketCmd(socket, W5500_Sn_CR_SEND)) {
        return 0;
    }

    // Aguardar envio completar
    uint32_t start = millis();
    while (millis() - start < 5000) {
        uint8_t ir = read8(W5500_SOCKET_REG(socket), W5500_Sn_IR);
        if (ir & W5500_Sn_IR_SEND_OK) {
            write8(W5500_SOCKET_REG(socket), W5500_Sn_IR, W5500_Sn_IR_SEND_OK);
            return length;
        }
        if (ir & W5500_Sn_IR_TIMEOUT) {
            write8(W5500_SOCKET_REG(socket), W5500_Sn_IR, W5500_Sn_IR_TIMEOUT);
            return 0;
        }
        // Verificar se conexao caiu
        if (socketStatus(socket) != W5500_Sn_SR_ESTABLISHED) {
            return 0;
        }
    }

    return 0;
}

uint16_t W5500Driver::tcpReceive(uint8_t socket, uint8_t* buffer, uint16_t maxLength) {
    if (socket >= W5500_SOCKET_COUNT) return 0;

    uint16_t available = socketAvailable(socket);
    if (available == 0) return 0;

    if (available > maxLength) {
        available = maxLength;
    }

    // Ler dados
    readSocketRx(socket, buffer, available);

    // Comando RECV para liberar buffer
    execSocketCmd(socket, W5500_Sn_CR_RECV);

    return available;
}

bool W5500Driver::tcpConnected(uint8_t socket) {
    return (socketStatus(socket) == W5500_Sn_SR_ESTABLISHED);
}

// ============================================================
// Acesso SPI de baixo nivel
// ============================================================

void W5500Driver::write8(uint8_t block, uint16_t addr, uint8_t data) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_WRITE));
    SPI.transfer(data);
    digitalWrite(_csPin, HIGH);
}

uint8_t W5500Driver::read8(uint8_t block, uint16_t addr) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_READ));
    uint8_t data = SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    return data;
}

void W5500Driver::write16(uint8_t block, uint16_t addr, uint16_t data) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_WRITE));
    SPI.transfer((data >> 8) & 0xFF);
    SPI.transfer(data & 0xFF);
    digitalWrite(_csPin, HIGH);
}

uint16_t W5500Driver::read16(uint8_t block, uint16_t addr) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_READ));
    uint16_t data = ((uint16_t)SPI.transfer(0) << 8);
    data |= SPI.transfer(0);
    digitalWrite(_csPin, HIGH);
    return data;
}

void W5500Driver::writeBuffer(uint8_t block, uint16_t addr, const uint8_t* data, uint16_t length) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_WRITE));
    for (uint16_t i = 0; i < length; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(_csPin, HIGH);
}

void W5500Driver::readBuffer(uint8_t block, uint16_t addr, uint8_t* buffer, uint16_t length) {
    digitalWrite(_csPin, LOW);
    SPI.transfer((addr >> 8) & 0xFF);
    SPI.transfer(addr & 0xFF);
    SPI.transfer(W5500_CTRL(block, W5500_CTRL_READ));
    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = SPI.transfer(0);
    }
    digitalWrite(_csPin, HIGH);
}

// ============================================================
// Funcoes internas
// ============================================================

bool W5500Driver::execSocketCmd(uint8_t socket, uint8_t cmd) {
    if (socket >= W5500_SOCKET_COUNT) return false;

    // Enviar comando
    write8(W5500_SOCKET_REG(socket), W5500_Sn_CR, cmd);

    // Aguardar comando ser processado (registrador volta a 0)
    uint16_t timeout = 1000;
    while (timeout-- > 0) {
        if (read8(W5500_SOCKET_REG(socket), W5500_Sn_CR) == 0) {
            return true;
        }
        delayMicroseconds(10);
    }

    return false;
}

uint16_t W5500Driver::writeSocketTx(uint8_t socket, const uint8_t* data, uint16_t length) {
    // Obter ponteiro de escrita atual
    uint16_t ptr = read16(W5500_SOCKET_REG(socket), W5500_Sn_TX_WR);

    // Escrever dados no buffer TX
    // O endereco no buffer eh ptr & 0x07FF (mascara para 2KB)
    writeBuffer(W5500_SOCKET_TX_BUF(socket), ptr & 0x07FF, data, length);

    // Atualizar ponteiro de escrita
    ptr += length;
    write16(W5500_SOCKET_REG(socket), W5500_Sn_TX_WR, ptr);

    return length;
}

uint16_t W5500Driver::readSocketRx(uint8_t socket, uint8_t* buffer, uint16_t length) {
    // Obter ponteiro de leitura atual
    uint16_t ptr = read16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RD);

    // Ler dados do buffer RX
    readBuffer(W5500_SOCKET_RX_BUF(socket), ptr & 0x07FF, buffer, length);

    // Atualizar ponteiro de leitura
    ptr += length;
    write16(W5500_SOCKET_REG(socket), W5500_Sn_RX_RD, ptr);

    return length;
}
