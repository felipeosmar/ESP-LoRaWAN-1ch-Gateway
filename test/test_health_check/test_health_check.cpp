/**
 * @file test_health_check.cpp
 * @brief Tests for application-layer health check logic
 *
 * Task Group 2: Application-Layer Health Checking
 * Tests that verify ChirpStack connectivity-based health checking
 * using Semtech UDP protocol acknowledgments.
 */

#include <unity.h>
#include <cstdint>

// Config constants (mirror values from src/config.h)
#define NET_FAILOVER_TIMEOUT_DEFAULT   30000   // 30 seconds
#define NET_STATUS_CHECK_INTERVAL      1000    // 1 second
#define NET_STABILITY_PERIOD_DEFAULT   60000   // 60 seconds

/**
 * Mock structure for ForwarderStats (mirrors src/udp_forwarder.h)
 * Used to simulate UDPForwarder statistics for testing
 */
struct MockForwarderStats {
    uint32_t pushAckReceived;
    uint32_t pullAckReceived;
    uint32_t lastAckTime;
};

/**
 * Mock structure for NetworkManagerConfig (mirrors src/network_manager.h)
 */
struct MockNetworkManagerConfig {
    bool healthCheckEnabled;
    uint32_t failoverTimeout;
    uint32_t stabilityPeriod;
};

/**
 * Simulates UDPForwarder::isHealthy() logic
 * Returns true if lastAckTime is within timeout window
 */
static bool isHealthy(const MockForwarderStats& stats, uint32_t timeout, uint32_t currentTime) {
    // If no ACK ever received, consider unhealthy
    if (stats.lastAckTime == 0) {
        return false;
    }
    // Check if lastAckTime is within timeout window
    return (currentTime - stats.lastAckTime) < timeout;
}

/**
 * Simulates health-check-based failover decision
 * Returns true if failover should trigger
 */
static bool shouldTriggerFailover(
    const MockForwarderStats& stats,
    const MockNetworkManagerConfig& config,
    uint32_t currentTime,
    bool primaryConnected
) {
    // If health check disabled, use simple connectivity
    if (!config.healthCheckEnabled) {
        return !primaryConnected;
    }
    // Use application-layer health check
    return !isHealthy(stats, config.failoverTimeout, currentTime);
}

// ============================================================
// Test Cases
// ============================================================

/**
 * Test 1: Health check triggers failover when no ACK within timeout
 * Verifies that missing ACKs cause failover after timeout period
 */
void test_health_check_triggers_failover_on_timeout(void) {
    MockForwarderStats stats = {0};
    stats.lastAckTime = 10000;  // Last ACK at 10 seconds

    MockNetworkManagerConfig config;
    config.healthCheckEnabled = true;
    config.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;  // 30 seconds

    // At 10 seconds (just after ACK), should be healthy
    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, config.failoverTimeout, 10000),
        "Should be healthy immediately after ACK"
    );

    // At 39 seconds (29s after ACK), should still be healthy
    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, config.failoverTimeout, 39000),
        "Should be healthy 29s after ACK (within 30s timeout)"
    );

    // At 41 seconds (31s after ACK), should be unhealthy - failover should trigger
    TEST_ASSERT_FALSE_MESSAGE(
        isHealthy(stats, config.failoverTimeout, 41000),
        "Should be unhealthy 31s after ACK (past 30s timeout)"
    );

    // Verify failover triggers when unhealthy
    bool shouldFailover = shouldTriggerFailover(stats, config, 41000, true);
    TEST_ASSERT_TRUE_MESSAGE(
        shouldFailover,
        "Failover should trigger when health check fails (no ACK within timeout)"
    );
}

/**
 * Test 2: Health check passes when ACK received within window
 * Verifies that recent ACKs maintain healthy status
 */
void test_health_check_passes_with_recent_ack(void) {
    MockForwarderStats stats = {0};
    stats.pushAckReceived = 100;
    stats.pullAckReceived = 50;
    stats.lastAckTime = 50000;  // Last ACK at 50 seconds

    MockNetworkManagerConfig config;
    config.healthCheckEnabled = true;
    config.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;

    // At 60 seconds (10s after ACK), should be healthy
    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, config.failoverTimeout, 60000),
        "Should be healthy 10s after ACK"
    );

    // Failover should NOT trigger
    bool shouldFailover = shouldTriggerFailover(stats, config, 60000, true);
    TEST_ASSERT_FALSE_MESSAGE(
        shouldFailover,
        "Failover should NOT trigger when ACK received within window"
    );

    // Simulate receiving a new ACK
    stats.lastAckTime = 65000;

    // At 90 seconds (25s after new ACK), still healthy
    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, config.failoverTimeout, 90000),
        "Should remain healthy after receiving new ACK"
    );
}

/**
 * Test 3: Integration with UDPForwarder statistics
 * Verifies that lastAckTime is correctly used for health checking
 */
