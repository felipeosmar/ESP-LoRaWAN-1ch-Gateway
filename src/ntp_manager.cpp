#include "ntp_manager.h"
#include <LittleFS.h>
#include <WiFi.h>

// Global instance
NTPManager ntpManager;

NTPManager::NTPManager() {
    memset(&status, 0, sizeof(status));
    setDefaultConfig();
}

void NTPManager::setDefaultConfig() {
    config.enabled = true;
    strlcpy(config.server1, NTP_SERVER1_DEFAULT, sizeof(config.server1));
    strlcpy(config.server2, NTP_SERVER2_DEFAULT, sizeof(config.server2));
    config.timezoneOffset = NTP_TIMEZONE_DEFAULT;
    config.daylightOffset = NTP_DAYLIGHT_DEFAULT;
    config.syncInterval = NTP_SYNC_INTERVAL_DEFAULT;
}

bool NTPManager::begin() {
    Serial.println("[NTP] Initializing NTP manager...");

    if (!config.enabled) {
        Serial.println("[NTP] NTP is disabled");
        return true;
    }

    Serial.printf("[NTP] Servers: %s, %s\n", config.server1, config.server2);
    Serial.printf("[NTP] Timezone offset: %ld seconds\n", config.timezoneOffset);

    applyConfig();

    // Try initial sync
    if (sync()) {
        Serial.println("[NTP] Initial sync successful");
    } else {
        Serial.println("[NTP] Initial sync failed (will retry later)");
    }

    return true;
}

void NTPManager::applyConfig() {
    configTime(config.timezoneOffset, config.daylightOffset,
               config.server1, config.server2);
}

bool NTPManager::sync() {
    if (!config.enabled) {
        return false;
    }

    status.lastSyncAttempt = millis();

    Serial.println("[NTP] Syncing time...");

    // Wait for time to be set (max 10 seconds)
    time_t now = 0;
    int attempts = 0;
    const int maxAttempts = 20;

    while (now < 1000000000 && attempts < maxAttempts) {
        delay(500);
        time(&now);
        attempts++;
    }

    if (now > 1000000000) {
        status.synced = true;
        status.lastSyncTime = millis();
        status.syncCount++;

        struct tm timeinfo;
        gmtime_r(&now, &timeinfo);
        Serial.printf("[NTP] Time synced: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        return true;
    } else {
        status.failCount++;
        Serial.println("[NTP] Sync failed (timeout)");
        return false;
    }
}

void NTPManager::update() {
    if (!config.enabled) return;

    unsigned long now = millis();

    // Periodic resync
    if (now - status.lastSyncTime >= config.syncInterval) {
        // Re-apply config in case connection was lost
        applyConfig();
        sync();
    }
}

void NTPManager::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("ntp")) {
        Serial.println("[NTP] No NTP config in JSON, using defaults");
        return;
    }

    JsonObjectConst ntp = doc["ntp"];

    config.enabled = ntp["enabled"] | true;
    strlcpy(config.server1, ntp["server1"] | NTP_SERVER1_DEFAULT, sizeof(config.server1));
    strlcpy(config.server2, ntp["server2"] | NTP_SERVER2_DEFAULT, sizeof(config.server2));
    config.timezoneOffset = ntp["timezone_offset"] | NTP_TIMEZONE_DEFAULT;
    config.daylightOffset = ntp["daylight_offset"] | NTP_DAYLIGHT_DEFAULT;
    config.syncInterval = ntp["sync_interval"] | NTP_SYNC_INTERVAL_DEFAULT;

    Serial.printf("[NTP] Config loaded: %s, TZ offset: %ld\n",
                  config.server1, config.timezoneOffset);
}

bool NTPManager::saveConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[NTP] Cannot open config for reading");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("[NTP] Failed to parse config");
        return false;
    }

    // Update NTP section
    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["enabled"] = config.enabled;
    ntp["server1"] = config.server1;
    ntp["server2"] = config.server2;
    ntp["timezone_offset"] = config.timezoneOffset;
    ntp["daylight_offset"] = config.daylightOffset;
    ntp["sync_interval"] = config.syncInterval;

    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[NTP] Cannot open config for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println("[NTP] Config saved");
    return true;
}

String NTPManager::getFormattedTime() {
    if (!status.synced) {
        return "Not synced";
    }

    time_t now;
    struct tm timeinfo;
    char buf[32];

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

String NTPManager::getIsoTimestamp() {
    time_t now;
    struct tm timeinfo;
    char buf[32];

    time(&now);
    gmtime_r(&now, &timeinfo);

    // Format expected by ChirpStack Gateway Bridge (Semtech format)
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S GMT", &timeinfo);
    return String(buf);
}

time_t NTPManager::getEpochTime() {
    time_t now;
    time(&now);
    return now;
}

String NTPManager::getStatusJson() {
    DynamicJsonDocument doc(512);

    doc["enabled"] = config.enabled;
    doc["synced"] = status.synced;

    JsonObject cfg = doc.createNestedObject("config");
    cfg["server1"] = config.server1;
    cfg["server2"] = config.server2;
    cfg["timezone_offset"] = config.timezoneOffset;
    cfg["daylight_offset"] = config.daylightOffset;
    cfg["sync_interval"] = config.syncInterval;

    JsonObject st = doc.createNestedObject("status");
    st["sync_count"] = status.syncCount;
    st["fail_count"] = status.failCount;

    if (status.lastSyncTime > 0) {
        unsigned long ago = (millis() - status.lastSyncTime) / 1000;
        st["last_sync_ago"] = ago;
    }

    if (status.synced) {
        st["current_time"] = getFormattedTime();
        st["epoch"] = (unsigned long)getEpochTime();
    }

    String output;
    serializeJson(doc, output);
    return output;
}
