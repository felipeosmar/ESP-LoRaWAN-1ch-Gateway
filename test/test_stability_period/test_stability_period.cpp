/**
 * @file test_stability_period.cpp
 * @brief Tests for return-to-primary stability period logic
 *
 * Task Group 3: Return-to-Primary with Stability Period
 * Tests that verify configurable stability-period-based return-to-primary
 * to prevent rapid interface oscillation.
 */

#include <unity.h>
#include <cstdint>

// Config constants (mirror values from src/config.h)
#define NET_STABILITY_PERIOD_DEFAULT   60000   // 60 seconds
#define NET_FAILOVER_TIMEOUT_DEFAULT   30000   // 30 seconds
#define NET_STATUS_CHECK_INTERVAL      1000    // 1 second

/**
 * Mock structure for NetworkManagerConfig
 * Used to simulate NetworkManager configuration for testing
 */
struct MockNetworkManagerConfig {
    bool failoverEnabled;
    uint32_t failoverTimeout;
    uint32_t stabilityPeriod;
    bool healthCheckEnabled;
};

/**
 * Mock structure for stability timer state
 * Simulates the return-to-primary tracking in NetworkManager
 */
struct MockStabilityState {
    bool failoverActive;
    uint32_t primaryStableStart;
    bool primaryConnected;
    bool secondaryConnected;
};

/**
 * Simulates the return-to-primary logic from NetworkManager::checkFailover()
 * Returns action to take: 0 = no action, 1 = start timer, 2 = return to primary
 * Also sets newPrimaryStableStart for timer tracking
 */
static int checkReturnToPrimary(
    const MockNetworkManagerConfig& config,
    MockStabilityState& state,
    uint32_t currentTime
) {
    if (!config.failoverEnabled) {
        return 0;  // No action
    }

    // Only check return-to-primary when in failover state
    if (!state.failoverActive || !state.primaryConnected) {
        // Reset timer when not in failover or primary not connected
        state.primaryStableStart = 0;
        return 0;
    }

    // Primary is connected while in failover mode
    if (state.primaryStableStart == 0) {
        // Start stability timer
        state.primaryStableStart = currentTime;
        return 1;  // Timer started
    }

    // Check if stability period has elapsed
    if (currentTime - state.primaryStableStart >= config.stabilityPeriod) {
        // Return to primary after stability period
        state.failoverActive = false;
        state.primaryStableStart = 0;
        return 2;  // Return to primary
    }

    return 0;  // Still waiting
}

/**
 * Simulates primary failure during stability period
 * Returns true if timer was reset due to primary failure
 */
static bool simulatePrimaryFailure(MockStabilityState& state) {
    if (state.primaryStableStart != 0) {
        // Primary failed during stability period - reset timer
        state.primaryStableStart = 0;
        return true;  // Timer was reset
    }
    return false;  // No timer was running
}

// ============================================================
// Test Cases
// ============================================================

/**
 * Test 1: Return to primary occurs after stability period
 * Verifies that the gateway waits for the full stability period
 * before switching back to primary interface
 */
void test_return_to_primary_after_stability_period(void) {
    MockNetworkManagerConfig config;
    config.failoverEnabled = true;
    config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;  // 60 seconds

    MockStabilityState state;
    state.failoverActive = true;
    state.primaryStableStart = 0;
    state.primaryConnected = true;
    state.secondaryConnected = true;

    uint32_t baseTime = 100000;  // Start at 100 seconds

    // First check - should start stability timer
    int result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result,
        "Should return 1 (timer started) on first check");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(baseTime, state.primaryStableStart,
        "primaryStableStart should be set to current time");

    // Check at 30 seconds - still within stability period
    result = checkReturnToPrimary(config, state, baseTime + 30000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should return 0 (still waiting) at 30s");
    TEST_ASSERT_TRUE_MESSAGE(state.failoverActive,
        "Failover should still be active at 30s");

    // Check at 59 seconds - still within stability period
    result = checkReturnToPrimary(config, state, baseTime + 59000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should return 0 (still waiting) at 59s");
    TEST_ASSERT_TRUE_MESSAGE(state.failoverActive,
        "Failover should still be active at 59s");

    // Check at 60 seconds - should return to primary
    result = checkReturnToPrimary(config, state, baseTime + 60000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result,
        "Should return 2 (return to primary) at 60s");
    TEST_ASSERT_FALSE_MESSAGE(state.failoverActive,
        "Failover should be inactive after returning to primary");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state.primaryStableStart,
        "primaryStableStart should be reset after return to primary");
}

/**
 * Test 2: Return to primary aborted if primary fails during stability period
 * Verifies that the stability timer is reset when primary goes down
 */
