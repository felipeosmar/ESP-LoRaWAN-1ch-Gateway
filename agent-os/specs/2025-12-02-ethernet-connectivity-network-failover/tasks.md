# Task Breakdown: Ethernet Connectivity and Network Failover

## Overview

**Total Tasks:** 47
**Estimated Complexity:** High
**Primary Files Affected:**
- `src/network_manager.h/.cpp` - Failover logic and health checking
- `src/ethernet_adapter.h/.cpp` - DNS via ATmega
- `src_atmega/include/protocol.h` - DNS command definition
- `src_atmega/src/main.cpp` - ATmega DNS handler
- `src/lcd_manager.h/.cpp` - Network status display
- `src/oled_manager.h/.cpp` - Network status display
- `src/web_server.cpp` - API extensions
- `src/config.h` - Default configuration updates

## Task List

---

### ATmega Protocol Layer

#### Task Group 1: DNS Protocol Command Implementation
**Dependencies:** None
**Complexity:** Medium
**Specialist:** Embedded Systems Engineer (ATmega)

This task group adds DNS resolution capability to the ATmega328P/W5500 stack, enabling native hostname resolution over Ethernet without WiFi fallback.

- [x] 1.0 Complete ATmega DNS protocol command
  - [x] 1.1 Write 3-5 focused tests for DNS protocol command
    - Test CMD_DNS_RESOLVE request/response packet format
    - Test DNS resolution success with valid hostname
    - Test DNS resolution failure handling (RSP_ERROR)
    - Test DNS timeout behavior
  - [x] 1.2 Add CMD_DNS_RESOLVE (0x25) to protocol.h
    - Add command constant to Ethernet commands section (0x10-0x3F)
    - Document request format: null-terminated hostname string
    - Document response format: 4-byte IPv4 address or RSP_ERROR
    - Reference file: `src_atmega/include/protocol.h`
  - [x] 1.3 Implement DNS handler in ATmega firmware
    - Add case for CMD_DNS_RESOLVE in command dispatcher
    - Use W5500 DNS client library for resolution
    - Implement 5-second DNS timeout
    - Return 4-byte IP on success, RSP_ERROR on failure
    - Memory constraint: minimize buffer usage (ATmega has 2KB RAM)
    - Reference file: `src_atmega/src/main.cpp`
  - [x] 1.4 Add dnsResolve() method to ATmegaBridge class
    - Method signature: `bool dnsResolve(const char* hostname, IPAddress& result)`
    - Send CMD_DNS_RESOLVE with hostname
    - Parse 4-byte IP response
    - Handle timeout and error responses
    - Reference file: `src/atmega_bridge.h`, `src/atmega_bridge.cpp`
  - [x] 1.5 Update EthernetAdapter::hostByName() to use bridge DNS
    - Call `_bridge.dnsResolve()` instead of WiFi fallback
    - Maintain existing DNS cache (5-minute TTL already implemented)
    - Reference file: `src/ethernet_adapter.cpp`
  - [x] 1.6 Ensure DNS protocol tests pass
    - Run ONLY the 3-5 tests written in 1.1
    - Verify ATmega firmware compiles within memory limits
    - Do NOT run entire test suite

**Acceptance Criteria:**
- CMD_DNS_RESOLVE (0x25) added to protocol.h
- ATmega handles DNS requests via W5500 stack
- EthernetAdapter resolves hostnames natively without WiFi
- DNS cache continues to work with 5-minute TTL
- ATmega firmware fits within 2KB RAM constraint

---

### Network Manager Core

#### Task Group 2: Application-Layer Health Checking
**Dependencies:** Task Group 1
**Complexity:** High
**Specialist:** Backend/Embedded Engineer (ESP32)

This task group implements ChirpStack connectivity-based health checking using Semtech UDP protocol acknowledgments instead of simple ping/IP checks.

