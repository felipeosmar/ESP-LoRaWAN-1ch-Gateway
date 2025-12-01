/**
 * @file w5500_raw.h
 * @brief Definicoes de registradores W5500 para acesso SPI direto
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Este arquivo define os registradores do W5500 para acesso direto via SPI,
 * sem usar a biblioteca Ethernet completa (que nao cabe no ATmega328P).
 *
 * O W5500 usa um protocolo SPI de 4 bytes:
 * - 2 bytes de endereco (16-bit offset address)
 * - 1 byte de controle (BSB[4:0] | R/W | OP Mode[1:0])
 * - N bytes de dados
 *
 * BSB (Block Select Bits):
 *   00000 = Common Register
 *   00001 = Socket 0 Register
 *   00010 = Socket 0 TX Buffer
 *   00011 = Socket 0 RX Buffer
 *   00101 = Socket 1 Register
 *   ...
 *
 * R/W: 0 = Read, 1 = Write
 * OP Mode: 00 = VDM (Variable Length Data Mode)
 */

#ifndef W5500_RAW_H
#define W5500_RAW_H

#include <stdint.h>

// ============================================================
// Block Select Bits (BSB)
// ============================================================

#define W5500_COMMON_REG        0x00    // Common Register Block
#define W5500_SOCKET_REG(n)     ((n)*4 + 1)  // Socket n Register Block
#define W5500_SOCKET_TX_BUF(n)  ((n)*4 + 2)  // Socket n TX Buffer Block
#define W5500_SOCKET_RX_BUF(n)  ((n)*4 + 3)  // Socket n RX Buffer Block

// Control byte
#define W5500_CTRL_READ         0x00
#define W5500_CTRL_WRITE        0x04
#define W5500_CTRL_VDM          0x00    // Variable Data Length Mode (N bytes)
#define W5500_CTRL_FDM_1        0x01    // Fixed Data Length Mode - 1 byte
#define W5500_CTRL_FDM_2        0x02    // Fixed Data Length Mode - 2 bytes
#define W5500_CTRL_FDM_4        0x03    // Fixed Data Length Mode - 4 bytes

// Macro para montar control byte
#define W5500_CTRL(block, rw)   (((block) << 3) | (rw) | W5500_CTRL_VDM)

// ============================================================
// Common Register Addresses (Block 0x00)
// ============================================================

#define W5500_MR        0x0000  // Mode Register (1 byte)
#define W5500_GAR       0x0001  // Gateway Address (4 bytes)
#define W5500_SUBR      0x0005  // Subnet Mask (4 bytes)
#define W5500_SHAR      0x0009  // Source Hardware Address / MAC (6 bytes)
#define W5500_SIPR      0x000F  // Source IP Address (4 bytes)
#define W5500_INTLEVEL  0x0013  // Interrupt Low Level Timer (2 bytes)
#define W5500_IR        0x0015  // Interrupt Register (1 byte)
#define W5500_IMR       0x0016  // Interrupt Mask Register (1 byte)
#define W5500_SIR       0x0017  // Socket Interrupt Register (1 byte)
#define W5500_SIMR      0x0018  // Socket Interrupt Mask Register (1 byte)
#define W5500_RTR       0x0019  // Retry Time (2 bytes)
#define W5500_RCR       0x001B  // Retry Count (1 byte)
#define W5500_PTIMER    0x001C  // PPP LCP Request Timer (1 byte)
#define W5500_PMAGIC    0x001D  // PPP LCP Magic Number (1 byte)
#define W5500_PHAR      0x001E  // PPP Destination MAC (6 bytes)
#define W5500_PSID      0x0024  // PPP Session ID (2 bytes)
#define W5500_PMRU      0x0026  // PPP Max Segment Size (2 bytes)
#define W5500_UIPR      0x0028  // Unreachable IP Address (4 bytes)
#define W5500_UPORTR    0x002C  // Unreachable Port (2 bytes)
#define W5500_PHYCFGR   0x002E  // PHY Configuration Register (1 byte)
#define W5500_VERSIONR  0x0039  // Chip Version Register (1 byte) - Should be 0x04

// ============================================================
// PHY Configuration Register bits (PHYCFGR - 0x002E)
// ============================================================

#define W5500_PHYCFGR_RST       0x80    // PHY Reset
#define W5500_PHYCFGR_OPMD      0x40    // Operation Mode (0=SW, 1=HW)
#define W5500_PHYCFGR_OPMDC     0x38    // Operation Mode Configuration
#define W5500_PHYCFGR_DPX       0x04    // Duplex Status (0=Half, 1=Full)
#define W5500_PHYCFGR_SPD       0x02    // Speed Status (0=10Mbps, 1=100Mbps)
#define W5500_PHYCFGR_LNK       0x01    // Link Status (0=Down, 1=Up)

// ============================================================
// Socket Register Addresses (offset dentro do bloco do socket)
// ============================================================

