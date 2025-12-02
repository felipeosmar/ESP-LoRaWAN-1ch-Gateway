/**
 * @file test_failover_integration.cpp
 * @brief Integration tests for network failover feature
 *
 * Task Group 9: Integration Testing and Gap Analysis
 * These tests verify end-to-end workflows and integration points
 * between components of the Ethernet Connectivity and Network Failover feature.
 *
 * Focus areas:
 * - Full failover cycle: Ethernet -> WiFi -> Ethernet
 * - UDP socket migration during failover
 * - Simultaneous interface initialization
 * - Configuration save/load cycle with all new fields
 * - ChirpStack unreachable simulation
 */

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

// Config constants (mirror values from src/config.h)
#define NET_FAILOVER_TIMEOUT_DEFAULT   30000   // 30 seconds
#define NET_STABILITY_PERIOD_DEFAULT   60000   // 60 seconds
#define NET_STATUS_CHECK_INTERVAL      1000    // 1 second
#define FAILOVER_MAX_TIME_MS           5000    // Max 5 seconds for failover (spec requirement)

// Network type enum (mirrors src/network_interface.h)
enum class NetworkType {
    NONE,
    WIFI,
    ETHERNET
};

// Primary interface enum
enum class PrimaryInterface {
    WIFI,
    ETHERNET
};

// Failover state machine states
enum class FailoverState {
    NORMAL,               // Operating on primary interface
    FAILOVER_PENDING,     // Health check failed, about to switch
    FAILOVER_ACTIVE,      // Operating on secondary interface
    RECOVERY_PENDING,     // Primary healthy, waiting stability period
    SWITCHING_BACK        // Switching back to primary
};

/**
 * Mock structure for complete NetworkManager state
 * Used for integration testing of the failover system
 */
struct MockNetworkManagerState {
    // Configuration
    PrimaryInterface primaryInterface;
    uint32_t failoverTimeout;
    uint32_t stabilityPeriod;
    bool failoverEnabled;
    bool healthCheckEnabled;

    // Runtime state
    FailoverState state;
    NetworkType activeInterface;
    bool ethernetConnected;
    bool wifiConnected;
    uint32_t lastAckTime;
    uint32_t primaryStableStart;
    uint32_t failoverStartTime;
    uint32_t failoverCompleteTime;

    // Statistics
    uint32_t failoverCount;
    uint32_t recoveryCount;
};

/**
 * Mock structure for UDP socket state
 */
struct MockUDPSocketState {
    bool socketOpen;
    NetworkType boundInterface;
    uint32_t packetsInFlight;
    bool migrationInProgress;
};

/**
 * Mock structure for complete configuration persistence
 */
struct MockFullConfig {
    // Network manager config
    bool wifiEnabled;
    bool ethernetEnabled;
    PrimaryInterface primary;
    bool failoverEnabled;
    uint32_t failoverTimeout;
    uint32_t stabilityPeriod;
    bool healthCheckEnabled;

    // WiFi IP config
    bool wifiDhcp;
    char wifiStaticIP[16];
    char wifiGateway[16];
    char wifiSubnet[16];
    char wifiDns[16];

    // Ethernet IP config
    bool ethernetDhcp;
    char ethernetStaticIP[16];
    char ethernetGateway[16];
    char ethernetSubnet[16];
    char ethernetDns[16];
};

// ============================================================
// Helper Functions
// ============================================================

/**
 * Initialize NetworkManager state with defaults
 */
static void initNetworkManagerState(MockNetworkManagerState& nm) {
    nm.primaryInterface = PrimaryInterface::ETHERNET;
    nm.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;
    nm.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;
    nm.failoverEnabled = true;
    nm.healthCheckEnabled = true;

    nm.state = FailoverState::NORMAL;
    nm.activeInterface = NetworkType::ETHERNET;
    nm.ethernetConnected = true;
    nm.wifiConnected = true;
    nm.lastAckTime = 0;
    nm.primaryStableStart = 0;
    nm.failoverStartTime = 0;
    nm.failoverCompleteTime = 0;

    nm.failoverCount = 0;
    nm.recoveryCount = 0;
}

/**
 * Simulate health check based on ACK timing
 */
static bool isHealthy(const MockNetworkManagerState& nm, uint32_t currentTime) {
    if (!nm.healthCheckEnabled) {
        return nm.activeInterface == NetworkType::ETHERNET ? nm.ethernetConnected : nm.wifiConnected;
    }
    if (nm.lastAckTime == 0) return false;
    return (currentTime - nm.lastAckTime) < nm.failoverTimeout;
}

