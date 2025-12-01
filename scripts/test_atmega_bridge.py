#!/usr/bin/env python3
"""
ATmega328P Bridge Test Script
Gateway LoRa JVTECH v4.1

Script para testar todas as funcionalidades do firmware bridge.
"""

import sys
import argparse
from atmega_protocol import (
    AtmegaBridge, format_mac, format_ip, parse_ip,
    RESP_OK, RESP_ERROR, RESP_NOT_INIT
)


def test_system(bridge: AtmegaBridge) -> bool:
    """Test system commands."""
    print("\n" + "=" * 50)
    print("SYSTEM TESTS")
    print("=" * 50)

    # PING
    print("\n[1] Testing PING...")
    if bridge.ping():
        print("    OK - PONG received")
    else:
        print("    FAILED - No PONG")
        return False

    # GET_VERSION
    print("\n[2] Testing GET_VERSION...")
    version = bridge.get_version()
    if version:
        print(f"    OK - Firmware version: {version}")
    else:
        print("    FAILED - Could not get version")
        return False

    # GET_STATUS
    print("\n[3] Testing GET_STATUS...")
    status = bridge.get_status()
    if status:
        print(f"    OK - Status received:")
        print(f"        Ethernet initialized: {status['eth_initialized']}")
        print(f"        Ethernet link up:     {status['eth_link_up']}")
        print(f"        RTC present:          {status['rtc_present']}")
        h, m, s = status['uptime_hours'], status['uptime_minutes'], status['uptime_seconds']
        print(f"        Uptime:               {h}h {m}m {s}s ({status['uptime_total_seconds']}s)")
        print(f"        Free RAM:             {status['free_ram']} bytes")
    else:
        print("    FAILED - Could not get status")
        return False

    return True


def test_ethernet(bridge: AtmegaBridge) -> bool:
    """Test Ethernet commands."""
    print("\n" + "=" * 50)
    print("ETHERNET TESTS")
    print("=" * 50)

    # LINK STATUS
    print("\n[1] Testing ETH_LINK_STATUS...")
    link = bridge.eth_get_link_status()
    if link is not None:
        print(f"    OK - Link status: {'UP' if link else 'DOWN'}")
    else:
        print("    FAILED - Could not get link status")

    # GET MAC
    print("\n[2] Testing ETH_GET_MAC...")
    mac = bridge.eth_get_mac()
    if mac:
        print(f"    OK - MAC address: {format_mac(mac)}")
    else:
        print("    FAILED - Could not get MAC")

    # GET IP
    print("\n[3] Testing ETH_GET_IP...")
    ip = bridge.eth_get_ip()
    if ip:
        print(f"    OK - IP address: {format_ip(ip)}")
    else:
        print("    FAILED - Could not get IP")

    return True


def test_i2c(bridge: AtmegaBridge) -> bool:
    """Test I2C commands."""
    print("\n" + "=" * 50)
    print("I2C TESTS")
    print("=" * 50)

    # I2C SCAN
    print("\n[1] Testing I2C_SCAN...")
    devices = bridge.i2c_scan()
    if devices:
        print(f"    OK - Found {len(devices)} device(s):")
        for addr in devices:
            device_name = ""
            if addr == 0x68:
                device_name = " (RTC DS1307)"
            elif addr == 0x3C or addr == 0x3D:
                device_name = " (OLED Display)"
            print(f"        0x{addr:02X}{device_name}")
    else:
        print("    OK - No I2C devices found (or scan failed)")

    return True


def test_udp_loopback(bridge: AtmegaBridge, server_ip: str, server_port: int) -> bool:
    """Test UDP send/receive with external server."""
    print("\n" + "=" * 50)
    print("UDP TESTS")
    print("=" * 50)

    socket = 0
    local_port = 5000

    print(f"\n[1] Opening UDP socket {socket} on port {local_port}...")
    if bridge.udp_begin(socket, local_port):
        print("    OK - Socket opened")
    else:
        print("    FAILED - Could not open socket")
        return False

    print(f"\n[2] Sending UDP packet to {server_ip}:{server_port}...")
    dest_ip = parse_ip(server_ip)
    sent = bridge.udp_send(socket, dest_ip, server_port, b"Hello from ATmega!")
    if sent > 0:
        print(f"    OK - Sent {sent} bytes")
    else:
        print("    FAILED - Send failed")

    print(f"\n[3] Checking available data...")
    available = bridge.udp_available(socket)
    print(f"    Available: {available} bytes")

    if available > 0:
        print(f"\n[4] Receiving UDP data...")
        result = bridge.udp_recv(socket, 256)
        if result:
            data, port, ip = result
            print(f"    OK - Received {len(data)} bytes from {format_ip(ip)}:{port}")
            print(f"    Data: {data}")
        else:
            print("    FAILED - Receive failed")

    print(f"\n[5] Closing UDP socket...")
    if bridge.udp_close(socket):
        print("    OK - Socket closed")
    else:
        print("    FAILED - Could not close socket")

    return True