#define W5500_Sn_MR     0x0000  // Socket n Mode Register (1 byte)
#define W5500_Sn_CR     0x0001  // Socket n Command Register (1 byte)
#define W5500_Sn_IR     0x0002  // Socket n Interrupt Register (1 byte)
#define W5500_Sn_SR     0x0003  // Socket n Status Register (1 byte)
#define W5500_Sn_PORT   0x0004  // Socket n Source Port (2 bytes)
#define W5500_Sn_DHAR   0x0006  // Socket n Destination Hardware Address (6 bytes)
#define W5500_Sn_DIPR   0x000C  // Socket n Destination IP Address (4 bytes)
#define W5500_Sn_DPORT  0x0010  // Socket n Destination Port (2 bytes)
#define W5500_Sn_MSSR   0x0012  // Socket n Max Segment Size (2 bytes)
#define W5500_Sn_TOS    0x0015  // Socket n IP TOS (1 byte)
#define W5500_Sn_TTL    0x0016  // Socket n IP TTL (1 byte)
#define W5500_Sn_RXBUF_SIZE 0x001E  // Socket n RX Buffer Size (1 byte)
#define W5500_Sn_TXBUF_SIZE 0x001F  // Socket n TX Buffer Size (1 byte)
#define W5500_Sn_TX_FSR 0x0020  // Socket n TX Free Size (2 bytes)
#define W5500_Sn_TX_RD  0x0022  // Socket n TX Read Pointer (2 bytes)
#define W5500_Sn_TX_WR  0x0024  // Socket n TX Write Pointer (2 bytes)
#define W5500_Sn_RX_RSR 0x0026  // Socket n RX Received Size (2 bytes)
#define W5500_Sn_RX_RD  0x0028  // Socket n RX Read Pointer (2 bytes)
#define W5500_Sn_RX_WR  0x002A  // Socket n RX Write Pointer (2 bytes)
#define W5500_Sn_IMR    0x002C  // Socket n Interrupt Mask (1 byte)
#define W5500_Sn_FRAG   0x002D  // Socket n Fragment Offset (2 bytes)
#define W5500_Sn_KPALVTR 0x002F // Socket n Keep Alive Timer (1 byte)

// ============================================================
// Socket Mode Register bits (Sn_MR)
// ============================================================

#define W5500_Sn_MR_CLOSE   0x00    // Closed
#define W5500_Sn_MR_TCP     0x01    // TCP
#define W5500_Sn_MR_UDP     0x02    // UDP
#define W5500_Sn_MR_MACRAW  0x04    // MAC RAW

#define W5500_Sn_MR_MULTI   0x80    // Multicast (UDP)
#define W5500_Sn_MR_BCASTB  0x40    // Broadcast Blocking (UDP)
#define W5500_Sn_MR_ND      0x20    // No Delayed ACK (TCP)
#define W5500_Sn_MR_UCASTB  0x10    // Unicast Blocking (UDP)

// ============================================================
// Socket Command Register values (Sn_CR)
// ============================================================

#define W5500_Sn_CR_OPEN        0x01    // Open socket
#define W5500_Sn_CR_LISTEN      0x02    // Listen (TCP server)
#define W5500_Sn_CR_CONNECT     0x04    // Connect (TCP client)
#define W5500_Sn_CR_DISCON      0x08    // Disconnect
#define W5500_Sn_CR_CLOSE       0x10    // Close socket
#define W5500_Sn_CR_SEND        0x20    // Send data
#define W5500_Sn_CR_SEND_MAC    0x21    // Send data with MAC (UDP)
#define W5500_Sn_CR_SEND_KEEP   0x22    // Send keep-alive (TCP)
#define W5500_Sn_CR_RECV        0x40    // Receive data

// ============================================================
// Socket Interrupt Register bits (Sn_IR)
// ============================================================

#define W5500_Sn_IR_SEND_OK 0x10    // Send operation completed
#define W5500_Sn_IR_TIMEOUT 0x08    // Timeout occurred
#define W5500_Sn_IR_RECV    0x04    // Data received
#define W5500_Sn_IR_DISCON  0x02    // Disconnect (TCP)
#define W5500_Sn_IR_CON     0x01    // Connection established (TCP)

// ============================================================
// Socket Status Register values (Sn_SR)
// ============================================================

#define W5500_Sn_SR_CLOSED      0x00    // Socket closed
#define W5500_Sn_SR_INIT        0x13    // TCP initialized
#define W5500_Sn_SR_LISTEN      0x14    // TCP listening
#define W5500_Sn_SR_SYNSENT     0x15    // TCP SYN sent
#define W5500_Sn_SR_SYNRECV     0x16    // TCP SYN received
#define W5500_Sn_SR_ESTABLISHED 0x17    // TCP connection established
#define W5500_Sn_SR_FIN_WAIT    0x18    // TCP FIN wait
#define W5500_Sn_SR_CLOSING     0x1A    // TCP closing
#define W5500_Sn_SR_TIME_WAIT   0x1B    // TCP time wait
#define W5500_Sn_SR_CLOSE_WAIT  0x1C    // TCP close wait
#define W5500_Sn_SR_LAST_ACK    0x1D    // TCP last ACK
#define W5500_Sn_SR_UDP         0x22    // UDP socket open
#define W5500_Sn_SR_MACRAW      0x42    // MACRAW socket open

// ============================================================
// Default Values
// ============================================================

#define W5500_SOCKET_COUNT      8       // W5500 has 8 sockets
#define W5500_TX_BUF_SIZE       2048    // Default 2KB per socket
#define W5500_RX_BUF_SIZE       2048    // Default 2KB per socket
#define W5500_CHIP_VERSION      0x04    // Expected chip version

// ============================================================
// UDP Header size in RX buffer
// When receiving UDP, first 8 bytes are:
// [4 bytes IP][2 bytes Port][2 bytes Length]
// ============================================================

#define W5500_UDP_HEADER_SIZE   8

#endif // W5500_RAW_H