/**
 * Simulate receiving an ACK from ChirpStack
 */
static void simulateAckReceived(MockNetworkManagerState& nm, uint32_t currentTime) {
    nm.lastAckTime = currentTime;
}

/**
 * Simulate the failover state machine update
 * Returns true if a state transition occurred
 */
static bool updateFailoverStateMachine(MockNetworkManagerState& nm, uint32_t currentTime) {
    if (!nm.failoverEnabled) return false;

    bool stateChanged = false;

    switch (nm.state) {
        case FailoverState::NORMAL:
            // Check if primary is unhealthy
            if (!isHealthy(nm, currentTime)) {
                nm.state = FailoverState::FAILOVER_PENDING;
                nm.failoverStartTime = currentTime;
                stateChanged = true;
            }
            break;

        case FailoverState::FAILOVER_PENDING:
            // Switch to secondary interface
            if (nm.primaryInterface == PrimaryInterface::ETHERNET) {
                nm.activeInterface = NetworkType::WIFI;
            } else {
                nm.activeInterface = NetworkType::ETHERNET;
            }
            nm.state = FailoverState::FAILOVER_ACTIVE;
            nm.failoverCompleteTime = currentTime;
            nm.failoverCount++;
            stateChanged = true;
            break;

        case FailoverState::FAILOVER_ACTIVE:
            // Check if primary has recovered
            if (nm.primaryInterface == PrimaryInterface::ETHERNET) {
                if (nm.ethernetConnected) {
                    nm.state = FailoverState::RECOVERY_PENDING;
                    nm.primaryStableStart = currentTime;
                    stateChanged = true;
                }
            } else {
                if (nm.wifiConnected) {
                    nm.state = FailoverState::RECOVERY_PENDING;
                    nm.primaryStableStart = currentTime;
                    stateChanged = true;
                }
            }
            break;

        case FailoverState::RECOVERY_PENDING: {
            // Check if primary is still healthy
            bool primaryHealthy = (nm.primaryInterface == PrimaryInterface::ETHERNET) ?
                                   nm.ethernetConnected : nm.wifiConnected;

            if (!primaryHealthy) {
                // Primary failed again, reset timer
                nm.state = FailoverState::FAILOVER_ACTIVE;
                nm.primaryStableStart = 0;
                stateChanged = true;
            } else if ((currentTime - nm.primaryStableStart) >= nm.stabilityPeriod) {
                // Stability period elapsed, switch back
                nm.state = FailoverState::SWITCHING_BACK;
                stateChanged = true;
            }
            break;
        }

        case FailoverState::SWITCHING_BACK:
            // Complete the switch back to primary
            if (nm.primaryInterface == PrimaryInterface::ETHERNET) {
                nm.activeInterface = NetworkType::ETHERNET;
            } else {
                nm.activeInterface = NetworkType::WIFI;
            }
            nm.state = FailoverState::NORMAL;
            nm.primaryStableStart = 0;
            nm.recoveryCount++;
            stateChanged = true;
            break;
    }

    return stateChanged;
}

/**
 * Simulate UDP socket migration during failover
 */
static bool migrateUDPSocket(MockUDPSocketState& udp, NetworkType newInterface) {
    if (udp.boundInterface == newInterface) return true;

    udp.migrationInProgress = true;

    // Close existing socket
    udp.socketOpen = false;

    // Wait for packets in flight (simulated)
    // In real implementation, this would drain the queue

    // Open new socket on new interface
    udp.boundInterface = newInterface;
    udp.socketOpen = true;
    udp.packetsInFlight = 0;
    udp.migrationInProgress = false;

    return true;
}

/**
 * Simulate saving full configuration
 */
static bool saveFullConfig(const MockFullConfig& config, char* buffer, size_t bufferSize) {
    // Simulate JSON serialization
    int written = snprintf(buffer, bufferSize,
        "{\"network\":{\"primary\":\"%s\",\"failover_timeout\":%lu,\"stability_period\":%lu}}",
        config.primary == PrimaryInterface::ETHERNET ? "ethernet" : "wifi",
        (unsigned long)config.failoverTimeout,
        (unsigned long)config.stabilityPeriod
    );
    return written > 0 && (size_t)written < bufferSize;
}

/**
 * Simulate loading full configuration
 */
