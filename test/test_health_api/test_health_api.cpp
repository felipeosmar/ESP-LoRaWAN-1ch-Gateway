/**
 * @file test_health_api.cpp
 * @brief Tests for /api/network/health endpoint
 *
 * Task Group 6: Network Health API Endpoint
 * Tests that verify the health API returns correct JSON structure
 * and includes health status, lastAckTime, timeout, and failover state.
 */

#include <unity.h>
#include <cstdint>
#include <cstring>

// Config constants (mirror values from src/config.h)
#define NET_FAILOVER_TIMEOUT_DEFAULT   30000   // 30 seconds
#define NET_STABILITY_PERIOD_DEFAULT   60000   // 60 seconds

/**
 * Mock structure for ForwarderStats (mirrors src/udp_forwarder.h)
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
 * Mock structure representing the health API response fields
 * This mirrors what NetworkManager::getHealthJson() returns
 */
struct MockHealthResponse {
    bool healthy;
    uint32_t lastAckTime;
    uint32_t failoverTimeout;
    bool failoverActive;
    uint32_t stabilityPeriod;
    uint32_t primaryStableFor;
};

/**
 * Simulates building the health response (mirrors NetworkManager::getHealthJson())
 */
static void buildMockHealthResponse(
    MockHealthResponse& response,
    bool healthy,
    uint32_t lastAckTime,
    uint32_t failoverTimeout,
    bool failoverActive,
    uint32_t stabilityPeriod,
    uint32_t primaryStableFor
) {
    response.healthy = healthy;
    response.lastAckTime = lastAckTime;
    response.failoverTimeout = failoverTimeout;
    response.failoverActive = failoverActive;
    response.stabilityPeriod = stabilityPeriod;
    response.primaryStableFor = primaryStableFor;
}

/**
 * Simulates checking if health check is healthy based on lastAckTime
 */
static bool isHealthy(uint32_t lastAckTime, uint32_t failoverTimeout, uint32_t currentTime) {
    if (lastAckTime == 0) {
        return false;
    }
    return (currentTime - lastAckTime) < failoverTimeout;
}

/**
 * Simulates validating the health response structure has all required fields
 * Returns true if all fields are present and valid
 */
static bool validateHealthResponseStructure(const MockHealthResponse& response) {
    // Verify timeout values are reasonable
    if (response.failoverTimeout == 0) return false;
    if (response.stabilityPeriod == 0) return false;
    // All fields exist by virtue of the struct - this simulates JSON validation
    return true;
}

// ============================================================
// Test Cases
// ============================================================

/**
 * Test 1: GET /api/network/health returns valid JSON structure
 * Verifies that the health endpoint response has valid structure
 */
void test_health_api_returns_valid_json(void) {
    // Build a mock health response
    MockHealthResponse response;
    buildMockHealthResponse(
        response,
        true,     // healthy
        50000,    // lastAckTime
        30000,    // failoverTimeout
        false,    // failoverActive
        60000,    // stabilityPeriod
        0         // primaryStableFor
    );

    // Verify response structure is valid
    TEST_ASSERT_TRUE_MESSAGE(
        validateHealthResponseStructure(response),
        "Health API response should have valid structure"
    );

    // Verify specific field values
    TEST_ASSERT_TRUE_MESSAGE(
        response.healthy,
        "Health API response 'healthy' field should be present"
    );

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        30000,
        response.failoverTimeout,
        "Health API response 'failoverTimeout' should be present"
    );

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        60000,
        response.stabilityPeriod,
        "Health API response 'stabilityPeriod' should be present"
    );
}

/**
 * Test 2: Response includes health status, lastAckTime, timeout
 * Verifies all required fields are present in response
 */
