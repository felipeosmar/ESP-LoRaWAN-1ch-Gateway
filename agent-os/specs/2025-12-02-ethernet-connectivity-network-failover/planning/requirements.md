# Spec Requirements: Ethernet Connectivity and Network Failover

## Initial Description

This spec covers items 1-5 from the v1.0 roadmap:
- Ethernet Connectivity - Enable W5500 Ethernet module support on the ATmega328P bridge
- Network Failover - Implement automatic switching between WiFi and Ethernet connections
- Failover Status Display - Update LCD/OLED displays and web interface to show network status
- Ethernet Configuration UI - Add Ethernet settings page to web interface
- Connection Health Monitoring - Implement periodic connectivity checks to the network server

The goal is to provide reliable dual-network connectivity with intelligent failover based on actual LoRaWAN network server (ChirpStack) reachability, not just IP-layer connectivity.

## Requirements Discussion

### First Round Questions

**Q1:** Should Ethernet or WiFi be the primary interface by default?
**Answer:** Ethernet should be the default primary interface (not WiFi). This aligns with enterprise deployment patterns where wired connections are preferred for reliability.

**Q2:** What should trigger the failover mechanism?
**Answer:** Failover should be activated when communication with the network server (ChirpStack) is not possible. This means active connectivity testing to the LoRaWAN network server endpoint, not just generic IP connectivity or ping tests. This is application-layer health checking.

**Q3:** What timeout period should be used before triggering failover?
**Answer:** The failover timeout should match the data packet transmission time (the LoRa packet forwarding cycle). This ties the failover detection directly to the gateway's primary function - if packets cannot be forwarded within a normal transmission window, failover should occur.

**Q4:** How should the gateway determine when to return to the primary interface?
**Answer:** Return to primary should be configurable with a longer stability period. This prevents rapid flip-flopping between interfaces when the primary connection is unstable.

**Q5:** Should IP addressing be DHCP or static, and should settings be shared between interfaces?
**Answer:** DHCP is the default with static IP as an option. WiFi and Ethernet should have separate, independent IP configurations. This allows different network segments or addressing schemes for each interface.

**Q6:** What network status information should be shown on the display?
**Answer:** Add all of the following:
- Icon showing WiFi vs Ethernet ("W" or "E") to indicate active interface
- Network switch notifications when failover occurs
- WiFi signal strength indicator when WiFi is the active interface

**Q7:** How should DNS resolution be handled with Ethernet?
**Answer:** Must implement proper DNS resolution via the ATmega/W5500 stack. DNS queries should go through the active interface's configured DNS server, not fall back to WiFi when Ethernet is active.

**Q8:** What features are explicitly out of scope?
**Answer:** The following are explicitly excluded from this spec:
- VPN/secure tunnel support
- Multiple network server failover (only single ChirpStack server)
- IPv6 support (IPv4 only)
- Network bandwidth monitoring

### Existing Code to Reference

No similar existing features were explicitly identified by the user. However, based on the tech stack:

- **ATmega328P firmware** - The ATmega bridge code that currently handles peripheral management will need to be extended for W5500 Ethernet support
- **ESP32 WiFi implementation** - The existing WiFi connectivity code provides patterns for network state management
- **Web configuration interface** - Existing configuration pages provide UI patterns for the Ethernet settings page
- **Display drivers** - Current LCD/OLED code provides the framework for status display updates

### Follow-up Questions

No additional follow-up questions were required. The user's answers were comprehensive and addressed all critical aspects of the implementation.

## Visual Assets

### Files Provided:
No visual assets provided.

### Visual Insights:
N/A - No visual mockups or wireframes were submitted for this spec.

## Requirements Summary

### Functional Requirements

**Ethernet Connectivity (ATmega/W5500)**
- Enable W5500 Ethernet module support on the ATmega328P bridge processor
- Implement DHCP client for automatic IP configuration (default mode)
- Support static IP configuration as alternative to DHCP
- Implement DNS resolution through the W5500 stack
- Separate IP configuration storage for Ethernet (independent from WiFi settings)

**Network Failover Logic**
- Set Ethernet as the default primary interface
- Implement application-layer health checking against ChirpStack server (not just ping/IP connectivity)
- Trigger failover when LoRaWAN packet forwarding to ChirpStack fails
- Use packet transmission cycle timing as the failover timeout threshold
- Implement configurable stability period before returning to primary interface
- Prevent rapid interface switching (hysteresis/debounce mechanism)

**Connection Health Monitoring**
- Periodic connectivity verification to the configured LoRaWAN network server
- Health check integrated with the packet forwarding cycle
- Distinguish between network-layer failures and application-layer failures
- Log failover events for troubleshooting

**Display Status Updates**
- Show active interface indicator: "W" for WiFi, "E" for Ethernet
- Display notification when network switch occurs
- Show WiFi signal strength (RSSI) when WiFi is active interface
- Update both LCD 16x2 and OLED SSD1306 displays

**Web Configuration Interface**
- Add Ethernet configuration page with:
  - DHCP/Static IP mode toggle
  - Static IP address, subnet mask, gateway, DNS fields
  - Interface status display
- Add failover configuration section with:
  - Primary interface selection
  - Stability period configuration for return-to-primary
- Display current network status on dashboard

### Reusability Opportunities

- Existing WiFi state machine patterns for network connection management
- Current web interface styling and form patterns for configuration pages
- Display update routines for LCD and OLED status rendering
- JSON configuration storage patterns using LittleFS
- ESPAsyncWebServer API patterns for new configuration endpoints

### Scope Boundaries

**In Scope:**
- W5500 Ethernet driver implementation on ATmega328P
- ESP32-to-ATmega communication protocol for Ethernet operations
- DHCP client for Ethernet interface
- Static IP configuration for Ethernet interface
- DNS resolution via ATmega/W5500
- ChirpStack connectivity-based failover detection
- Configurable failover timeout (tied to packet transmission cycle)
- Configurable return-to-primary stability period
- Active interface indication on displays ("W" or "E")
- Failover event notifications on displays
- WiFi signal strength display when WiFi active
- Ethernet configuration page in web interface
- Failover settings in web interface
- Independent IP configurations for WiFi and Ethernet

**Out of Scope:**
- VPN or secure tunnel support
- Multiple network server failover (single ChirpStack server only)
- IPv6 support (IPv4 only for this release)
- Network bandwidth monitoring or QoS
- Ethernet-only operation mode (WiFi remains available)
- Advanced routing or multi-path networking

### Technical Considerations

**Hardware Architecture**
- W5500 Ethernet module is connected to ATmega328P, not directly to ESP32
- ESP32 must communicate with ATmega328P to perform Ethernet operations
- Requires defining/extending serial protocol between ESP32 and ATmega328P
- ATmega328P has limited memory; Ethernet stack must be memory-efficient

**Protocol Requirements**
- Semtech UDP Packet Forwarder v2 protocol used for ChirpStack communication
- Health checking should verify actual packet forwarding capability
- DNS required for resolving ChirpStack server hostname

**Configuration Storage**
- Ethernet settings stored in LittleFS JSON configuration
- Separate configuration objects for WiFi and Ethernet
- Failover preferences stored in system configuration

**Timing Considerations**
- Failover detection tied to LoRa packet forwarding cycle
- Must not block LoRa packet reception during failover operations
- Stability period for return-to-primary prevents oscillation

**Display Constraints**
- LCD 16x2 has limited character space; use single-character indicators
- OLED can show more detailed status but should remain consistent with LCD
- Status updates should not interfere with primary gateway status display
