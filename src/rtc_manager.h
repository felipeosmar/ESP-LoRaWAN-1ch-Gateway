#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <time.h>
#include "config.h"

// DS1307 I2C Address
#define DS1307_ADDRESS 0x68

// DS1307 Registers
#define DS1307_REG_SECONDS   0x00
#define DS1307_REG_MINUTES   0x01
#define DS1307_REG_HOURS     0x02
#define DS1307_REG_DAY       0x03
#define DS1307_REG_DATE      0x04
#define DS1307_REG_MONTH     0x05
#define DS1307_REG_YEAR      0x06
#define DS1307_REG_CONTROL   0x07

// DS1307 Square Wave Frequencies
#define DS1307_SQW_OFF       0x00
#define DS1307_SQW_1HZ       0x10
#define DS1307_SQW_4KHZ      0x11
#define DS1307_SQW_8KHZ      0x12
#define DS1307_SQW_32KHZ     0x13

// RTC Configuration structure
struct RTCConfig {
    bool enabled;               // RTC module enabled
    uint8_t i2cAddress;        // I2C address (0x68 default for DS1307)
    uint8_t sdaPin;            // I2C SDA pin
    uint8_t sclPin;            // I2C SCL pin
    bool syncWithNTP;          // Auto-sync with NTP when available
    uint32_t syncInterval;     // Sync interval in seconds (0 = manual only)
    uint8_t squareWaveMode;    // Square wave output mode (0=off, 1=1Hz, etc)
    int8_t timezoneOffset;     // Timezone offset in hours from UTC
};

// RTC DateTime structure
struct RTCDateTime {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayOfWeek;         // 1=Sunday, 7=Saturday
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

// RTC Status structure
struct RTCStatus {
    bool available;            // RTC module found and responding
    bool oscillatorRunning;    // Clock oscillator is running
    bool timeSynced;           // Time has been synchronized
    uint32_t lastSyncTime;     // Unix timestamp of last sync
    uint32_t readCount;        // Number of successful reads
    uint32_t writeCount;       // Number of successful writes
    uint32_t errorCount;       // Number of failed operations
    RTCDateTime currentTime;   // Current time from RTC
};

class RTCManager {
public:
    RTCManager();

    // Lifecycle
    bool begin();
    void update();
    bool isAvailable() const { return status.available; }

    // Time operations
    bool getDateTime(RTCDateTime& dt);
    bool setDateTime(const RTCDateTime& dt);
    bool setTimeFromEpoch(time_t epoch);
    bool setTimeFromNTP();
    time_t getEpochTime();

    // Formatted time strings
    String getFormattedDate();      // YYYY-MM-DD
    String getFormattedTime();      // HH:MM:SS
    String getFormattedDateTime();  // YYYY-MM-DD HH:MM:SS
    String getIsoTimestamp();       // ISO 8601 format

    // Square wave control
    bool setSquareWave(uint8_t mode);
    uint8_t getSquareWaveMode();

    // Configuration
    RTCConfig& getConfig() { return config; }
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();

    // Status
    RTCStatus& getStatus() { return status; }
    String getStatusJson();

    // Utility
    static const char* getDayName(uint8_t dayOfWeek);
    static const char* getMonthName(uint8_t month);
    static uint8_t calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day);

private:
    RTCConfig config;
    RTCStatus status;
    bool _initialized;
    unsigned long _lastSyncCheck;
    unsigned long _lastNTPSync;

    void setDefaultConfig();
    bool detectDevice();
    bool isOscillatorRunning();
    void startOscillator();

    // BCD conversion helpers
    static uint8_t bcdToDec(uint8_t val) { return ((val / 16) * 10) + (val % 16); }
    static uint8_t decToBcd(uint8_t val) { return ((val / 10) * 16) + (val % 10); }

    // I2C helpers
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    bool writeRegisters(uint8_t startReg, uint8_t* data, uint8_t len);
    bool readRegisters(uint8_t startReg, uint8_t* data, uint8_t len);
};

// Global instance
extern RTCManager rtcManager;

#endif // RTC_MANAGER_H