- [x] 2.0 Complete application-layer health check integration
  - [x] 2.1 Write 4-6 focused tests for health check logic
    - Test health check triggers failover when no ACK within timeout
    - Test health check passes when ACK received within window
    - Test integration with UDPForwarder statistics
    - Test failover does not trigger during normal operation
    - Test health check runs at NET_STATUS_CHECK_INTERVAL (1 second)
  - [x] 2.2 Extend NetworkManagerConfig struct for health check settings
    - Add `healthCheckEnabled` field (default true)
    - Add `stabilityPeriod` field (default 60000ms)
    - Update loadConfig() to parse new fields
    - Update saveConfig() to serialize new fields
    - Reference file: `src/network_manager.h`, `src/network_manager.cpp`
  - [x] 2.3 Add health check query interface to UDPForwarder
    - Add method: `uint32_t getLastAckTime()` returning `stats.lastAckTime`
    - Add method: `bool isHealthy(uint32_t timeout)` comparing lastAckTime to timeout
    - Ensure pushAckReceived and pullAckReceived update lastAckTime
    - Reference file: `src/udp_forwarder.h`, `src/udp_forwarder.cpp`
  - [x] 2.4 Implement health-check-based failover in NetworkManager
    - Query UDPForwarder::isHealthy() in checkFailover()
    - Trigger failover when no ACK within `failoverTimeout` (default 30s)
    - Replace simple isConnected() check with application-layer health
    - Log health check failures with timestamps
    - Reference file: `src/network_manager.cpp`
  - [x] 2.5 Ensure health check tests pass
    - Run ONLY the 4-6 tests written in 2.1
    - Verify health check does not block LoRa packet reception
    - Do NOT run entire test suite

**Acceptance Criteria:**
- Health checks verify ChirpStack connectivity via ACK statistics
- Failover triggers based on application-layer health, not just IP connectivity
- Health check runs every 1 second without blocking LoRa reception
- Failover timeout configurable (default 30 seconds)

---

#### Task Group 3: Return-to-Primary with Stability Period
**Dependencies:** Task Group 2
**Complexity:** Medium
**Specialist:** Backend/Embedded Engineer (ESP32)

This task group implements configurable stability-period-based return-to-primary to prevent rapid interface oscillation.

- [x] 3.0 Complete stability period implementation
  - [x] 3.1 Write 3-5 focused tests for stability period logic
    - Test return to primary occurs after stability period
    - Test return to primary aborted if primary fails during stability period
    - Test stability period resets on primary failure
    - Test configurable stability period (default 60s)
  - [x] 3.2 Add NET_STABILITY_PERIOD_DEFAULT to config.h
    - Define `NET_STABILITY_PERIOD_DEFAULT 60000` (60 seconds)
    - Reference file: `src/config.h`
  - [x] 3.3 Implement stability timer in NetworkManager
    - Add `_primaryStableStart` member variable for timing
    - Track when primary interface becomes healthy
    - Reset timer if primary fails health check
    - Use stabilityPeriod from config (not hardcoded 5000ms)
    - Reference file: `src/network_manager.cpp`
  - [x] 3.4 Update checkFailover() return-to-primary logic
    - Replace hardcoded 5-second check with configurable stabilityPeriod
    - Primary must pass health checks for ENTIRE stability period
    - Log return-to-primary events with timestamps
    - Reference file: `src/network_manager.cpp`
  - [x] 3.5 Ensure stability period tests pass
    - Run ONLY the 3-5 tests written in 3.1
    - Do NOT run entire test suite

**Acceptance Criteria:**
- Return-to-primary waits for full stability period (default 60s)
- Timer resets if primary fails during stability period
- Stability period is configurable via web UI and JSON config
- Prevents rapid oscillation between interfaces

---

#### Task Group 4: Primary Interface Default Change
**Dependencies:** None
**Complexity:** Low
**Specialist:** Backend/Embedded Engineer (ESP32)

This task group changes the default primary interface from WiFi to Ethernet.