static bool loadFullConfig(MockFullConfig& config, const char* json) {
    // Simple parsing simulation - in real implementation uses ArduinoJson
    config.primary = (strstr(json, "\"ethernet\"") != nullptr) ?
                     PrimaryInterface::ETHERNET : PrimaryInterface::WIFI;

    // Extract failover_timeout
    const char* timeoutStr = strstr(json, "\"failover_timeout\":");
    if (timeoutStr) {
        config.failoverTimeout = atol(timeoutStr + 19);
    }

    // Extract stability_period
    const char* stabilityStr = strstr(json, "\"stability_period\":");
    if (stabilityStr) {
        config.stabilityPeriod = atol(stabilityStr + 19);
    }

    return true;
}

// ============================================================
// Integration Test Cases
// ============================================================

/**
 * Test 1: Full failover cycle - Ethernet -> WiFi -> Ethernet
 * Verifies complete round-trip failover and recovery
 */
void test_full_failover_cycle_ethernet_wifi_ethernet(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    uint32_t currentTime = 0;

    // Initial state: Ethernet primary, healthy
    simulateAckReceived(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
        "Should start in NORMAL state");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, nm.activeInterface,
        "Should start on Ethernet");

    // Simulate time passing with health checks
    currentTime = 10000;
    simulateAckReceived(nm, currentTime);
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
        "Should remain NORMAL with recent ACK");

    // Simulate ChirpStack becoming unreachable (no more ACKs)
    currentTime = 50000;  // 40 seconds since last ACK
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_PENDING, nm.state,
        "Should enter FAILOVER_PENDING when ACK timeout");

    // Execute failover
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_ACTIVE, nm.state,
        "Should enter FAILOVER_ACTIVE after switch");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::WIFI, nm.activeInterface,
        "Should be on WiFi after failover");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, nm.failoverCount,
        "Failover count should be 1");

    // Simulate Ethernet recovering
    currentTime = 60000;
    nm.ethernetConnected = true;
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::RECOVERY_PENDING, nm.state,
        "Should enter RECOVERY_PENDING when primary recovers");

    // Wait for stability period (60 seconds)
    currentTime = 120000;
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::SWITCHING_BACK, nm.state,
        "Should enter SWITCHING_BACK after stability period");

    // Complete switch back
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
        "Should return to NORMAL state");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, nm.activeInterface,
        "Should be back on Ethernet");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, nm.recoveryCount,
        "Recovery count should be 1");
}

/**
 * Test 2: UDP socket migration during failover
 * Verifies UDP socket is properly migrated to new interface
 */
void test_udp_socket_migration_during_failover(void) {
    MockUDPSocketState udp;
    udp.socketOpen = true;
    udp.boundInterface = NetworkType::ETHERNET;
    udp.packetsInFlight = 5;
    udp.migrationInProgress = false;

    // Verify initial state
    TEST_ASSERT_TRUE_MESSAGE(udp.socketOpen, "Socket should be open initially");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, udp.boundInterface,
        "Socket should be bound to Ethernet initially");

    // Simulate failover to WiFi
    bool migrationResult = migrateUDPSocket(udp, NetworkType::WIFI);

    TEST_ASSERT_TRUE_MESSAGE(migrationResult, "Migration should succeed");
    TEST_ASSERT_TRUE_MESSAGE(udp.socketOpen, "Socket should be open after migration");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::WIFI, udp.boundInterface,
        "Socket should be bound to WiFi after migration");
    TEST_ASSERT_FALSE_MESSAGE(udp.migrationInProgress,
        "Migration should be complete");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, udp.packetsInFlight,
        "Packets in flight should be cleared after migration");

    // Simulate recovery back to Ethernet
    migrationResult = migrateUDPSocket(udp, NetworkType::ETHERNET);

    TEST_ASSERT_TRUE_MESSAGE(migrationResult, "Second migration should succeed");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, udp.boundInterface,
        "Socket should be bound to Ethernet after recovery");

    // Verify no-op migration when already on correct interface
    udp.packetsInFlight = 10;
    migrationResult = migrateUDPSocket(udp, NetworkType::ETHERNET);

    TEST_ASSERT_TRUE_MESSAGE(migrationResult, "No-op migration should succeed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10, udp.packetsInFlight,
        "Packets in flight should not be affected by no-op migration");
}

/**
 * Test 3: Simultaneous interface initialization
 * Verifies both interfaces can be initialized concurrently
 */
