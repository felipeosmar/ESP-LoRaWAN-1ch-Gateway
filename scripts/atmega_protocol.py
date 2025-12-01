#!/usr/bin/env python3
"""
ATmega328P Bridge Protocol Library
Gateway LoRa JVTECH v4.1

Biblioteca para comunicacao com o firmware bridge do ATmega328P.
"""

import serial
import struct
import time
from typing import Optional, Tuple, List

# Protocol constants
FRAME_START = 0xAA
FRAME_END = 0x55

# Command categories
CMD_SYSTEM_BASE = 0x00
CMD_ETHERNET_BASE = 0x10
CMD_UDP_BASE = 0x20
CMD_TCP_BASE = 0x30
CMD_RTC_BASE = 0x40
CMD_I2C_BASE = 0x50
CMD_BUZZER_BASE = 0x60

# System commands (0x00-0x0F) - Must match firmware protocol.h
CMD_PING = 0x00
CMD_GET_VERSION = 0x01
CMD_RESET = 0x02
CMD_GET_STATUS = 0x03
CMD_SET_LED = 0x04

# Ethernet commands (0x10-0x1F)
CMD_ETH_INIT = 0x10
CMD_ETH_STATUS = 0x11
CMD_ETH_GET_MAC = 0x12
CMD_ETH_SET_MAC = 0x13
CMD_ETH_GET_IP = 0x14
CMD_ETH_SET_IP = 0x15
CMD_ETH_DHCP = 0x16
CMD_ETH_LINK_STATUS = 0x17

# UDP commands (0x20-0x2F)
CMD_UDP_BEGIN = 0x20
CMD_UDP_CLOSE = 0x21
CMD_UDP_SEND = 0x22
CMD_UDP_RECV = 0x23
CMD_UDP_AVAILABLE = 0x24

# TCP commands (0x30-0x3F)
CMD_TCP_CONNECT = 0x30
CMD_TCP_LISTEN = 0x31
CMD_TCP_CLOSE = 0x32
CMD_TCP_SEND = 0x33
CMD_TCP_RECV = 0x34
CMD_TCP_AVAILABLE = 0x35
CMD_TCP_STATUS = 0x36

# RTC commands (0x40-0x4F)
CMD_RTC_GET_TIME = 0x40
CMD_RTC_SET_TIME = 0x41
CMD_RTC_GET_DATE = 0x42
CMD_RTC_SET_DATE = 0x43
CMD_RTC_GET_DATETIME = 0x44
CMD_RTC_SET_DATETIME = 0x45
CMD_RTC_GET_TEMP = 0x46

# I2C commands (0x50-0x5F)
CMD_I2C_SCAN = 0x50
CMD_I2C_WRITE = 0x51
CMD_I2C_READ = 0x52
CMD_I2C_WRITE_READ = 0x53

# Note: Buzzer commands removed - buzzer is connected to ESP32, not ATmega

# Response codes
RESP_OK = 0x00
RESP_ERROR = 0x01
RESP_UNKNOWN_CMD = 0x02
RESP_INVALID_PARAM = 0x03
RESP_TIMEOUT = 0x04
RESP_BUSY = 0x05
RESP_NOT_INIT = 0x06


def crc8(data: bytes) -> int:
    """Calculate CRC8 checksum (matching ATmega firmware)."""
    crc = 0xFF  # Initial value
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x31  # Polynomial x^8 + x^5 + x^4 + 1
            else:
                crc <<= 1
            crc &= 0xFF
    return crc