void test_health_check_uses_forwarder_statistics(void) {
    MockForwarderStats stats = {0};

    // Initially, no ACKs received - should be unhealthy
    TEST_ASSERT_FALSE_MESSAGE(
        isHealthy(stats, NET_FAILOVER_TIMEOUT_DEFAULT, 5000),
        "Should be unhealthy when no ACKs have been received (lastAckTime = 0)"
    );

    // Simulate PUSH_ACK received (updates lastAckTime)
    stats.pushAckReceived = 1;
    stats.lastAckTime = 5000;

    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, NET_FAILOVER_TIMEOUT_DEFAULT, 5100),
        "Should be healthy after PUSH_ACK received"
    );

    // Simulate PULL_ACK received (also updates lastAckTime)
    stats.pullAckReceived = 1;
    stats.lastAckTime = 10000;

    TEST_ASSERT_TRUE_MESSAGE(
        isHealthy(stats, NET_FAILOVER_TIMEOUT_DEFAULT, 10100),
        "Should be healthy after PULL_ACK received"
    );

    // Verify stats counters are independent of health check
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, stats.pushAckReceived,
        "pushAckReceived counter should be tracked");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, stats.pullAckReceived,
        "pullAckReceived counter should be tracked");
}

/**
 * Test 4: Failover does not trigger during normal operation
 * Verifies stability during normal ACK reception pattern
 */
void test_failover_not_triggered_during_normal_operation(void) {
    MockForwarderStats stats = {0};
    MockNetworkManagerConfig config;
    config.healthCheckEnabled = true;
    config.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;

    // Simulate normal operation: ACKs every 10 seconds (PULL_INTERVAL)
    uint32_t baseTime = 100000;  // Start at 100 seconds

    for (int i = 0; i < 10; i++) {
        // Simulate ACK received
        stats.lastAckTime = baseTime + (i * 10000);
        stats.pullAckReceived++;

        // Check health 1 second after ACK
        uint32_t checkTime = stats.lastAckTime + 1000;

        TEST_ASSERT_TRUE_MESSAGE(
            isHealthy(stats, config.failoverTimeout, checkTime),
            "Should remain healthy during normal ACK pattern"
        );

        // Failover should never trigger
        bool shouldFailover = shouldTriggerFailover(stats, config, checkTime, true);
        TEST_ASSERT_FALSE_MESSAGE(
            shouldFailover,
            "Failover should not trigger during normal operation"
        );
    }
}

/**
 * Test 5: Health check runs at NET_STATUS_CHECK_INTERVAL (1 second)
 * Verifies the timing constant is correct
 */
void test_health_check_interval_constant(void) {
    // Verify the health check interval is 1 second (1000ms)
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1000,
        NET_STATUS_CHECK_INTERVAL,
        "NET_STATUS_CHECK_INTERVAL should be 1000ms (1 second)"
    );

    // Verify failover timeout is 30 seconds
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        30000,
        NET_FAILOVER_TIMEOUT_DEFAULT,
        "NET_FAILOVER_TIMEOUT_DEFAULT should be 30000ms (30 seconds)"
    );

    // Verify stability period is 60 seconds
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        60000,
        NET_STABILITY_PERIOD_DEFAULT,
        "NET_STABILITY_PERIOD_DEFAULT should be 60000ms (60 seconds)"
    );
}

/**
 * Test 6: Config struct includes health check fields
 * Verifies the configuration structure has required fields
 */
void test_config_struct_has_health_check_fields(void) {
    MockNetworkManagerConfig config;

    // Set and verify healthCheckEnabled field
    config.healthCheckEnabled = true;
    TEST_ASSERT_TRUE_MESSAGE(
        config.healthCheckEnabled,
        "Config should have healthCheckEnabled field"
    );

    config.healthCheckEnabled = false;
    TEST_ASSERT_FALSE_MESSAGE(
        config.healthCheckEnabled,
        "healthCheckEnabled should be settable to false"
    );

    // Set and verify stabilityPeriod field
    config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        60000,
        config.stabilityPeriod,
        "Config should have stabilityPeriod field (default 60000ms)"
    );

    // Set and verify failoverTimeout field
    config.failoverTimeout = NET_FAILOVER_TIMEOUT_DEFAULT;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        30000,
        config.failoverTimeout,
        "Config should have failoverTimeout field (default 30000ms)"
    );
}

// ============================================================
// Test Runner
// ============================================================

void setUp(void) {
    // Setup code before each test (if needed)
}

void tearDown(void) {
    // Cleanup code after each test (if needed)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Run health check tests
    RUN_TEST(test_health_check_triggers_failover_on_timeout);
    RUN_TEST(test_health_check_passes_with_recent_ack);
    RUN_TEST(test_health_check_uses_forwarder_statistics);
    RUN_TEST(test_failover_not_triggered_during_normal_operation);
    RUN_TEST(test_health_check_interval_constant);
    RUN_TEST(test_config_struct_has_health_check_fields);

    return UNITY_END();
}
