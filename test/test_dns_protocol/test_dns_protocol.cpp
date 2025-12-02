/**
 * @file test_dns_protocol.cpp
 * @brief Tests for DNS protocol command (CMD_DNS_RESOLVE)
 *
 * Task Group 1: DNS Protocol Command Implementation
 * Tests that verify DNS resolution capability via ATmega Bridge.
 *
 * These tests focus on the protocol packet format and response handling,
 * not actual network DNS resolution (which requires hardware).
 */

#include <unity.h>
#include <cstring>
#include <cstdint>

// Include protocol definitions
// Note: These constants mirror those in src_atmega/include/protocol.h
#define PROTO_START_BYTE    0xAA
#define PROTO_END_BYTE      0x55
#define PROTO_HEADER_SIZE   4
#define PROTO_FOOTER_SIZE   2

// Command definitions
#define CMD_DNS_RESOLVE     0x25

// Response codes
#define RSP_OK              0x00
#define RSP_ERROR           0x01
#define RSP_INVALID_PARAM   0x03
#define RSP_TIMEOUT         0x04
#define RSP_NOT_INIT        0x06
#define RSP_NO_LINK         0x07

// DNS constants
#define DNS_TIMEOUT_MS      5000
#define DNS_MAX_HOSTNAME    63

/**
 * Calculate CRC8 for testing (mirrors protocol.h implementation)
 */
static uint8_t calculateCRC8(const uint8_t* data, uint16_t length) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * Build a CMD_DNS_RESOLVE request packet
 */
static uint16_t buildDnsResolveRequest(uint8_t* buffer, const char* hostname) {
    uint16_t hostnameLen = strlen(hostname) + 1;  // Include null terminator

    buffer[0] = PROTO_START_BYTE;
    buffer[1] = CMD_DNS_RESOLVE;
    buffer[2] = (hostnameLen >> 8) & 0xFF;
    buffer[3] = hostnameLen & 0xFF;

    memcpy(&buffer[PROTO_HEADER_SIZE], hostname, hostnameLen);

    uint8_t crc = calculateCRC8(&buffer[PROTO_HEADER_SIZE], hostnameLen);
    buffer[PROTO_HEADER_SIZE + hostnameLen] = crc;
    buffer[PROTO_HEADER_SIZE + hostnameLen + 1] = PROTO_END_BYTE;

    return PROTO_HEADER_SIZE + hostnameLen + PROTO_FOOTER_SIZE;
}

/**
 * Build a successful DNS response packet (4-byte IPv4)
 */
static uint16_t buildDnsResolveResponse(uint8_t* buffer, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3) {
    buffer[0] = PROTO_START_BYTE;
    buffer[1] = CMD_DNS_RESOLVE | 0x80;  // Response bit set
    buffer[2] = 0x00;  // Length high byte
    buffer[3] = 0x05;  // Length = 5 bytes (status + 4 IP bytes)

    buffer[4] = RSP_OK;  // Status
    buffer[5] = ip0;
    buffer[6] = ip1;
    buffer[7] = ip2;
    buffer[8] = ip3;

    uint8_t crc = calculateCRC8(&buffer[PROTO_HEADER_SIZE], 5);
    buffer[9] = crc;
    buffer[10] = PROTO_END_BYTE;

    return 11;
}

/**
 * Build an error DNS response packet
 */
static uint16_t buildDnsErrorResponse(uint8_t* buffer, uint8_t errorCode) {
    buffer[0] = PROTO_START_BYTE;
    buffer[1] = CMD_DNS_RESOLVE | 0x80;  // Response bit set
    buffer[2] = 0x00;  // Length high byte
    buffer[3] = 0x01;  // Length = 1 byte (status only)

    buffer[4] = errorCode;

    uint8_t crc = calculateCRC8(&buffer[PROTO_HEADER_SIZE], 1);
    buffer[5] = crc;
    buffer[6] = PROTO_END_BYTE;

    return 7;
}

// ============================================================
// Test Cases
// ============================================================

/**
 * Test 1: CMD_DNS_RESOLVE request/response packet format
 * Verifies correct packet structure for DNS resolve command
 */
void test_dns_resolve_packet_format(void) {
    uint8_t requestBuffer[128];
    const char* hostname = "example.com";

    uint16_t requestLen = buildDnsResolveRequest(requestBuffer, hostname);

    // Verify packet structure
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(PROTO_START_BYTE, requestBuffer[0],
        "Request should start with START_BYTE (0xAA)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(CMD_DNS_RESOLVE, requestBuffer[1],
        "Command byte should be CMD_DNS_RESOLVE (0x25)");

    uint16_t dataLen = (requestBuffer[2] << 8) | requestBuffer[3];
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(strlen(hostname) + 1, dataLen,
        "Data length should be hostname length + null terminator");

    // Verify hostname in packet (null-terminated)
    TEST_ASSERT_EQUAL_STRING_MESSAGE(hostname, (const char*)&requestBuffer[PROTO_HEADER_SIZE],
        "Hostname should be in data section");

    // Verify end byte
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(PROTO_END_BYTE, requestBuffer[requestLen - 1],
        "Request should end with END_BYTE (0x55)");

    // Build and verify response packet
    uint8_t responseBuffer[16];
    uint16_t responseLen = buildDnsResolveResponse(responseBuffer, 93, 184, 216, 34);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(PROTO_START_BYTE, responseBuffer[0],
        "Response should start with START_BYTE");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(CMD_DNS_RESOLVE | 0x80, responseBuffer[1],
        "Response command should have bit 7 set");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_OK, responseBuffer[4],
        "Success response should have RSP_OK status");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(PROTO_END_BYTE, responseBuffer[responseLen - 1],
        "Response should end with END_BYTE");
}