def run_all_tests(bridge: AtmegaBridge) -> bool:
    """Run all available tests."""
    results = []

    results.append(("System", test_system(bridge)))
    results.append(("Ethernet", test_ethernet(bridge)))
    results.append(("I2C", test_i2c(bridge)))

    print("\n" + "=" * 50)
    print("TEST SUMMARY")
    print("=" * 50)

    all_passed = True
    for name, passed in results:
        status = "PASS" if passed else "FAIL"
        print(f"  {name}: {status}")
        if not passed:
            all_passed = False

    return all_passed


def interactive_mode(bridge: AtmegaBridge):
    """Interactive command mode."""
    print("\nInteractive mode. Commands:")
    print("  ping        - Send PING")
    print("  version     - Get firmware version")
    print("  status      - Get system status")
    print("  link        - Get Ethernet link status")
    print("  mac         - Get MAC address")
    print("  ip          - Get IP address")
    print("  i2c         - Scan I2C bus")
    print("  quit        - Exit")
    print()

    while True:
        try:
            cmd = input("atmega> ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            break

        if not cmd:
            continue

        parts = cmd.split()
        command = parts[0]

        if command == 'quit' or command == 'exit':
            break
        elif command == 'ping':
            if bridge.ping():
                print("PONG")
            else:
                print("No response")
        elif command == 'version':
            v = bridge.get_version()
            print(f"Version: {v}" if v else "Failed")
        elif command == 'status':
            s = bridge.get_status()
            if s:
                print(f"  Eth init: {s['eth_initialized']}")
                print(f"  Link up:  {s['eth_link_up']}")
                print(f"  RTC:      {s['rtc_present']}")
                print(f"  Uptime:   {s['uptime_hours']}h {s['uptime_minutes']}m {s['uptime_seconds']}s")
                print(f"  RAM free: {s['free_ram']} bytes")
            else:
                print("Failed")
        elif command == 'link':
            link = bridge.eth_get_link_status()
            print(f"Link: {'UP' if link else 'DOWN'}" if link is not None else "Failed")
        elif command == 'mac':
            mac = bridge.eth_get_mac()
            print(f"MAC: {format_mac(mac)}" if mac else "Failed")
        elif command == 'ip':
            ip = bridge.eth_get_ip()
            print(f"IP: {format_ip(ip)}" if ip else "Failed")
        elif command == 'i2c':
            devices = bridge.i2c_scan()
            if devices:
                print(f"Found {len(devices)} device(s): " +
                      ", ".join(f"0x{a:02X}" for a in devices))
            else:
                print("No devices found")
        else:
            print(f"Unknown command: {command}")


def main():
    parser = argparse.ArgumentParser(
        description='ATmega328P Bridge Test Script',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s                    # Run all tests on /dev/ttyUSB0
  %(prog)s -p /dev/ttyUSB1    # Use different port
  %(prog)s -i                 # Interactive mode
  %(prog)s --ping             # Just ping
  %(prog)s --udp 192.168.1.100 5000  # Test UDP with server
'''
    )

    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-i', '--interactive', action='store_true',
                        help='Interactive mode')
    parser.add_argument('--ping', action='store_true',
                        help='Only send ping')
    parser.add_argument('--udp', nargs=2, metavar=('IP', 'PORT'),
                        help='Test UDP with server IP and port')

    args = parser.parse_args()

    bridge = AtmegaBridge(args.port, args.baudrate)

    print(f"Connecting to ATmega328P on {args.port}...")
    if not bridge.connect():
        print("Failed to connect!")
        sys.exit(1)

    print("Connected!")

    try:
        if args.interactive:
            interactive_mode(bridge)
        elif args.ping:
            if bridge.ping():
                print("PING OK - PONG received")
            else:
                print("PING failed")
                sys.exit(1)
        elif args.udp:
            test_udp_loopback(bridge, args.udp[0], int(args.udp[1]))
        else:
            success = run_all_tests(bridge)
            sys.exit(0 if success else 1)
    finally:
        bridge.disconnect()
        print("\nDisconnected.")


if __name__ == '__main__':
    main()