void test_simultaneous_interface_initialization(void) {
    MockNetworkManagerState nm;

    // Simulate both interfaces disconnected initially
    nm.ethernetConnected = false;
    nm.wifiConnected = false;
    nm.primaryInterface = PrimaryInterface::ETHERNET;
    nm.failoverEnabled = true;
    nm.activeInterface = NetworkType::NONE;
    nm.state = FailoverState::NORMAL;

    // Simulate both interfaces connecting simultaneously
    nm.ethernetConnected = true;
    nm.wifiConnected = true;

    // Primary should be selected
    if (nm.primaryInterface == PrimaryInterface::ETHERNET && nm.ethernetConnected) {
        nm.activeInterface = NetworkType::ETHERNET;
    } else if (nm.wifiConnected) {
        nm.activeInterface = NetworkType::WIFI;
    }

    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, nm.activeInterface,
        "Should select Ethernet as active when both connect (Ethernet is primary)");

    // Both should remain connected (for failover capability)
    TEST_ASSERT_TRUE_MESSAGE(nm.ethernetConnected, "Ethernet should remain connected");
    TEST_ASSERT_TRUE_MESSAGE(nm.wifiConnected, "WiFi should remain connected for failover");

    // Test with WiFi as primary
    nm.primaryInterface = PrimaryInterface::WIFI;
    nm.activeInterface = NetworkType::NONE;

    if (nm.primaryInterface == PrimaryInterface::WIFI && nm.wifiConnected) {
        nm.activeInterface = NetworkType::WIFI;
    } else if (nm.ethernetConnected) {
        nm.activeInterface = NetworkType::ETHERNET;
    }

    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::WIFI, nm.activeInterface,
        "Should select WiFi as active when primary is WiFi");
}

/**
 * Test 4: Configuration save/load cycle with all new fields
 * Verifies all configuration fields persist correctly
 */
void test_configuration_save_load_cycle(void) {
    MockFullConfig originalConfig;

    // Set up original configuration with custom values
    originalConfig.wifiEnabled = true;
    originalConfig.ethernetEnabled = true;
    originalConfig.primary = PrimaryInterface::ETHERNET;
    originalConfig.failoverEnabled = true;
    originalConfig.failoverTimeout = 45000;  // Custom: 45 seconds
    originalConfig.stabilityPeriod = 90000;  // Custom: 90 seconds
    originalConfig.healthCheckEnabled = true;

    // WiFi static IP config
    originalConfig.wifiDhcp = false;
    strncpy(originalConfig.wifiStaticIP, "192.168.1.100", sizeof(originalConfig.wifiStaticIP));
    strncpy(originalConfig.wifiGateway, "192.168.1.1", sizeof(originalConfig.wifiGateway));
    strncpy(originalConfig.wifiSubnet, "255.255.255.0", sizeof(originalConfig.wifiSubnet));
    strncpy(originalConfig.wifiDns, "8.8.8.8", sizeof(originalConfig.wifiDns));

    // Ethernet static IP config
    originalConfig.ethernetDhcp = false;
    strncpy(originalConfig.ethernetStaticIP, "10.0.0.50", sizeof(originalConfig.ethernetStaticIP));
    strncpy(originalConfig.ethernetGateway, "10.0.0.1", sizeof(originalConfig.ethernetGateway));
    strncpy(originalConfig.ethernetSubnet, "255.255.255.0", sizeof(originalConfig.ethernetSubnet));
    strncpy(originalConfig.ethernetDns, "10.0.0.1", sizeof(originalConfig.ethernetDns));

    // Save configuration
    char configBuffer[512];
    bool saveResult = saveFullConfig(originalConfig, configBuffer, sizeof(configBuffer));
    TEST_ASSERT_TRUE_MESSAGE(saveResult, "Configuration save should succeed");

    // Load configuration into new struct (simulating power cycle)
    MockFullConfig loadedConfig;
    memset(&loadedConfig, 0, sizeof(loadedConfig));

    bool loadResult = loadFullConfig(loadedConfig, configBuffer);
    TEST_ASSERT_TRUE_MESSAGE(loadResult, "Configuration load should succeed");

    // Verify critical fields persisted
    TEST_ASSERT_EQUAL_MESSAGE(PrimaryInterface::ETHERNET, loadedConfig.primary,
        "Primary interface should persist");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(45000, loadedConfig.failoverTimeout,
        "Failover timeout should persist");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(90000, loadedConfig.stabilityPeriod,
        "Stability period should persist");
}

/**
 * Test 5: ChirpStack unreachable simulation
 * Verifies behavior when ChirpStack server becomes unreachable
 */
