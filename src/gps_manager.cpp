/**
 * @file gps_manager.cpp
 * @brief GPS Manager implementation for ESP32 LoRaWAN Gateway
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "gps_manager.h"
#include <LittleFS.h>

// Global instance
GPSManager gpsManager;

GPSManager::GPSManager() : _initialized(false), _poweredOn(false), _serial(nullptr), _lastReadTime(0) {
    setDefaultConfig();
    memset(&_status, 0, sizeof(_status));
}

void GPSManager::setDefaultConfig() {
    _config.enabled = GPS_ENABLED_DEFAULT;
    _config.useFixedLocation = GPS_USE_FIXED_DEFAULT;
    _config.rxPin = GPS_RX_PIN;
    _config.txPin = GPS_TX_PIN;
    _config.enablePin = GPS_ENABLE_PIN;
    _config.resetPin = GPS_RESET_PIN;
    _config.baudRate = GPS_BAUD_RATE;
    _config.fixedLatitude = GPS_LATITUDE_DEFAULT;
    _config.fixedLongitude = GPS_LONGITUDE_DEFAULT;
    _config.fixedAltitude = GPS_ALTITUDE_DEFAULT;
    _config.updateInterval = GPS_UPDATE_INTERVAL;
}

bool GPSManager::begin() {
#if GPS_ENABLED
    if (!_config.enabled) {
        Serial.println(F("[GPS] Disabled in config"));
        return false;
    }

    // Initialize power control pin (HIGH to enable GPS)
    if (_config.enablePin != 255) {
        pinMode(_config.enablePin, OUTPUT);
        digitalWrite(_config.enablePin, LOW);  // Start with GPS off
        _poweredOn = false;
        Serial.printf("[GPS] Power control on GPIO%d\n", _config.enablePin);
    }

    // Initialize reset pin (active LOW)
    if (_config.resetPin != 255) {
        pinMode(_config.resetPin, OUTPUT);
        digitalWrite(_config.resetPin, HIGH);  // Not in reset
        Serial.printf("[GPS] Reset control on GPIO%d\n", _config.resetPin);
    }

    // Use Serial2 for GPS
    _serial = &Serial2;
    _serial->begin(_config.baudRate, SERIAL_8N1, _config.rxPin, _config.txPin);

    _initialized = true;
    _lastReadTime = millis();

    Serial.printf("[GPS] Initialized on RX=%d, TX=%d @ %lu baud\n",
                  _config.rxPin, _config.txPin, _config.baudRate);

    // Power on the GPS module
    powerOn();

    if (_config.useFixedLocation) {
        Serial.println(F("[GPS] Using fixed location"));
        _status.latitude = _config.fixedLatitude;
        _status.longitude = _config.fixedLongitude;
        _status.altitude = _config.fixedAltitude;
        _status.hasFix = true;
    }

    return true;
#else
    Serial.println(F("[GPS] Not enabled in firmware"));
    return false;
#endif
}

void GPSManager::update() {
#if GPS_ENABLED
    if (!_initialized || !_config.enabled) return;

    // If using fixed location, no need to read GPS
    if (_config.useFixedLocation) {
        _status.hasFix = true;
        _status.latitude = _config.fixedLatitude;
        _status.longitude = _config.fixedLongitude;
        _status.altitude = _config.fixedAltitude;
        _status.lastUpdate = millis();
        return;
    }

    // Read GPS data
    processGPSData();
#endif
}

void GPSManager::processGPSData() {
#if GPS_ENABLED
    static char nmeaBuffer[128];
    static uint8_t nmeaIndex = 0;
    static unsigned long lastDebugPrint = 0;
    static unsigned long bytesReceived = 0;
    static unsigned long sentencesReceived = 0;

    while (_serial->available()) {
        char c = _serial->read();
        bytesReceived++;

        if (c == '$') {
            // Start of new sentence
            nmeaIndex = 0;
        }

        if (nmeaIndex < sizeof(nmeaBuffer) - 1) {
            nmeaBuffer[nmeaIndex++] = c;
        }

        if (c == '\n' || c == '\r') {
            if (nmeaIndex > 5) {
                nmeaBuffer[nmeaIndex] = '\0';
                sentencesReceived++;

                // Debug: Print raw NMEA sentences (first 80 chars)
                if (millis() - lastDebugPrint > 5000) {  // Print every 5 seconds
                    Serial.printf("[GPS DEBUG] Raw: %.80s\n", nmeaBuffer);
                    lastDebugPrint = millis();
                }

                parseNMEA(nmeaBuffer);
            }
            nmeaIndex = 0;
        }
    }

    // Periodic debug info
    static unsigned long lastStatsDebug = 0;
    if (millis() - lastStatsDebug > 10000) {  // Every 10 seconds
        Serial.printf("[GPS DEBUG] Bytes: %lu, Sentences: %lu, Fix: %s, Sats: %d\n",
                      bytesReceived, sentencesReceived,
                      _status.hasFix ? "YES" : "NO", _status.satellites);
        lastStatsDebug = millis();
    }
#endif
}

bool GPSManager::parseNMEA(const char* sentence) {
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        parseGPGGA(sentence);
        return true;
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        parseGPRMC(sentence);
        return true;
    }
    return false;
}

void GPSManager::parseGPGGA(const char* sentence) {
    // $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
    char buf[128];
    strncpy(buf, sentence, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* token;
    char* rest = buf;
    int field = 0;

    double lat = 0, lon = 0;
    char latDir = 'N', lonDir = 'E';
    int quality = 0;
    int sats = 0;
    double hdop = 99.9;
    double alt = 0;

    while ((token = strtok_r(rest, ",", &rest))) {
        switch (field) {
            case 1: // Time
                if (strlen(token) >= 6) {
                    char timeStr[16];
                    snprintf(timeStr, sizeof(timeStr), "%.2s:%.2s:%.2s",
                             token, token + 2, token + 4);
                    // Time is stored in dateTime along with date from RMC
                }
                break;
            case 2: // Latitude
                if (strlen(token) > 0) {
                    lat = atof(token);
                    // Convert from ddmm.mmmm to decimal degrees
                    int deg = (int)(lat / 100);
                    double min = lat - (deg * 100);
                    lat = deg + (min / 60.0);
                }
                break;
            case 3: // N/S
                if (strlen(token) > 0) latDir = token[0];
                break;
            case 4: // Longitude
                if (strlen(token) > 0) {
                    lon = atof(token);
                    // Convert from dddmm.mmmm to decimal degrees
                    int deg = (int)(lon / 100);
                    double min = lon - (deg * 100);
                    lon = deg + (min / 60.0);
                }
                break;
            case 5: // E/W
                if (strlen(token) > 0) lonDir = token[0];
                break;
            case 6: // Quality
                quality = atoi(token);
                break;
            case 7: // Satellites
                sats = atoi(token);
                break;
            case 8: // HDOP
                if (strlen(token) > 0) hdop = atof(token);
                break;
            case 9: // Altitude
                if (strlen(token) > 0) alt = atof(token);
                break;
        }
        field++;
    }

    // Apply direction
    if (latDir == 'S') lat = -lat;
    if (lonDir == 'W') lon = -lon;

    // Update status if valid
    if (quality > 0) {
        _status.hasFix = true;
        _status.latitude = lat;
        _status.longitude = lon;
        _status.altitude = alt;
        _status.satellites = sats;
        _status.hdop = hdop;
        _status.lastUpdate = millis();
        _status.fixAge = 0;
        _status.validFixes++;
    } else {
        _status.hasFix = false;
        _status.fixAge = millis() - _status.lastUpdate;
        _status.failedFixes++;
    }
}

void GPSManager::parseGPRMC(const char* sentence) {
    // $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
    char buf[128];
    strncpy(buf, sentence, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* token;
    char* rest = buf;
    int field = 0;

    char timeStr[12] = "";
    char dateStr[12] = "";
    double speed = 0;
    double course = 0;
    bool valid = false;

    while ((token = strtok_r(rest, ",", &rest))) {
        switch (field) {
            case 1: // Time
                if (strlen(token) >= 6) {
                    snprintf(timeStr, sizeof(timeStr), "%.2s:%.2s:%.2s",
                             token, token + 2, token + 4);
                }
                break;
            case 2: // Status A=valid, V=invalid
                valid = (token[0] == 'A');
                break;
            case 7: // Speed (knots)
                if (strlen(token) > 0) {
                    speed = atof(token) * 1.852; // Convert to km/h
                }
                break;
            case 8: // Course
                if (strlen(token) > 0) {
                    course = atof(token);
                }
                break;
            case 9: // Date
                if (strlen(token) >= 6) {
                    snprintf(dateStr, sizeof(dateStr), "20%.2s-%.2s-%.2s",
                             token + 4, token + 2, token);
                }
                break;
        }
        field++;
    }

    if (valid) {
        _status.speed = speed;
        _status.course = course;
        snprintf(_status.dateTime, sizeof(_status.dateTime), "%sT%sZ", dateStr, timeStr);
    }
}

bool GPSManager::isAvailable() const {
    return _initialized && _config.enabled;
}

bool GPSManager::hasFix() const {
    return _status.hasFix || _config.useFixedLocation;
}

double GPSManager::getLatitude() const {
    if (_config.useFixedLocation) return _config.fixedLatitude;
    return _status.latitude;
}

double GPSManager::getLongitude() const {
    if (_config.useFixedLocation) return _config.fixedLongitude;
    return _status.longitude;
}

int32_t GPSManager::getAltitude() const {
    if (_config.useFixedLocation) return _config.fixedAltitude;
    return (int32_t)_status.altitude;
}

void GPSManager::loadConfig(const JsonDocument& doc) {
#if GPS_ENABLED
    if (!doc.containsKey("gps")) {
        Serial.println(F("[GPS] No GPS config in JSON, using defaults"));
        return;
    }

    JsonObjectConst gpsCfg = doc["gps"];

    _config.enabled = gpsCfg["enabled"] | GPS_ENABLED_DEFAULT;
    _config.useFixedLocation = gpsCfg["use_fixed"] | GPS_USE_FIXED_DEFAULT;
    _config.rxPin = gpsCfg["rx_pin"] | GPS_RX_PIN;
    _config.txPin = gpsCfg["tx_pin"] | GPS_TX_PIN;
    _config.enablePin = gpsCfg["enable_pin"] | GPS_ENABLE_PIN;
    _config.resetPin = gpsCfg["reset_pin"] | GPS_RESET_PIN;
    _config.baudRate = gpsCfg["baud_rate"] | GPS_BAUD_RATE;
    _config.fixedLatitude = gpsCfg["latitude"] | GPS_LATITUDE_DEFAULT;
    _config.fixedLongitude = gpsCfg["longitude"] | GPS_LONGITUDE_DEFAULT;
    _config.fixedAltitude = gpsCfg["altitude"] | GPS_ALTITUDE_DEFAULT;
    _config.updateInterval = gpsCfg["update_interval"] | GPS_UPDATE_INTERVAL;

    Serial.printf("[GPS] Config loaded: enabled=%d, fixed=%d, lat=%.6f, lon=%.6f\n",
                  _config.enabled, _config.useFixedLocation,
                  _config.fixedLatitude, _config.fixedLongitude);
    Serial.printf("[GPS] Pins: RX=%d, TX=%d, EN=%d, RST=%d\n",
                  _config.rxPin, _config.txPin, _config.enablePin, _config.resetPin);
#endif
}

bool GPSManager::saveConfig() {
#if GPS_ENABLED
    // Read existing config
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println(F("[GPS] Cannot read config file"));
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("[GPS] JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }

    // Update GPS section
    JsonObject gpsCfg = doc.createNestedObject("gps");
    gpsCfg["enabled"] = _config.enabled;
    gpsCfg["use_fixed"] = _config.useFixedLocation;
    gpsCfg["rx_pin"] = _config.rxPin;
    gpsCfg["tx_pin"] = _config.txPin;
    gpsCfg["enable_pin"] = _config.enablePin;
    gpsCfg["reset_pin"] = _config.resetPin;
    gpsCfg["baud_rate"] = _config.baudRate;
    gpsCfg["latitude"] = _config.fixedLatitude;
    gpsCfg["longitude"] = _config.fixedLongitude;
    gpsCfg["altitude"] = _config.fixedAltitude;
    gpsCfg["update_interval"] = _config.updateInterval;

    // Write back
    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println(F("[GPS] Cannot write config file"));
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println(F("[GPS] Config saved"));
    return true;
#else
    return false;
#endif
}

String GPSManager::getStatusJson() {
    DynamicJsonDocument doc(512);

    doc["enabled"] = _config.enabled;
    doc["use_fixed"] = _config.useFixedLocation;
    doc["has_fix"] = hasFix();
    doc["powered_on"] = _poweredOn;
    doc["satellites"] = _status.satellites;
    doc["latitude"] = getLatitude();
    doc["longitude"] = getLongitude();
    doc["altitude"] = getAltitude();
    doc["speed"] = _status.speed;
    doc["course"] = _status.course;
    doc["hdop"] = _status.hdop;
    doc["fix_age"] = _status.fixAge;
    doc["valid_fixes"] = _status.validFixes;
    doc["failed_fixes"] = _status.failedFixes;
    doc["date_time"] = _status.dateTime;

    // Config info
    doc["rx_pin"] = _config.rxPin;
    doc["tx_pin"] = _config.txPin;
    doc["enable_pin"] = _config.enablePin;
    doc["reset_pin"] = _config.resetPin;
    doc["baud_rate"] = _config.baudRate;

    String response;
    serializeJson(doc, response);
    return response;
}

void GPSManager::powerOn() {
#if GPS_ENABLED
    if (_config.enablePin != 255) {
        digitalWrite(_config.enablePin, HIGH);
        _poweredOn = true;
        Serial.println(F("[GPS] Powered ON"));
        delay(100);  // Allow GPS to stabilize
    }
#endif
}

void GPSManager::powerOff() {
#if GPS_ENABLED
    if (_config.enablePin != 255) {
        digitalWrite(_config.enablePin, LOW);
        _poweredOn = false;
        Serial.println(F("[GPS] Powered OFF"));
    }
#endif
}

void GPSManager::reset() {
#if GPS_ENABLED
    if (_config.resetPin != 255) {
        Serial.println(F("[GPS] Resetting..."));
        digitalWrite(_config.resetPin, LOW);   // Assert reset
        delay(100);                             // Hold reset for 100ms
        digitalWrite(_config.resetPin, HIGH);  // Release reset
        delay(500);                             // Wait for GPS to restart
        Serial.println(F("[GPS] Reset complete"));

        // Clear status after reset
        _status.hasFix = false;
        _status.satellites = 0;
        _status.validFixes = 0;
        _status.failedFixes = 0;
    }
#endif
}