- [x] 4.0 Complete primary interface default change
  - [x] 4.1 Write 2 focused tests for default configuration
    - Test default config has Ethernet as primary
    - Test existing WiFi-only configs continue to work (backwards compatibility)
  - [x] 4.2 Update NET_PRIMARY_WIFI_DEFAULT in config.h
    - Change from `true` to `false` (Ethernet becomes default primary)
    - Reference file: `src/config.h`
  - [x] 4.3 Update NetworkManager constructor default
    - Change `_config.primary = PrimaryInterface::WIFI` to `PrimaryInterface::ETHERNET`
    - Reference file: `src/network_manager.cpp`
  - [x] 4.4 Ensure backwards compatibility
    - Verify loadConfig() correctly parses "wifi" primary from existing configs
    - Missing primary field defaults to Ethernet
    - Reference file: `src/network_manager.cpp`
  - [x] 4.5 Ensure default configuration tests pass
    - Run ONLY the 2 tests written in 4.1
    - Do NOT run entire test suite

**Acceptance Criteria:**
- Fresh installations default to Ethernet as primary
- Existing configurations continue to work unchanged
- "wifi" primary value still respected when explicitly set

---

### Display Layer

#### Task Group 5: Network Status Display Updates
**Dependencies:** Task Groups 2, 3 (COMPLETED)
**Complexity:** Medium
**Specialist:** UI/Embedded Engineer

This task group adds network interface indicators and failover notifications to both LCD 16x2 and OLED SSD1306 displays.

- [x] 5.0 Complete display network status updates
  - [x] 5.1 Write 4-6 focused tests for display updates
    - Test LCD shows "E" for Ethernet active
    - Test LCD shows "W" for WiFi active
    - Test OLED shows interface indicator in header
    - Test failover notification displays for 2 seconds
    - Test WiFi signal strength shown when WiFi active
  - [x] 5.2 Add network interface indicator to LCD display
    - Update showStatus() to include "E" or "W" indicator
    - Format: `"LORA GW  E 12:34"` (E=Ethernet) or `"LORA GW  W 12:34"` (W=WiFi)
    - Query NetworkManager for active interface type
    - Reference file: `src/lcd_manager.h`, `src/lcd_manager.cpp`
  - [x] 5.3 Add network interface indicator to OLED display
    - Update header area to show interface icon (E/W)
    - Show connection status alongside indicator
    - When WiFi active, show signal strength bars
    - Reference file: `src/oled_manager.h`, `src/oled_manager.cpp`
  - [x] 5.4 Implement failover notification overlay
    - Add showFailoverNotification(const char* fromIface, const char* toIface) method
    - Display for 2 seconds then return to normal mode
    - Implement for both LCD and OLED
    - Called by NetworkManager when failover occurs
    - Reference files: `src/lcd_manager.cpp`, `src/oled_manager.cpp`
  - [x] 5.5 Add display callback to NetworkManager for failover events
    - Add callback registration method: `setFailoverCallback(void (*callback)(const char*, const char*))`
    - Call callback when switchToInterface() executes failover
    - Reference file: `src/network_manager.h`, `src/network_manager.cpp`
  - [x] 5.6 Ensure display update tests pass
    - Run ONLY the 4-6 tests written in 5.1
    - Verify indicator updates within 1 second of interface switch
    - Do NOT run entire test suite

**Acceptance Criteria:**
- LCD shows "E" or "W" to indicate active interface
- OLED shows interface icon with status in header
- Failover notification visible for 2 seconds on both displays
- WiFi signal strength shown when WiFi is active
- Interface indicator updates within 1 second of switch

---

### Web Interface Layer

#### Task Group 6: Network Health API Endpoint
**Dependencies:** Task Group 2
**Complexity:** Low
**Specialist:** Backend/API Engineer

This task group adds the `/api/network/health` endpoint to expose health check status.