class AtmegaBridge:
    """Class for communicating with ATmega328P bridge firmware."""

    def __init__(self, port: str = '/dev/ttyUSB0', baudrate: int = 115200, timeout: float = 2.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser: Optional[serial.Serial] = None

    def connect(self, wait_boot: bool = True) -> bool:
        """Open serial connection."""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            if wait_boot:
                time.sleep(2.0)  # Wait for ATmega to boot (DTR toggles reset)
            else:
                time.sleep(0.1)  # Just wait for connection to stabilize
            self.ser.reset_input_buffer()
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    def disconnect(self):
        """Close serial connection."""
        if self.ser:
            self.ser.close()
            self.ser = None

    def send_command(self, cmd: int, data: bytes = b'') -> Tuple[bool, bytes]:
        """
        Send command and receive response.

        Returns:
            Tuple of (success, response_data)
        """
        if not self.ser:
            return False, b''

        # Build frame
        # CRC is calculated over DATA only (not header)
        length = len(data)
        frame = bytes([FRAME_START, cmd, (length >> 8) & 0xFF, length & 0xFF])
        frame += data
        frame += bytes([crc8(data), FRAME_END])

        # Send
        self.ser.reset_input_buffer()
        self.ser.write(frame)
        self.ser.flush()

        # Receive response
        try:
            # Wait for start byte
            start = self.ser.read(1)
            if not start or start[0] != FRAME_START:
                return False, b''

            # Read header (cmd, len_h, len_l)
            header = self.ser.read(3)
            if len(header) < 3:
                return False, b''

            resp_cmd = header[0]
            resp_len = (header[1] << 8) | header[2]

            # Read data
            resp_data = self.ser.read(resp_len) if resp_len > 0 else b''

            # Read CRC and end byte
            footer = self.ser.read(2)
            if len(footer) < 2:
                return False, b''

            # Verify end byte
            if footer[1] != FRAME_END:
                return False, b''

            # Verify CRC (calculated over data only, not header)
            calc_crc = crc8(resp_data)
            if calc_crc != footer[0]:
                return False, b''

            return True, resp_data

        except Exception as e:
            print(f"Receive error: {e}")
            return False, b''

    # ================== System Commands ==================

    def ping(self) -> bool:
        """Send PING, expect PONG."""
        success, data = self.send_command(CMD_PING)
        # Response format: [status, "PONG"]
        if success and len(data) >= 5 and data[0] == RESP_OK and data[1:5] == b'PONG':
            return True
        return False

    def get_version(self) -> Optional[str]:
        """Get firmware version."""
        success, data = self.send_command(CMD_GET_VERSION)
        # Response: [status, major, minor, patch] as bytes
        if success and len(data) >= 4 and data[0] == RESP_OK:
            return f"{data[1]}.{data[2]}.{data[3]}"
        return None

    def get_status(self) -> Optional[dict]:
        """Get system status."""
        success, data = self.send_command(CMD_GET_STATUS)
        # Response: [status, eth_init, eth_link, rtc_init, hours, mins, secs, ram_h, ram_l]
        if success and len(data) >= 9 and data[0] == RESP_OK:
            uptime_secs = data[4] * 3600 + data[5] * 60 + data[6]
            return {
                'eth_initialized': bool(data[1]),
                'eth_link_up': bool(data[2]),
                'rtc_present': bool(data[3]),
                'uptime_hours': data[4],
                'uptime_minutes': data[5],
                'uptime_seconds': data[6],
                'uptime_total_seconds': uptime_secs,
                'free_ram': (data[7] << 8) | data[8]
            }
        return None

    # ================== Ethernet Commands ==================

    def eth_init(self, mac: bytes = None, ip: bytes = None,
                 gateway: bytes = None, subnet: bytes = None) -> bool:
        """Initialize Ethernet with optional configuration."""
        data = b''
        if mac:
            data += mac[:6].ljust(6, b'\x00')
        else:
            data += bytes(6)
        if ip:
            data += ip[:4].ljust(4, b'\x00')
        else:
            data += bytes(4)
        if gateway:
            data += gateway[:4].ljust(4, b'\x00')
        else:
            data += bytes(4)
        if subnet:
            data += subnet[:4].ljust(4, b'\x00')
        else:
            data += bytes(4)

        success, resp = self.send_command(CMD_ETH_INIT, data)
        return success and len(resp) > 0 and resp[0] == RESP_OK

    def eth_get_link_status(self) -> Optional[bool]:
        """Get Ethernet link status."""
        success, data = self.send_command(CMD_ETH_LINK_STATUS)
        if success and len(data) >= 2:
            return bool(data[1])
        return None

    def eth_get_mac(self) -> Optional[bytes]:
        """Get MAC address."""
        success, data = self.send_command(CMD_ETH_GET_MAC)
        if success and len(data) >= 7 and data[0] == RESP_OK:
            return data[1:7]
        return None

    def eth_get_ip(self) -> Optional[bytes]:
        """Get IP address."""
        success, data = self.send_command(CMD_ETH_GET_IP)
        if success and len(data) >= 5 and data[0] == RESP_OK:
            return data[1:5]
        return None

    # ================== UDP Commands ==================

    def udp_begin(self, socket: int, port: int) -> bool:
        """Open UDP socket."""
        data = bytes([socket, (port >> 8) & 0xFF, port & 0xFF])
        success, resp = self.send_command(CMD_UDP_BEGIN, data)
        return success and len(resp) > 0 and resp[0] == RESP_OK

    def udp_send(self, socket: int, dest_ip: bytes, dest_port: int, payload: bytes) -> int:
        """Send UDP packet. Returns bytes sent."""
        data = bytes([socket]) + dest_ip[:4] + bytes([(dest_port >> 8) & 0xFF, dest_port & 0xFF])
        data += payload
        success, resp = self.send_command(CMD_UDP_SEND, data)
        if success and len(resp) >= 3 and resp[0] == RESP_OK:
            return (resp[1] << 8) | resp[2]
        return 0

    def udp_recv(self, socket: int, max_len: int = 256) -> Optional[Tuple[bytes, int, bytes]]:
        """
        Receive UDP packet.
        Returns: (data, port, ip) or None
        """
        data = bytes([socket, (max_len >> 8) & 0xFF, max_len & 0xFF])
        success, resp = self.send_command(CMD_UDP_RECV, data)
        if success and len(resp) >= 9 and resp[0] == RESP_OK:
            src_ip = resp[1:5]
            src_port = (resp[5] << 8) | resp[6]
            length = (resp[7] << 8) | resp[8]
            payload = resp[9:9+length]
            return payload, src_port, src_ip
        return None

    def udp_available(self, socket: int) -> int:
        """Check bytes available on UDP socket."""
        success, resp = self.send_command(CMD_UDP_AVAILABLE, bytes([socket]))
        if success and len(resp) >= 3 and resp[0] == RESP_OK:
            return (resp[1] << 8) | resp[2]
        return 0

    def udp_close(self, socket: int) -> bool:
        """Close UDP socket."""
        success, resp = self.send_command(CMD_UDP_CLOSE, bytes([socket]))
        return success and len(resp) > 0 and resp[0] == RESP_OK

    # ================== TCP Commands ==================

    def tcp_connect(self, socket: int, dest_ip: bytes, dest_port: int, timeout_ms: int = 5000) -> bool:
        """Connect to TCP server."""
        data = bytes([socket]) + dest_ip[:4]
        data += bytes([(dest_port >> 8) & 0xFF, dest_port & 0xFF])
        data += bytes([(timeout_ms >> 8) & 0xFF, timeout_ms & 0xFF])
        success, resp = self.send_command(CMD_TCP_CONNECT, data)
        return success and len(resp) > 0 and resp[0] == RESP_OK

    def tcp_send(self, socket: int, payload: bytes) -> int:
        """Send TCP data. Returns bytes sent."""
        data = bytes([socket]) + payload
        success, resp = self.send_command(CMD_TCP_SEND, data)
        if success and len(resp) >= 3 and resp[0] == RESP_OK:
            return (resp[1] << 8) | resp[2]
        return 0

    def tcp_recv(self, socket: int, max_len: int = 256) -> Optional[bytes]:
        """Receive TCP data."""
        data = bytes([socket, (max_len >> 8) & 0xFF, max_len & 0xFF])
        success, resp = self.send_command(CMD_TCP_RECV, data)
        if success and len(resp) >= 3 and resp[0] == RESP_OK:
            length = (resp[1] << 8) | resp[2]
            return resp[3:3+length]
        return None

    def tcp_close(self, socket: int) -> bool:
        """Close TCP connection."""
        success, resp = self.send_command(CMD_TCP_CLOSE, bytes([socket]))
        return success and len(resp) > 0 and resp[0] == RESP_OK

    def tcp_connected(self, socket: int) -> bool:
        """Check if TCP is connected using TCP_STATUS command."""
        success, resp = self.send_command(CMD_TCP_STATUS, bytes([socket]))
        # Status response: [RESP_OK, socket_status]
        # Socket status 0x17 = ESTABLISHED
        return success and len(resp) >= 2 and resp[0] == RESP_OK and resp[1] == 0x17

    # ================== I2C Commands ==================

    def i2c_scan(self) -> List[int]:
        """Scan I2C bus, return list of detected addresses."""
        success, data = self.send_command(CMD_I2C_SCAN)
        if success and len(data) >= 2 and data[0] == RESP_OK:
            count = data[1]
            return list(data[2:2+count])
        return []

    # Note: Buzzer commands removed - buzzer is connected to ESP32, not ATmega


def format_mac(mac: bytes) -> str:
    """Format MAC address as string."""
    return ':'.join(f'{b:02X}' for b in mac)


def format_ip(ip: bytes) -> str:
    """Format IP address as string."""
    return '.'.join(str(b) for b in ip)


def parse_ip(ip_str: str) -> bytes:
    """Parse IP string to bytes."""
    parts = ip_str.split('.')
    return bytes([int(p) for p in parts[:4]])


if __name__ == '__main__':
    # Simple test
    bridge = AtmegaBridge('/dev/ttyUSB0')
    if bridge.connect():
        print("Connected!")
        if bridge.ping():
            print("PING OK - PONG received")
        else:
            print("PING failed")
        bridge.disconnect()
    else:
        print("Connection failed")
