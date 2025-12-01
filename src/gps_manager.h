/**
 * @file gps_manager.h
 * @brief GPS Manager for ESP32 LoRaWAN Gateway
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Manages GPS module for location data.
 * Supports both hardware GPS and manual coordinate entry.
 *
 * Compatible GPS modules:
 * - Quectel L80-M39 (NMEA 0183, 9600 baud default)
 * - u-blox NEO-6M/7M/8M
 * - Any NMEA 0183 compatible GPS module
 *
 * Supported NMEA sentences:
 * - $GPGGA / $GNGGA (Position fix)
 * - $GPRMC / $GNRMC (Recommended minimum)
 *
 * Note: L80-M39 I/O voltage is 2.7V-2.9V. If ESP32 uses 3.3V,
 * a level shifter may be required for reliable operation.
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// GPS Configuration structure
struct GPSConfig {
    bool enabled;           // GPS module enabled
    bool useFixedLocation;  // Use manual coordinates instead of GPS
    uint8_t rxPin;          // Serial RX pin (GPS TX)
    uint8_t txPin;          // Serial TX pin (GPS RX)
    uint8_t enablePin;      // Power control pin (HIGH to enable)
    uint8_t resetPin;       // Reset pin (active LOW)
    uint32_t baudRate;      // GPS serial baud rate
    double fixedLatitude;   // Manual latitude
    double fixedLongitude;  // Manual longitude
    int32_t fixedAltitude;  // Manual altitude in meters
    uint32_t updateInterval; // Update interval in milliseconds
};

// GPS Status structure
struct GPSStatus {
    bool hasFix;            // GPS has valid fix
    uint8_t satellites;     // Number of satellites
    double latitude;        // Current latitude
    double longitude;       // Current longitude
    double altitude;        // Current altitude in meters
    double speed;           // Speed in km/h
    double course;          // Course in degrees
    double hdop;            // Horizontal dilution of precision
    uint32_t fixAge;        // Age of last fix in milliseconds
    uint32_t lastUpdate;    // Last update timestamp
    uint32_t validFixes;    // Count of valid fixes
    uint32_t failedFixes;   // Count of failed fixes
    char dateTime[24];      // GPS date/time string
};

class GPSManager {
public:
    GPSManager();

    /**
     * @brief Initialize GPS module
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Update GPS data (call from loop)
     */
    void update();

    /**
     * @brief Check if GPS is available
     * @return true if GPS is initialized and enabled
     */
    bool isAvailable() const;

    /**
     * @brief Check if GPS has valid fix
     * @return true if has fix or using fixed location
     */
    bool hasFix() const;

    /**
     * @brief Get current latitude
     * @return Latitude in degrees
     */
    double getLatitude() const;

    /**
     * @brief Get current longitude
     * @return Longitude in degrees
     */
    double getLongitude() const;

    /**
     * @brief Get current altitude
     * @return Altitude in meters
     */
    int32_t getAltitude() const;

    /**
     * @brief Get configuration reference
     * @return Reference to GPSConfig
     */
    GPSConfig& getConfig() { return _config; }

    /**
     * @brief Get status reference
     * @return Reference to GPSStatus
     */
    GPSStatus& getStatus() { return _status; }

    /**
     * @brief Power on the GPS module
     */
    void powerOn();

    /**
     * @brief Power off the GPS module
     */
    void powerOff();

    /**
     * @brief Reset the GPS module
     */
    void reset();

    /**
     * @brief Check if GPS is powered on
     * @return true if powered on
     */
    bool isPoweredOn() const { return _poweredOn; }

    /**
     * @brief Load configuration from JSON document
     * @param doc JSON document containing configuration
     */
    void loadConfig(const JsonDocument& doc);

    /**
     * @brief Save configuration to LittleFS
     * @return true if successful
     */
    bool saveConfig();

    /**
     * @brief Get status as JSON string
     * @return JSON formatted status
     */
    String getStatusJson();

private:
    GPSConfig _config;
    GPSStatus _status;
    bool _initialized;
    bool _poweredOn;
    HardwareSerial* _serial;
    unsigned long _lastReadTime;

    void setDefaultConfig();
    void processGPSData();
    bool parseNMEA(const char* sentence);
    void parseGPGGA(const char* sentence);
    void parseGPRMC(const char* sentence);
};

// Global instance
extern GPSManager gpsManager;

#endif // GPS_MANAGER_H