void test_return_to_primary_aborted_on_primary_failure(void) {
    MockNetworkManagerConfig config;
    config.failoverEnabled = true;
    config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;

    MockStabilityState state;
    state.failoverActive = true;
    state.primaryStableStart = 0;
    state.primaryConnected = true;
    state.secondaryConnected = true;

    uint32_t baseTime = 100000;

    // Start stability timer
    int result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result,
        "Should start timer when primary connects during failover");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, state.primaryStableStart,
        "Timer should be running");

    // Wait 30 seconds
    result = checkReturnToPrimary(config, state, baseTime + 30000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Should still be waiting at 30s");

    // Primary fails at 30 seconds
    state.primaryConnected = false;
    result = checkReturnToPrimary(config, state, baseTime + 30000);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state.primaryStableStart,
        "Timer should be reset when primary disconnects");

    // Primary comes back at 35 seconds
    state.primaryConnected = true;
    result = checkReturnToPrimary(config, state, baseTime + 35000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result,
        "Should restart timer when primary reconnects");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(baseTime + 35000, state.primaryStableStart,
        "New timer should start from reconnection time");

    // Should NOT return to primary at original 60s (now only 25s since reconnect)
    result = checkReturnToPrimary(config, state, baseTime + 60000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should not return to primary - only 25s since reconnect");
    TEST_ASSERT_TRUE_MESSAGE(state.failoverActive,
        "Failover should still be active");

    // Should return to primary at 95s (60s after reconnection at 35s)
    result = checkReturnToPrimary(config, state, baseTime + 95000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result,
        "Should return to primary 60s after reconnection");
    TEST_ASSERT_FALSE_MESSAGE(state.failoverActive,
        "Failover should be inactive after return to primary");
}

/**
 * Test 3: Stability period resets on primary failure
 * Verifies that the timer is properly reset multiple times
 */
void test_stability_period_resets_on_primary_failure(void) {
    MockStabilityState state;
    state.failoverActive = true;
    state.primaryStableStart = 50000;  // Timer was running
    state.primaryConnected = true;
    state.secondaryConnected = true;

    // Primary fails
    state.primaryConnected = false;
    bool wasReset = simulatePrimaryFailure(state);

    TEST_ASSERT_TRUE_MESSAGE(wasReset,
        "Timer should have been reset on primary failure");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state.primaryStableStart,
        "primaryStableStart should be 0 after reset");

    // Primary fails again when no timer running
    state.primaryStableStart = 0;
    wasReset = simulatePrimaryFailure(state);

    TEST_ASSERT_FALSE_MESSAGE(wasReset,
        "No timer to reset when primaryStableStart is already 0");

    // Start timer again
    state.primaryConnected = true;
    MockNetworkManagerConfig config;
    config.failoverEnabled = true;
    config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;

    int result = checkReturnToPrimary(config, state, 100000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result,
        "Timer should restart after previous reset");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(100000, state.primaryStableStart,
        "Timer should be set to current time");

    // Reset again
    state.primaryConnected = false;
    wasReset = simulatePrimaryFailure(state);
    TEST_ASSERT_TRUE_MESSAGE(wasReset,
        "Timer should reset again on subsequent failure");
}

/**
 * Test 4: Configurable stability period (default 60s)
 * Verifies that stability period can be configured and defaults to 60s
 */
void test_configurable_stability_period(void) {
    // Test default value
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        60000,
        NET_STABILITY_PERIOD_DEFAULT,
        "Default stability period should be 60000ms (60 seconds)"
    );

    MockNetworkManagerConfig config;
    config.failoverEnabled = true;

    MockStabilityState state;
    state.failoverActive = true;
    state.primaryStableStart = 0;
    state.primaryConnected = true;
    state.secondaryConnected = true;

    uint32_t baseTime = 100000;

    // Test with custom 30-second stability period
    config.stabilityPeriod = 30000;

    int result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "Timer should start");

    // Should not return at 29 seconds
    result = checkReturnToPrimary(config, state, baseTime + 29000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should not return at 29s with 30s stability period");

    // Should return at 30 seconds
    result = checkReturnToPrimary(config, state, baseTime + 30000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result,
        "Should return at 30s with 30s stability period");

    // Reset state and test with longer period (120 seconds)
    state.failoverActive = true;
    state.primaryStableStart = 0;
    state.primaryConnected = true;
    config.stabilityPeriod = 120000;

    result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "Timer should start");

    // Should not return at 60 seconds (would pass with default)
    result = checkReturnToPrimary(config, state, baseTime + 60000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should not return at 60s with 120s stability period");
    TEST_ASSERT_TRUE_MESSAGE(state.failoverActive,
        "Failover should still be active at 60s");

    // Should return at 120 seconds
    result = checkReturnToPrimary(config, state, baseTime + 120000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result,
        "Should return at 120s with 120s stability period");
}

/**
 * Test 5: Stability timer not started when not in failover mode
 * Verifies that timer logic only applies during failover
 */
void test_stability_timer_only_during_failover(void) {
    MockNetworkManagerConfig config;
    config.failoverEnabled = true;
    config.stabilityPeriod = NET_STABILITY_PERIOD_DEFAULT;

    MockStabilityState state;
    state.failoverActive = false;  // Not in failover mode
    state.primaryStableStart = 0;
    state.primaryConnected = true;
    state.secondaryConnected = true;

    uint32_t baseTime = 100000;

    // Should not start timer when not in failover
    int result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "Should not start timer when not in failover mode");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, state.primaryStableStart,
        "primaryStableStart should remain 0 when not in failover");

    // Enable failover mode
    state.failoverActive = true;
    result = checkReturnToPrimary(config, state, baseTime);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result,
        "Should start timer when failover becomes active");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, state.primaryStableStart,
        "Timer should be running");
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

    // Run stability period tests
    RUN_TEST(test_return_to_primary_after_stability_period);
    RUN_TEST(test_return_to_primary_aborted_on_primary_failure);
    RUN_TEST(test_stability_period_resets_on_primary_failure);
    RUN_TEST(test_configurable_stability_period);
    RUN_TEST(test_stability_timer_only_during_failover);

    return UNITY_END();
}