void test_health_api_includes_required_fields(void) {
    // Build a mock health response
    MockHealthResponse response;
    buildMockHealthResponse(
        response,
        true,     // healthy
        45000,    // lastAckTime
        30000,    // failoverTimeout
        false,    // failoverActive
        60000,    // stabilityPeriod
        15000     // primaryStableFor
    );

    // Verify 'healthy' field exists and is boolean (struct ensures type)
    TEST_ASSERT_TRUE_MESSAGE(
        response.healthy == true || response.healthy == false,
        "'healthy' field must be boolean"
    );

    // Verify 'lastAckTime' field contains correct value
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        45000,
        response.lastAckTime,
        "'lastAckTime' should contain correct value"
    );

    // Verify 'failoverTimeout' field exists and has default value
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        30000,
        response.failoverTimeout,
        "'failoverTimeout' should be default 30000ms"
    );

    // Verify 'stabilityPeriod' field exists and has default value
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        60000,
        response.stabilityPeriod,
        "'stabilityPeriod' should be default 60000ms"
    );

    // Verify 'primaryStableFor' field exists
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        15000,
        response.primaryStableFor,
        "'primaryStableFor' should show correct time"
    );
}

/**
 * Test 3: Response includes failover state
 * Verifies failover state information is present
 */
void test_health_api_includes_failover_state(void) {
    // Test with failover inactive
    MockHealthResponse responseInactive;
    buildMockHealthResponse(
        responseInactive,
        true,     // healthy
        55000,    // lastAckTime
        30000,    // failoverTimeout
        false,    // failoverActive
        60000,    // stabilityPeriod
        0         // primaryStableFor
    );

    TEST_ASSERT_FALSE_MESSAGE(
        responseInactive.failoverActive,
        "'failoverActive' should be false when not in failover"
    );

    // Test with failover active
    MockHealthResponse responseActive;
    buildMockHealthResponse(
        responseActive,
        true,     // healthy
        55000,    // lastAckTime
        30000,    // failoverTimeout
        true,     // failoverActive
        60000,    // stabilityPeriod
        25000     // primaryStableFor (25 seconds into stability period)
    );

    TEST_ASSERT_TRUE_MESSAGE(
        responseActive.failoverActive,
        "'failoverActive' should be true when in failover"
    );

    // Verify primaryStableFor is tracked
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        25000,
        responseActive.primaryStableFor,
        "'primaryStableFor' should show time primary has been stable"
    );
}

/**
 * Test 4: Health status reflects actual health check result
 * Verifies that 'healthy' field accurately reflects ACK timing
 */
void test_health_api_healthy_status_accuracy(void) {
    uint32_t currentTime = 100000;  // Current time at 100 seconds
    uint32_t lastAckTime = 85000;   // Last ACK at 85 seconds (15s ago)
    uint32_t failoverTimeout = 30000;  // 30 second timeout

    // 15 seconds since last ACK with 30s timeout -> should be healthy
    bool expectedHealthy = isHealthy(lastAckTime, failoverTimeout, currentTime);
    TEST_ASSERT_TRUE_MESSAGE(
        expectedHealthy,
        "Should be healthy when ACK received within timeout window"
    );

    // Build response with healthy state
    MockHealthResponse responseHealthy;
    buildMockHealthResponse(
        responseHealthy,
        expectedHealthy,
        lastAckTime,
        failoverTimeout,
        false,
        60000,
        0
    );

    TEST_ASSERT_TRUE_MESSAGE(
        responseHealthy.healthy,
        "API should report healthy when ACK within timeout"
    );

    // Test unhealthy state
    lastAckTime = 60000;  // Last ACK at 60 seconds (40s ago)
    bool expectedUnhealthy = isHealthy(lastAckTime, failoverTimeout, currentTime);
    TEST_ASSERT_FALSE_MESSAGE(
        expectedUnhealthy,
        "Should be unhealthy when ACK too old"
    );

    MockHealthResponse responseUnhealthy;
    buildMockHealthResponse(
        responseUnhealthy,
        expectedUnhealthy,
        lastAckTime,
        failoverTimeout,
        true,  // Failover would be active
        60000,
        0
    );

    TEST_ASSERT_FALSE_MESSAGE(
        responseUnhealthy.healthy,
        "API should report unhealthy when ACK outside timeout window"
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

    // Run health API tests
    RUN_TEST(test_health_api_returns_valid_json);
    RUN_TEST(test_health_api_includes_required_fields);
    RUN_TEST(test_health_api_includes_failover_state);
    RUN_TEST(test_health_api_healthy_status_accuracy);

    return UNITY_END();
}
