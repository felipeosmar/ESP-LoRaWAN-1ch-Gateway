# Specification: Ethernet Connectivity and Network Failover

## Goal

Provide reliable dual-network connectivity (Ethernet and WiFi) with intelligent failover based on actual LoRaWAN network server (ChirpStack) reachability, ensuring the gateway maintains packet forwarding capability even when the primary network interface fails.

## User Stories

- As a gateway operator, I want Ethernet to be the primary connection so that I have the most reliable wired connectivity for mission-critical deployments.
- As a gateway operator, I want automatic failover to WiFi when Ethernet/ChirpStack connectivity fails so that packet forwarding continues uninterrupted.
- As a gateway operator, I want to see which network interface is active ("W" or "E") on the display so I know the current connection status at a glance.
- As a gateway operator, I want to configure separate IP settings (DHCP/static) for WiFi and Ethernet so I can use different network segments.
- As a gateway operator, I want the gateway to return to Ethernet after a configurable stability period so it prefers the more reliable wired connection.
- As a gateway operator, I want native DNS resolution via the Ethernet interface so hostname resolution works without WiFi fallback.

## Core Requirements

### Primary Interface Selection
- Ethernet is the default primary interface (change from current WiFi default)
- User can configure primary interface via web UI
- Primary interface is attempted first on boot and after failover recovery

### Application-Layer Health Checking
- Health checks verify actual ChirpStack server connectivity, not just IP-layer ping
- Use Semtech UDP protocol acknowledgments (PUSH_ACK, PULL_ACK) as health indicators
- Failover triggered when no ACK received within the packet transmission cycle timeout
- Health check integrates with existing `UDPForwarder` statistics (`pushAckReceived`, `pullAckReceived`)

### Failover Logic
- Timeout tied to LoRa packet forwarding cycle (configurable, default 30 seconds)
- Immediate switch to secondary interface when primary fails health check
- Log failover events with timestamps for troubleshooting
- Maintain UDP socket continuity during interface switch (restart UDP on new interface)

### Return-to-Primary
- Configurable stability period before returning to primary (default 60 seconds)
- Primary must pass health checks for entire stability period before switch-back
- Prevents rapid oscillation between interfaces

### IP Configuration
- DHCP is default for both interfaces
- Static IP option with separate settings for each interface:
  - IP address, subnet mask, gateway, DNS server
- Configuration stored independently in LittleFS JSON

### DNS Resolution
- Implement native DNS via ATmega/W5500 stack (new protocol command required)
- Remove current WiFi fallback for DNS when Ethernet is active
- DNS cache with 5-minute TTL (already implemented)

### Display Status
- Active interface indicator: "E" for Ethernet, "W" for WiFi
- Notification when failover occurs (temporary message)
- WiFi signal strength (RSSI) shown when WiFi is active
- Updates on both LCD 16x2 and OLED SSD1306

### Web Interface
- Network status dashboard showing both interfaces
- Ethernet configuration page (DHCP/static IP settings)
- Failover configuration (primary interface, timeout, stability period)
- Manual interface override controls

## Reusable Components

### Existing Code to Leverage

| Component | File | Usage |
|-----------|------|-------|
| NetworkManager | `src/network_manager.h/.cpp` | Core failover framework - extend with health checking |
| EthernetAdapter | `src/ethernet_adapter.h/.cpp` | W5500 interface - already implements NetworkInterface |
| WiFiAdapter | `src/wifi_adapter.h/.cpp` | WiFi interface - reference implementation |
| NetworkInterface | `src/network_interface.h` | Abstract interface - no changes needed |
| ATmegaBridge | `src/atmega_bridge.h/.cpp` | Serial protocol - extend for DNS command |
| UDPForwarder | `src/udp_forwarder.h/.cpp` | Health check data source (ACK statistics) |
| Web API handlers | `src/web_server.cpp:1683-1849` | Network API endpoints - extend for new features |
| Display managers | `src/lcd_manager.h`, `src/oled_manager.h` | Display framework - add network status mode |
| Protocol definitions | `src_atmega/include/protocol.h` | ATmega protocol - add DNS command |
| Configuration | `src/config.h` | Defaults - update primary interface default |

### New Components Required

| Component | Reason |
|-----------|--------|
| DNS command in protocol | ATmega protocol lacks DNS resolution command (CMD_DNS_RESOLVE) |
| Health check integration | NetworkManager needs to query UDPForwarder for ACK status |
| Network status display mode | Display managers need new mode for interface indicator |
| Stability timer for return-to-primary | Current implementation uses fixed 5-second check |

## Technical Approach

### Architecture Overview