- [x] 6.0 Complete network health API endpoint
  - [x] 6.1 Write 3-4 focused tests for health API endpoint
    - Test GET /api/network/health returns valid JSON
    - Test response includes health status, lastAckTime, timeout
    - Test response includes failover state
    - Test endpoint returns 503 when NetworkManager unavailable
  - [x] 6.2 Add getHealthJson() method to NetworkManager
    - Return JSON with: `healthy`, `lastAckTime`, `failoverTimeout`, `failoverActive`, `stabilityPeriod`, `primaryStableFor`
    - Query UDPForwarder for ACK statistics
    - Reference file: `src/network_manager.h`, `src/network_manager.cpp`
  - [x] 6.3 Implement handleNetworkHealth() handler in WebServerManager
    - Route: GET `/api/network/health`
    - Call networkManager->getHealthJson()
    - Follow existing handler patterns from handleNetworkStatus()
    - Reference file: `src/web_server.cpp`
  - [x] 6.4 Register route in setupRoutes()
    - Add GET route for `/api/network/health`
    - Reference file: `src/web_server.cpp`
  - [x] 6.5 Ensure health API tests pass
    - Run ONLY the 3-4 tests written in 6.1
    - Do NOT run entire test suite

**Acceptance Criteria:**
- GET /api/network/health returns health check status
- Response includes lastAckTime, failover state, stability info
- Follows existing API response patterns
- Loads within 2 seconds

---

#### Task Group 7: Extended Network Configuration API
**Dependencies:** Task Groups 2, 3, 4 (COMPLETED)
**Complexity:** Medium
**Specialist:** Backend/API Engineer

This task group extends existing network configuration APIs to support stability period and WiFi static IP settings.

- [x] 7.0 Complete extended network configuration API
  - [x] 7.1 Write 4-5 focused tests for extended configuration API
    - Test GET /api/network/config includes stability_period
    - Test POST /api/network/config accepts stability_period
    - Test WiFi static IP configuration via API
    - Test configuration persists after power cycle
  - [x] 7.2 Extend handleNetworkConfig() response
    - Add `stability_period` to response JSON
    - Add `wifi` object with static IP settings (dhcp, static_ip, gateway, subnet, dns)
    - Reference file: `src/web_server.cpp`
  - [x] 7.3 Extend handleNetworkConfigPost() to accept new fields
    - Parse `stability_period` and update config
    - Parse `wifi.dhcp`, `wifi.static_ip`, `wifi.gateway`, `wifi.subnet`, `wifi.dns`
    - Store WiFi static IP settings in configuration
    - Reference file: `src/web_server.cpp`
  - [x] 7.4 Add WiFi static IP configuration to WiFiAdapter
    - Add WiFiConfig struct similar to EthernetConfig
    - Implement static IP support when `dhcp = false`
    - Reference file: `src/wifi_adapter.h`, `src/wifi_adapter.cpp`
  - [x] 7.5 Update loadConfig() and saveConfig() for WiFi settings
    - Parse/serialize wifi.dhcp, wifi.static_ip, etc. from JSON
    - Reference file: `src/network_manager.cpp`
  - [x] 7.6 Ensure extended configuration API tests pass
    - Run ONLY the 4-5 tests written in 7.1
    - Verify settings survive power cycle
    - Do NOT run entire test suite

**Acceptance Criteria:**
- API supports configuring stability_period
- API supports WiFi static IP configuration
- Both interfaces have independent IP settings
- Configuration persists in LittleFS JSON

---

#### Task Group 8: Web UI Network Dashboard Updates
**Dependencies:** Task Groups 6, 7 (COMPLETED)
**Complexity:** Medium
**Specialist:** Frontend/UI Engineer

This task group updates the web interface to display health check status and provide extended configuration options.