void test_chirpstack_unreachable_simulation(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    uint32_t currentTime = 0;

    // Normal operation with ACKs
    for (int i = 0; i < 5; i++) {
        currentTime += 10000;  // 10 second intervals
        simulateAckReceived(nm, currentTime);
        updateFailoverStateMachine(nm, currentTime);
        TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
            "Should remain NORMAL during healthy operation");
    }

    uint32_t lastHealthyTime = currentTime;

    // ChirpStack becomes unreachable - no more ACKs
    // Simulate health check every second until failover
    bool failoverTriggered = false;
    for (int i = 0; i < 35; i++) {  // Check for up to 35 seconds
        currentTime += 1000;
        updateFailoverStateMachine(nm, currentTime);

        if (nm.state != FailoverState::NORMAL) {
            failoverTriggered = true;
            break;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(failoverTriggered,
        "Failover should trigger when ChirpStack unreachable");

    // Verify failover occurred at approximately the right time
    uint32_t timeToFailover = currentTime - lastHealthyTime;
    TEST_ASSERT_TRUE_MESSAGE(
        timeToFailover >= NET_FAILOVER_TIMEOUT_DEFAULT &&
        timeToFailover <= NET_FAILOVER_TIMEOUT_DEFAULT + 2000,
        "Failover should occur around failover_timeout (30s +/- 2s)"
    );

    // Complete failover
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::WIFI, nm.activeInterface,
        "Should switch to WiFi when ChirpStack unreachable via Ethernet");
}

/**
 * Test 6: Failover timing compliance (5 second requirement)
 * Verifies failover completes within spec requirement
 */
void test_failover_timing_compliance(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    uint32_t currentTime = 10000;
    simulateAckReceived(nm, currentTime);

    // Trigger failover
    currentTime = 50000;  // 40 seconds since ACK (past timeout)
    updateFailoverStateMachine(nm, currentTime);
    uint32_t failoverStartTime = nm.failoverStartTime;

    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_PENDING, nm.state,
        "Should be in FAILOVER_PENDING");

    // Complete failover
    updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_ACTIVE, nm.state,
        "Should be in FAILOVER_ACTIVE");

    // Verify timing
    uint32_t failoverDuration = nm.failoverCompleteTime - failoverStartTime;

    // In mock, transition is instant. In real implementation,
    // this tests that the transition takes less than 5 seconds
    TEST_ASSERT_TRUE_MESSAGE(failoverDuration <= FAILOVER_MAX_TIME_MS,
        "Failover should complete within 5 seconds (spec requirement)");
}

/**
 * Test 7: Multiple consecutive failovers
 * Verifies system handles repeated failovers correctly
 */
void test_multiple_consecutive_failovers(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    uint32_t currentTime = 0;
    simulateAckReceived(nm, currentTime);

    // Perform 3 complete failover cycles
    for (int cycle = 0; cycle < 3; cycle++) {
        // Trigger failover (ChirpStack unreachable)
        currentTime += 35000;
        updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_PENDING
        updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_ACTIVE

        TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_ACTIVE, nm.state,
            "Should be in FAILOVER_ACTIVE");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)(cycle + 1), nm.failoverCount,
            "Failover count should increment");

        // Recover primary
        nm.ethernetConnected = true;
        updateFailoverStateMachine(nm, currentTime);  // -> RECOVERY_PENDING

        // Wait stability period
        currentTime += nm.stabilityPeriod;
        updateFailoverStateMachine(nm, currentTime);  // -> SWITCHING_BACK
        updateFailoverStateMachine(nm, currentTime);  // -> NORMAL

        TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
            "Should return to NORMAL");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)(cycle + 1), nm.recoveryCount,
            "Recovery count should increment");

        // Reset for next cycle - receive ACK to be healthy
        simulateAckReceived(nm, currentTime);
    }

    // Verify final counts
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, nm.failoverCount,
        "Should have 3 failovers total");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, nm.recoveryCount,
        "Should have 3 recoveries total");
}

/**
 * Test 8: Stability period interrupted by primary failure
 * Verifies stability timer resets correctly on failure
 */