```
+-------------------+       +-------------------+
|   UDPForwarder    |<----->|  NetworkManager   |
|  (Health Source)  |       |  (Failover Logic) |
+-------------------+       +-------------------+
                                   |
                    +--------------+--------------+
                    |                             |
              +-----v-----+                 +-----v-----+
              | WiFiAdapter|                |EthernetAdapter|
              +-----------+                 +-------------+
                    |                             |
              +-----v-----+                 +-----v-----+
              |  ESP32    |                 | ATmegaBridge |
              |   WiFi    |                 +-------------+
              +-----------+                       |
                                            +-----v-----+
                                            |  ATmega   |
                                            |  W5500    |
                                            +-----------+
```

### Health Check Implementation

1. NetworkManager queries UDPForwarder for last ACK timestamps
2. Compare `lastAckTime` against `failoverTimeout`
3. If no ACK received within timeout, trigger failover
4. Health check runs on `NET_STATUS_CHECK_INTERVAL` (1 second)

### DNS via ATmega Implementation

1. Add `CMD_DNS_RESOLVE` (0x25) to protocol.h
2. Request: hostname string (null-terminated)
3. Response: 4-byte IPv4 address or RSP_ERROR
4. ATmega uses W5500 DNS client library
5. Update `EthernetAdapter::hostByName()` to use bridge command

### Configuration Structure

```json
{
  "network": {
    "wifi_enabled": true,
    "ethernet_enabled": true,
    "primary": "ethernet",
    "failover_enabled": true,
    "failover_timeout": 30000,
    "reconnect_interval": 10000,
    "stability_period": 60000,
    "ethernet": {
      "enabled": true,
      "dhcp": true,
      "static_ip": "192.168.1.100",
      "gateway": "192.168.1.1",
      "subnet": "255.255.255.0",
      "dns": "8.8.8.8",
      "dhcp_timeout": 10000
    },
    "wifi": {
      "dhcp": true,
      "static_ip": "",
      "gateway": "",
      "subnet": "",
      "dns": ""
    }
  }
}
```

### Display Updates

**LCD 16x2 Format:**
```
Line 1: "LORA GW  E 12:34"  (E=Ethernet, W=WiFi, time)
Line 2: "RX:123 TX:45 OK"   (stats)
```

**OLED 128x64 Format:**
- Header shows interface icon (E/W) with connection status
- WiFi mode shows signal strength bars
- Failover notification overlay (2-second display)

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/network/status` | GET | Full network status JSON (existing) |
| `/api/network/config` | GET/POST | Network configuration (existing, extend) |
| `/api/network/force` | POST | Force interface selection (existing) |
| `/api/network/reconnect` | POST | Reconnect all interfaces (existing) |
| `/api/network/health` | GET | Health check status (new) |

### Default Configuration Changes

In `config.h`:
- `NET_PRIMARY_WIFI_DEFAULT` changes from `true` to `false`
- Add `NET_STABILITY_PERIOD_DEFAULT 60000`

## Out of Scope

- VPN or secure tunnel support
- Multiple network server failover (single ChirpStack server only)
- IPv6 support (IPv4 only for this release)
- Network bandwidth monitoring or QoS
- Ethernet-only operation mode (WiFi remains available as backup)
- Advanced routing or multi-path networking
- HTTPS for web interface
- mDNS/Bonjour discovery

## Success Criteria

1. **Failover Speed**: Interface switch completes within 5 seconds of health check failure
2. **Recovery Reliability**: Gateway successfully returns to primary after stability period in 95% of tests
3. **Packet Loss During Failover**: Maximum 2 LoRa packets lost during interface transition
4. **DNS Resolution**: Hostnames resolve via Ethernet without WiFi fallback
5. **Display Accuracy**: Interface indicator updates within 1 second of switch
6. **Configuration Persistence**: All network settings survive power cycle
7. **Web UI Responsiveness**: Network configuration page loads within 2 seconds

## Testing Requirements

### Unit Tests
- NetworkManager failover state machine transitions
- Health check timeout calculations
- DNS resolution via ATmega bridge
- Configuration save/load cycle

### Integration Tests
- Full failover cycle: Ethernet -> WiFi -> Ethernet
- Simultaneous interface initialization
- UDP socket migration during failover
- Web API configuration changes

### Hardware Tests
- Ethernet cable disconnect/reconnect scenarios
- WiFi AP power cycle scenarios
- ChirpStack server unreachable simulation
- Long-duration stability test (24+ hours)

### Display Tests
- Interface indicator updates on both LCD and OLED
- Failover notification visibility
- WiFi signal strength accuracy

## Implementation Notes

### Memory Constraints
- ATmega328P has 2KB RAM; DNS response buffer must be minimal
- ESP32 can handle larger buffers for UDP and configuration

### Timing Constraints
- Health check must not block LoRa packet reception
- Interface switch should be non-blocking where possible
- Stability timer runs in NetworkManager::update() loop

### Backwards Compatibility
- Existing WiFi-only configurations continue to work
- Missing Ethernet config uses DHCP defaults
- API responses maintain existing field structure