- [x] 8.0 Complete web UI network dashboard updates
  - [x] 8.1 Write 3-4 focused tests for web UI updates
    - Test network status dashboard shows both interfaces
    - Test health check status displayed
    - Test failover configuration form works
    - Test interface override controls function
  - [x] 8.2 Update network status dashboard
    - Show both WiFi and Ethernet status side-by-side
    - Display active interface with visual indicator
    - Show health check status (healthy/unhealthy, last ACK time)
    - Show failover state and stability timer countdown
    - Reference file: `data/web/index.html`
  - [x] 8.3 Add failover configuration section
    - Primary interface selection dropdown (WiFi/Ethernet)
    - Failover timeout input (milliseconds)
    - Stability period input (milliseconds)
    - Enable/disable failover toggle
    - Reference file: `data/web/index.html`
  - [x] 8.4 Add WiFi static IP configuration form
    - DHCP/Static IP toggle
    - Static IP, gateway, subnet, DNS input fields
    - Mirror existing Ethernet configuration form layout
    - Reference file: `data/web/index.html`
  - [x] 8.5 Add manual interface override controls
    - Button/dropdown to force WiFi, force Ethernet, or Auto mode
    - Call POST /api/network/force
    - Display current mode (Auto/Forced WiFi/Forced Ethernet)
    - Reference file: `data/web/index.html`, `data/web/app.js`
  - [x] 8.6 Ensure web UI tests pass
    - Run ONLY the 3-4 tests written in 8.1
    - Verify page loads within 2 seconds
    - Do NOT run entire test suite

**Acceptance Criteria:**
- Dashboard shows both interface statuses
- Health check status visible
- Failover and stability period configurable
- WiFi static IP configurable
- Interface override controls work
- Page loads within 2 seconds

---

### Integration and Testing

#### Task Group 9: Integration Testing and Gap Analysis
**Dependencies:** Task Groups 1-8
**Complexity:** Medium
**Specialist:** QA/Test Engineer

This task group reviews all tests from previous groups and fills critical coverage gaps.

- [x] 9.0 Complete integration testing and gap analysis
  - [x] 9.1 Review tests from Task Groups 1-8
    - Review DNS protocol tests (Task 1.1)
    - Review health check tests (Task 2.1)
    - Review stability period tests (Task 3.1)
    - Review default config tests (Task 4.1)
    - Review display tests (Task 5.1)
    - Review health API tests (Task 6.1)
    - Review config API tests (Task 7.1)
    - Review web UI tests (Task 8.1)
    - Total existing tests: approximately 26-37 tests
  - [x] 9.2 Analyze test coverage gaps for this feature
    - Identify critical end-to-end workflows lacking coverage
    - Focus on integration points between components
    - Prioritize failover cycle tests
    - Do NOT assess entire application test coverage
  - [x] 9.3 Write up to 10 additional strategic tests
    - Full failover cycle: Ethernet -> WiFi -> Ethernet
    - UDP socket migration during failover
    - Simultaneous interface initialization
    - Configuration save/load cycle with all new fields
    - ChirpStack unreachable simulation
    - Focus on integration points
    - Maximum 10 additional tests
  - [x] 9.4 Run feature-specific tests only
    - Run ALL tests from Task Groups 1-8
    - Run additional tests from 9.3
    - Expected total: approximately 36-47 tests
    - Do NOT run entire application test suite
    - Verify all critical workflows pass

**Acceptance Criteria:**
- All feature-specific tests pass (36-47 tests total)
- Full failover cycle verified
- UDP socket migration works correctly
- Configuration persistence verified
- No more than 10 additional tests added

---

## Execution Order

The recommended implementation sequence respects dependencies and groups work by specialist:

```
Phase 1 - Foundation (Parallel Start)
|
+-- Task Group 1: DNS Protocol (ATmega)          [COMPLETED]
+-- Task Group 4: Primary Interface Default      [COMPLETED]
|
Phase 2 - Core Network Logic (After Phase 1)
|
+-- Task Group 2: Health Checking                [COMPLETED]
    |
    +-- Task Group 3: Stability Period           [COMPLETED]
|
Phase 3 - Display and API (After Phase 2)
|
+-- Task Group 5: Display Updates                [COMPLETED]
+-- Task Group 6: Health API                     [COMPLETED]
+-- Task Group 7: Extended Config API            [COMPLETED]
|
Phase 4 - Web UI (After Phase 3)
|
+-- Task Group 8: Web UI Updates                 [COMPLETED]
|
Phase 5 - Integration (After All)
|
+-- Task Group 9: Integration Testing            [COMPLETED]
```

### Specialist Assignment Summary

| Task Group | Primary Specialist | Estimated Duration |
|------------|-------------------|-------------------|
| 1. DNS Protocol | Embedded Systems (ATmega) | 4-6 hours |
| 2. Health Checking | Backend/Embedded (ESP32) | 4-6 hours |
| 3. Stability Period | Backend/Embedded (ESP32) | 2-3 hours |
| 4. Default Change | Backend/Embedded (ESP32) | 1-2 hours |
| 5. Display Updates | UI/Embedded | 3-4 hours |
| 6. Health API | Backend/API | 2-3 hours |
| 7. Extended Config API | Backend/API | 3-4 hours |
| 8. Web UI Updates | Frontend/UI | 4-6 hours |
| 9. Integration Testing | QA/Test | 3-4 hours |

**Total Estimated Time:** 26-38 hours

---

## File Reference Summary

### Files to Modify

| File | Task Groups | Changes |
|------|-------------|---------|
| `src_atmega/include/protocol.h` | 1 | Add CMD_DNS_RESOLVE |
| `src_atmega/src/main.cpp` | 1 | DNS command handler |
| `src/atmega_bridge.h` | 1 | Add dnsResolve() |
| `src/atmega_bridge.cpp` | 1 | Implement dnsResolve() |
| `src/ethernet_adapter.cpp` | 1 | Update hostByName() |
| `src/config.h` | 3, 4 | Add NET_STABILITY_PERIOD_DEFAULT, change NET_PRIMARY_WIFI_DEFAULT |
| `src/network_manager.h` | 2, 3, 5, 6 | Config struct, health methods, callback |
| `src/network_manager.cpp` | 2, 3, 4, 5, 6, 7 | Health check, stability, failover, config |
| `src/udp_forwarder.h` | 2 | Add health query methods |
| `src/udp_forwarder.cpp` | 2 | Implement health query |
| `src/lcd_manager.h` | 5 | Add failover notification method |
| `src/lcd_manager.cpp` | 5 | Interface indicator, notification |
| `src/oled_manager.h` | 5 | Add failover notification method |
| `src/oled_manager.cpp` | 5 | Interface indicator, notification |
| `src/wifi_adapter.h` | 7 | Add WiFiConfig struct |
| `src/wifi_adapter.cpp` | 7 | Static IP support |
| `src/web_server.cpp` | 6, 7 | Health endpoint, extended config |
| `data/web/index.html` | 8 | Dashboard, forms, controls |
| `data/web/style.css` | 8 | Interface indicator styles |
| `data/web/app.js` | 8 | Network health, failover config, override controls |

### Configuration JSON Structure (Target)

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
    "health_check_enabled": true,
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

---

## Success Criteria Summary

From the spec, implementation must achieve:

1. **Failover Speed**: Interface switch completes within 5 seconds of health check failure
2. **Recovery Reliability**: Gateway successfully returns to primary after stability period in 95% of tests
3. **Packet Loss During Failover**: Maximum 2 LoRa packets lost during interface transition
4. **DNS Resolution**: Hostnames resolve via Ethernet without WiFi fallback
5. **Display Accuracy**: Interface indicator updates within 1 second of switch
6. **Configuration Persistence**: All network settings survive power cycle
7. **Web UI Responsiveness**: Network configuration page loads within 2 seconds