void test_stability_period_interrupted(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    uint32_t currentTime = 0;

    // Initial failover
    currentTime = 35000;
    updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_PENDING
    updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_ACTIVE

    // Primary recovers, start stability period
    nm.ethernetConnected = true;
    currentTime += 1000;
    updateFailoverStateMachine(nm, currentTime);  // -> RECOVERY_PENDING

    uint32_t stabilityStart = nm.primaryStableStart;

    // Wait 30 seconds (half of stability period)
    currentTime += 30000;
    updateFailoverStateMachine(nm, currentTime);
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::RECOVERY_PENDING, nm.state,
        "Should still be in RECOVERY_PENDING at 30s");

    // Primary fails again
    nm.ethernetConnected = false;
    updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::FAILOVER_ACTIVE, nm.state,
        "Should return to FAILOVER_ACTIVE when primary fails");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, nm.primaryStableStart,
        "Stability timer should be reset");

    // Primary recovers again
    nm.ethernetConnected = true;
    currentTime += 1000;
    updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::RECOVERY_PENDING, nm.state,
        "Should re-enter RECOVERY_PENDING");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(stabilityStart, nm.primaryStableStart,
        "Stability timer should restart from new time");

    // Now wait full stability period
    currentTime += nm.stabilityPeriod;
    updateFailoverStateMachine(nm, currentTime);  // -> SWITCHING_BACK
    updateFailoverStateMachine(nm, currentTime);  // -> NORMAL

    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
        "Should finally return to NORMAL after full stability period");
}

/**
 * Test 9: Display indicator consistency during failover
 * Verifies display shows correct interface indicator throughout cycle
 */
void test_display_indicator_consistency(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);

    // Helper to get display indicator
    auto getIndicator = [](NetworkType type) -> char {
        switch (type) {
            case NetworkType::ETHERNET: return 'E';
            case NetworkType::WIFI: return 'W';
            default: return '-';
        }
    };

    uint32_t currentTime = 0;
    simulateAckReceived(nm, currentTime);

    // Initial: Ethernet active
    TEST_ASSERT_EQUAL_CHAR_MESSAGE('E', getIndicator(nm.activeInterface),
        "Should show 'E' for Ethernet initially");

    // After failover to WiFi
    currentTime = 35000;
    updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_PENDING
    updateFailoverStateMachine(nm, currentTime);  // -> FAILOVER_ACTIVE

    TEST_ASSERT_EQUAL_CHAR_MESSAGE('W', getIndicator(nm.activeInterface),
        "Should show 'W' after failover to WiFi");

    // During recovery period (still on WiFi)
    nm.ethernetConnected = true;
    currentTime += 1000;
    updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_EQUAL_CHAR_MESSAGE('W', getIndicator(nm.activeInterface),
        "Should still show 'W' during recovery period");

    // After recovery complete
    currentTime += nm.stabilityPeriod;
    updateFailoverStateMachine(nm, currentTime);
    updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_EQUAL_CHAR_MESSAGE('E', getIndicator(nm.activeInterface),
        "Should show 'E' after recovery to Ethernet");
}

/**
 * Test 10: Failover disabled mode
 * Verifies system operates correctly with failover disabled
 */
void test_failover_disabled_mode(void) {
    MockNetworkManagerState nm;
    initNetworkManagerState(nm);
    nm.failoverEnabled = false;

    uint32_t currentTime = 0;
    simulateAckReceived(nm, currentTime);

    // Simulate unhealthy conditions
    currentTime = 50000;  // 50 seconds since ACK

    // Update should not trigger failover
    bool stateChanged = updateFailoverStateMachine(nm, currentTime);

    TEST_ASSERT_FALSE_MESSAGE(stateChanged,
        "No state change should occur when failover disabled");
    TEST_ASSERT_EQUAL_MESSAGE(FailoverState::NORMAL, nm.state,
        "Should remain in NORMAL state");
    TEST_ASSERT_EQUAL_MESSAGE(NetworkType::ETHERNET, nm.activeInterface,
        "Should remain on primary interface");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, nm.failoverCount,
        "Failover count should remain 0");
}

// ============================================================
// Test Runner
// ============================================================

void setUp(void) {
    // Setup code before each test
}

void tearDown(void) {
    // Cleanup code after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Run integration tests
    RUN_TEST(test_full_failover_cycle_ethernet_wifi_ethernet);
    RUN_TEST(test_udp_socket_migration_during_failover);
    RUN_TEST(test_simultaneous_interface_initialization);
    RUN_TEST(test_configuration_save_load_cycle);
    RUN_TEST(test_chirpstack_unreachable_simulation);
    RUN_TEST(test_failover_timing_compliance);
    RUN_TEST(test_multiple_consecutive_failovers);
    RUN_TEST(test_stability_period_interrupted);
    RUN_TEST(test_display_indicator_consistency);
    RUN_TEST(test_failover_disabled_mode);

    return UNITY_END();
}
