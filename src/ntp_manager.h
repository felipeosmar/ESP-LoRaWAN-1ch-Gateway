#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"

// NTP defaults
#define NTP_SERVER1_DEFAULT "pool.ntp.org"
#define NTP_SERVER2_DEFAULT "time.google.com"
#define NTP_TIMEZONE_DEFAULT 0
#define NTP_DAYLIGHT_DEFAULT 0
#define NTP_SYNC_INTERVAL_DEFAULT 3600000  // 1 hour in milliseconds

// NTP Configuration structure
struct NTPConfig {
    bool enabled;
    char server1[64];
    char server2[64];
    long timezoneOffset;    // Timezone offset in seconds
    int daylightOffset;     // Daylight saving offset in seconds
    uint32_t syncInterval;  // Sync interval in milliseconds
};

// NTP Status structure
struct NTPStatus {
    bool synced;
    unsigned long lastSyncTime;
    unsigned long lastSyncAttempt;
    uint32_t syncCount;
    uint32_t failCount;
};

class NTPManager {
public:
    NTPManager();

    // Initialize NTP
    bool begin();

    // Update (check for periodic resync)
    void update();

    // Manual sync
    bool sync();

    // Configuration
    void loadConfig(const JsonDocument& doc);
    bool saveConfig();
    NTPConfig& getConfig() { return config; }
    NTPStatus& getStatus() { return status; }

    // Status
    bool isSynced() const { return status.synced; }
    String getFormattedTime();
    String getIsoTimestamp();
    time_t getEpochTime();

    // Get status as JSON
    String getStatusJson();

private:
    NTPConfig config;
    NTPStatus status;

    void setDefaultConfig();
    void applyConfig();
};

// Global instance
extern NTPManager ntpManager;

#endif // NTP_MANAGER_H