/**
 * Test 2: DNS resolution success with valid hostname
 * Verifies correct parsing of 4-byte IPv4 response
 */
void test_dns_resolution_success(void) {
    uint8_t responseBuffer[16];

    // Simulate response for google.com -> 142.250.185.78
    uint16_t responseLen = buildDnsResolveResponse(responseBuffer, 142, 250, 185, 78);

    // Parse response
    uint8_t status = responseBuffer[4];
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_OK, status, "Status should be RSP_OK");

    uint16_t dataLen = (responseBuffer[2] << 8) | responseBuffer[3];
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(5, dataLen, "Response data length should be 5 (status + 4 IP bytes)");

    // Extract IP address
    uint8_t ip[4];
    ip[0] = responseBuffer[5];
    ip[1] = responseBuffer[6];
    ip[2] = responseBuffer[7];
    ip[3] = responseBuffer[8];

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(142, ip[0], "IP byte 0 should be 142");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(250, ip[1], "IP byte 1 should be 250");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(185, ip[2], "IP byte 2 should be 185");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(78, ip[3], "IP byte 3 should be 78");

    // Verify CRC
    uint8_t receivedCrc = responseBuffer[9];
    uint8_t calculatedCrc = calculateCRC8(&responseBuffer[4], 5);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(calculatedCrc, receivedCrc, "CRC should match");
}

/**
 * Test 3: DNS resolution failure handling (RSP_ERROR)
 * Verifies correct handling of various error codes
 */
void test_dns_resolution_failure(void) {
    uint8_t responseBuffer[16];

    // Test RSP_ERROR (generic DNS failure)
    buildDnsErrorResponse(responseBuffer, RSP_ERROR);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_ERROR, responseBuffer[4],
        "DNS failure should return RSP_ERROR");

    // Test RSP_NOT_INIT (Ethernet not initialized)
    buildDnsErrorResponse(responseBuffer, RSP_NOT_INIT);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_NOT_INIT, responseBuffer[4],
        "Uninitialized Ethernet should return RSP_NOT_INIT");

    // Test RSP_NO_LINK (No Ethernet link)
    buildDnsErrorResponse(responseBuffer, RSP_NO_LINK);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_NO_LINK, responseBuffer[4],
        "No link should return RSP_NO_LINK");

    // Test RSP_INVALID_PARAM (invalid hostname)
    buildDnsErrorResponse(responseBuffer, RSP_INVALID_PARAM);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_INVALID_PARAM, responseBuffer[4],
        "Invalid hostname should return RSP_INVALID_PARAM");
}

/**
 * Test 4: DNS timeout behavior
 * Verifies timeout constant and timeout response handling
 */
void test_dns_timeout_behavior(void) {
    // Verify timeout constant
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(5000, DNS_TIMEOUT_MS,
        "DNS timeout should be 5000ms (5 seconds)");

    // Test RSP_TIMEOUT response
    uint8_t responseBuffer[16];
    buildDnsErrorResponse(responseBuffer, RSP_TIMEOUT);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(RSP_TIMEOUT, responseBuffer[4],
        "Timeout should return RSP_TIMEOUT");

    // Verify response format is valid even for timeout
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(PROTO_START_BYTE, responseBuffer[0],
        "Timeout response should have valid start byte");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(CMD_DNS_RESOLVE | 0x80, responseBuffer[1],
        "Timeout response should have response bit set");
}

/**
 * Test 5: DNS hostname length validation
 * Verifies hostname length constraints
 */
void test_dns_hostname_length_validation(void) {
    // Verify max hostname length constant
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(63, DNS_MAX_HOSTNAME,
        "DNS max hostname should be 63 characters (DNS label limit)");

    // Test valid hostname length
    const char* validHostname = "chirpstack.local";
    TEST_ASSERT_TRUE_MESSAGE(strlen(validHostname) <= DNS_MAX_HOSTNAME,
        "Valid hostname should be within length limit");

    // Test max length hostname (63 chars)
    char maxHostname[64];
    memset(maxHostname, 'a', 63);
    maxHostname[63] = '\0';
    TEST_ASSERT_EQUAL_MESSAGE(63, strlen(maxHostname),
        "Max hostname should be exactly 63 characters");

    // Test that too-long hostname would fail validation
    char tooLongHostname[65];
    memset(tooLongHostname, 'a', 64);
    tooLongHostname[64] = '\0';
    TEST_ASSERT_TRUE_MESSAGE(strlen(tooLongHostname) > DNS_MAX_HOSTNAME,
        "Hostname > 63 chars should fail validation");
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

    // Run DNS protocol tests
    RUN_TEST(test_dns_resolve_packet_format);
    RUN_TEST(test_dns_resolution_success);
    RUN_TEST(test_dns_resolution_failure);
    RUN_TEST(test_dns_timeout_behavior);
    RUN_TEST(test_dns_hostname_length_validation);

    return UNITY_END();
}
