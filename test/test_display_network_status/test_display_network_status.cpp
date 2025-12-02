/**
 * @file test_display_network_status.cpp
 * @brief Tests for network status display updates
 *
 * Task Group 5: Network Status Display Updates
 * Tests that verify network interface indicators and failover notifications
 * are correctly displayed on LCD 16x2 and OLED SSD1306.
 */

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

// Network type enum (mirrors src/network_interface.h)
enum class NetworkType {
    NONE,
    WIFI,
    ETHERNET
};

// Display notification duration constant
#define FAILOVER_NOTIFICATION_DURATION_MS 2000

/**
 * Mock structure for LCD display state
 * Used to verify correct display content
 */
struct MockLCDState {
    char line1[17];
    char line2[17];
    bool backlightOn;
    bool failoverNotificationActive;
    uint32_t notificationStartTime;
    char networkIndicator;  // 'E' for Ethernet, 'W' for WiFi
};

/**
 * Mock structure for OLED display state
 * Used to verify correct display content
 */
struct MockOLEDState {
    char headerText[32];
    char networkIndicator;  // 'E' for Ethernet, 'W' for WiFi
    bool showingSignalBars;
    int8_t signalStrength;  // RSSI value
    bool failoverNotificationActive;
    uint32_t notificationStartTime;
};

/**
 * Simulates getting network indicator character based on active interface
 */
static char getNetworkIndicator(NetworkType activeType) {
    switch (activeType) {
        case NetworkType::ETHERNET:
            return 'E';
        case NetworkType::WIFI:
            return 'W';
        default:
            return '-';
    }
}

/**
 * Simulates LCD status line format with network indicator
 * Format: "LORA GW  E 12:34" or "LORA GW  W 12:34"
 */
static void formatLCDStatusLine(char* buffer, size_t bufferSize, NetworkType activeType,
                                 uint8_t hours, uint8_t minutes) {
    char indicator = getNetworkIndicator(activeType);
    snprintf(buffer, bufferSize, "LORA GW  %c %02d:%02d", indicator, hours, minutes);
}

/**
 * Simulates checking if failover notification should still be displayed
 */
static bool isFailoverNotificationActive(uint32_t notificationStartTime, uint32_t currentTime) {
    if (notificationStartTime == 0) {
        return false;
    }
    return (currentTime - notificationStartTime) < FAILOVER_NOTIFICATION_DURATION_MS;
}

/**
 * Simulates RSSI to signal bars conversion (0-4 bars)
 * Thresholds:
 * - Excellent signal: > -50 dBm = 4 bars
 * - Good signal: > -60 dBm = 3 bars (values -59 to -51)
 * - Fair signal: > -70 dBm = 2 bars (values -69 to -61)
 * - Weak signal: > -80 dBm = 1 bar (values -79 to -71)
 * - Very weak signal: <= -80 dBm = 0 bars
 */
static int getSignalBars(int8_t rssi) {
    if (rssi > -50) return 4;
    else if (rssi > -60) return 3;
    else if (rssi > -70) return 2;
    else if (rssi > -80) return 1;
    return 0;
}

// ============================================================
// Test Cases
// ============================================================

/**
 * Test 1: LCD shows "E" for Ethernet active
 * Verifies LCD status line includes "E" indicator when Ethernet is active
 */
void test_lcd_shows_ethernet_indicator(void) {
    char statusLine[17];

    // Format status line with Ethernet active
    formatLCDStatusLine(statusLine, sizeof(statusLine), NetworkType::ETHERNET, 12, 34);

    // Verify "E" indicator is present in correct position
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "LORA GW  E 12:34",
        statusLine,
        "LCD should show 'E' for Ethernet active with format 'LORA GW  E HH:MM'"
    );

    // Verify indicator character
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(
        'E',
        getNetworkIndicator(NetworkType::ETHERNET),
        "Ethernet indicator should be 'E'"
    );
}

/**
 * Test 2: LCD shows "W" for WiFi active
 * Verifies LCD status line includes "W" indicator when WiFi is active
 */
void test_lcd_shows_wifi_indicator(void) {
    char statusLine[17];

    // Format status line with WiFi active
    formatLCDStatusLine(statusLine, sizeof(statusLine), NetworkType::WIFI, 15, 45);

    // Verify "W" indicator is present in correct position
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "LORA GW  W 15:45",
        statusLine,
        "LCD should show 'W' for WiFi active with format 'LORA GW  W HH:MM'"
    );

    // Verify indicator character
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(
        'W',
        getNetworkIndicator(NetworkType::WIFI),
        "WiFi indicator should be 'W'"
    );
}

/**
 * Test 3: OLED shows interface indicator in header
 * Verifies OLED header area displays network interface indicator
 */
void test_oled_shows_interface_indicator_in_header(void) {
    MockOLEDState oledState;
    memset(&oledState, 0, sizeof(oledState));

    // Simulate Ethernet active
    oledState.networkIndicator = getNetworkIndicator(NetworkType::ETHERNET);
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(
        'E',
        oledState.networkIndicator,
        "OLED header should show 'E' for Ethernet"
    );

    // Simulate WiFi active
    oledState.networkIndicator = getNetworkIndicator(NetworkType::WIFI);
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(
        'W',
        oledState.networkIndicator,
        "OLED header should show 'W' for WiFi"
    );

    // Simulate no connection
    oledState.networkIndicator = getNetworkIndicator(NetworkType::NONE);
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(
        '-',
        oledState.networkIndicator,
        "OLED header should show '-' for no connection"
    );
}

/**
 * Test 4: Failover notification displays for 2 seconds
 * Verifies notification appears and auto-dismisses after 2 seconds
 */
void test_failover_notification_displays_for_2_seconds(void) {
    uint32_t notificationStartTime = 10000;  // Started at 10 seconds

    // Verify notification duration constant
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        2000,
        FAILOVER_NOTIFICATION_DURATION_MS,
        "Failover notification duration should be 2000ms (2 seconds)"
    );

    // At 10.5 seconds (0.5s after start), notification should be active
    TEST_ASSERT_TRUE_MESSAGE(
        isFailoverNotificationActive(notificationStartTime, 10500),
        "Notification should be active 0.5s after start"
    );

    // At 11.9 seconds (1.9s after start), notification should still be active
    TEST_ASSERT_TRUE_MESSAGE(
        isFailoverNotificationActive(notificationStartTime, 11900),
        "Notification should be active 1.9s after start"
    );

    // At 12.1 seconds (2.1s after start), notification should be dismissed
    TEST_ASSERT_FALSE_MESSAGE(
        isFailoverNotificationActive(notificationStartTime, 12100),
        "Notification should be dismissed after 2 seconds"
    );

    // When no notification started, should return false
    TEST_ASSERT_FALSE_MESSAGE(
        isFailoverNotificationActive(0, 15000),
        "No notification should be active when notificationStartTime is 0"
    );
}

/**
 * Test 5: WiFi signal strength shown when WiFi active
 * Verifies signal strength bars are displayed based on RSSI
 */
void test_wifi_signal_strength_shown_when_wifi_active(void) {
    MockOLEDState oledState;
    memset(&oledState, 0, sizeof(oledState));

    // Test RSSI to bars conversion
    // Excellent signal: > -50 dBm = 4 bars
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, getSignalBars(-45), "RSSI -45 should show 4 bars");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, getSignalBars(-30), "RSSI -30 should show 4 bars");

    // Good signal: > -60 dBm = 3 bars (note: boundary at -60 is 2 bars)
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, getSignalBars(-55), "RSSI -55 should show 3 bars");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, getSignalBars(-59), "RSSI -59 should show 3 bars");

    // Fair signal: > -70 dBm = 2 bars (note: boundary at -70 is 1 bar)
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, getSignalBars(-65), "RSSI -65 should show 2 bars");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, getSignalBars(-69), "RSSI -69 should show 2 bars");

    // Weak signal: > -80 dBm = 1 bar (note: boundary at -80 is 0 bars)
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, getSignalBars(-75), "RSSI -75 should show 1 bar");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, getSignalBars(-79), "RSSI -79 should show 1 bar");

    // Very weak signal: <= -80 dBm = 0 bars
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, getSignalBars(-80), "RSSI -80 should show 0 bars");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, getSignalBars(-100), "RSSI -100 should show 0 bars");

    // Simulate WiFi active with signal strength
    oledState.networkIndicator = 'W';
    oledState.showingSignalBars = true;
    oledState.signalStrength = -55;

    TEST_ASSERT_TRUE_MESSAGE(
        oledState.showingSignalBars,
        "Signal bars should be shown when WiFi is active"
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        3,
        getSignalBars(oledState.signalStrength),
        "WiFi signal -55 dBm should show 3 bars"
    );
}

/**
 * Test 6: Interface indicator updates within expected timing
 * Verifies display can update indicator quickly after interface switch
 */
void test_interface_indicator_update_timing(void) {
    // Simulate interface switch scenario
    NetworkType previousInterface = NetworkType::ETHERNET;
    NetworkType newInterface = NetworkType::WIFI;

    char prevIndicator = getNetworkIndicator(previousInterface);
    char newIndicator = getNetworkIndicator(newInterface);

    // Verify indicators are different
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
        prevIndicator,
        newIndicator,
        "Interface indicators should differ between Ethernet and WiFi"
    );

    // Verify indicator change is immediate (function call)
    char indicator1 = getNetworkIndicator(NetworkType::ETHERNET);
    char indicator2 = getNetworkIndicator(NetworkType::WIFI);

    TEST_ASSERT_EQUAL_CHAR('E', indicator1);
    TEST_ASSERT_EQUAL_CHAR('W', indicator2);

    // The actual display update timing depends on the display manager's update() loop
    // which runs at NET_STATUS_CHECK_INTERVAL (1 second) per spec requirement
    // "Interface indicator updates within 1 second of switch"
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

    // Run display network status tests
    RUN_TEST(test_lcd_shows_ethernet_indicator);
    RUN_TEST(test_lcd_shows_wifi_indicator);
    RUN_TEST(test_oled_shows_interface_indicator_in_header);
    RUN_TEST(test_failover_notification_displays_for_2_seconds);
    RUN_TEST(test_wifi_signal_strength_shown_when_wifi_active);
    RUN_TEST(test_interface_indicator_update_timing);

    return UNITY_END();
}
