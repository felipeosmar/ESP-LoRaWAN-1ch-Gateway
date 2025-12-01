#include "rtc_manager.h"
#include <LittleFS.h>

// Global instance
RTCManager rtcManager;

// Day and month names
static const char* DAY_NAMES[] = {"", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* MONTH_NAMES[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

RTCManager::RTCManager() : _initialized(false), _lastSyncCheck(0), _lastNTPSync(0) {
    setDefaultConfig();
    memset(&status, 0, sizeof(status));
}

void RTCManager::setDefaultConfig() {
    config.enabled = RTC_ENABLED;
    config.i2cAddress = RTC_ADDRESS;
    config.sdaPin = RTC_SDA;
    config.sclPin = RTC_SCL;
    config.syncWithNTP = true;
    config.syncInterval = 3600;  // 1 hour default
    config.squareWaveMode = DS1307_SQW_OFF;
    config.timezoneOffset = -3;  // BRT default
}

bool RTCManager::begin() {
    if (!config.enabled) {
        Serial.println("[RTC] Disabled in config");
        return false;
    }

    Serial.println("[RTC] Initializing DS1307...");
    Serial.printf("[RTC] I2C Address: 0x%02X, SDA: %d, SCL: %d\n",
                  config.i2cAddress, config.sdaPin, config.sclPin);

    // I2C should already be initialized by LCD manager or main
    // Just verify the device is present
    if (!detectDevice()) {
        Serial.println("[RTC] DS1307 not found on I2C bus!");
        status.available = false;
        return false;
    }

    Serial.println("[RTC] DS1307 detected");
    status.available = true;

    // Check if oscillator is running
    if (!isOscillatorRunning()) {
        Serial.println("[RTC] Oscillator stopped, starting...");
        startOscillator();
    }

    status.oscillatorRunning = isOscillatorRunning();
    Serial.printf("[RTC] Oscillator running: %s\n", status.oscillatorRunning ? "Yes" : "No");
    Serial.flush();  // Ensure message is sent before potentially blocking operations

    // Set square wave mode
    Serial.println("[RTC] Setting square wave mode...");
    Serial.flush();
    yield();  // Give watchdog a chance

    if (!setSquareWave(config.squareWaveMode)) {
        Serial.println("[RTC] Warning: Failed to set square wave mode");
    }
    Serial.println("[RTC] Square wave configured");
    Serial.flush();

    // Read current time
    Serial.println("[RTC] Reading current time...");
    Serial.flush();
    yield();

    RTCDateTime dt;
    if (getDateTime(dt)) {
        status.currentTime = dt;
        Serial.printf("[RTC] Current time: %s\n", getFormattedDateTime().c_str());
        Serial.flush();
    } else {
        Serial.println("[RTC] Warning: Failed to read initial time");
        Serial.flush();
    }

    yield();
    delay(10);  // Small delay to let serial complete

    _initialized = true;
    Serial.println("[RTC] Init complete");
    Serial.flush();
    delay(10);

    return true;
}

void RTCManager::update() {
    if (!_initialized || !config.enabled || !status.available) return;

    unsigned long now = millis();

    // Periodic time read (every second)
    static unsigned long lastRead = 0;
    if (now - lastRead >= 1000) {
        lastRead = now;
        RTCDateTime dt;
        if (getDateTime(dt)) {
            status.currentTime = dt;
        }
    }

    // NTP sync check (if enabled and interval > 0)
    if (config.syncWithNTP && config.syncInterval > 0) {
        if (now - _lastSyncCheck >= config.syncInterval * 1000UL) {
            _lastSyncCheck = now;
            // Check if we should sync with NTP
            // This will be called from main loop when NTP time is available
        }
    }
}

bool RTCManager::detectDevice() {
    Wire.beginTransmission(config.i2cAddress);
    return (Wire.endTransmission() == 0);
}

bool RTCManager::isOscillatorRunning() {
    uint8_t seconds = readRegister(DS1307_REG_SECONDS);
    // Bit 7 of seconds register: 0 = running, 1 = stopped
    return !(seconds & 0x80);
}

void RTCManager::startOscillator() {
    uint8_t seconds = readRegister(DS1307_REG_SECONDS);
    // Clear bit 7 to start oscillator
    writeRegister(DS1307_REG_SECONDS, seconds & 0x7F);
}

bool RTCManager::getDateTime(RTCDateTime& dt) {
    uint8_t data[7];

    if (!readRegisters(DS1307_REG_SECONDS, data, 7)) {
        status.errorCount++;
        return false;
    }

    dt.seconds = bcdToDec(data[0] & 0x7F);  // Mask out CH bit
    dt.minutes = bcdToDec(data[1] & 0x7F);
    dt.hours = bcdToDec(data[2] & 0x3F);    // 24-hour mode
    dt.dayOfWeek = data[3] & 0x07;
    dt.day = bcdToDec(data[4] & 0x3F);
    dt.month = bcdToDec(data[5] & 0x1F);
    dt.year = bcdToDec(data[6]) + 2000;

    status.readCount++;
    return true;
}

bool RTCManager::setDateTime(const RTCDateTime& dt) {
    uint8_t data[7];

    data[0] = decToBcd(dt.seconds) & 0x7F;  // Clear CH bit to keep oscillator running
    data[1] = decToBcd(dt.minutes);
    data[2] = decToBcd(dt.hours);           // 24-hour mode
    data[3] = dt.dayOfWeek;
    data[4] = decToBcd(dt.day);
    data[5] = decToBcd(dt.month);
    data[6] = decToBcd(dt.year - 2000);

    if (!writeRegisters(DS1307_REG_SECONDS, data, 7)) {
        status.errorCount++;
        return false;
    }

    status.writeCount++;
    status.timeSynced = true;
    status.lastSyncTime = getEpochTime();

    Serial.printf("[RTC] Time set: %04d-%02d-%02d %02d:%02d:%02d\n",
                  dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds);

    return true;
}

bool RTCManager::setTimeFromEpoch(time_t epoch) {
    struct tm timeinfo;
    gmtime_r(&epoch, &timeinfo);

    // Apply timezone offset
    epoch += config.timezoneOffset * 3600;
    localtime_r(&epoch, &timeinfo);

    RTCDateTime dt;
    dt.seconds = timeinfo.tm_sec;
    dt.minutes = timeinfo.tm_min;
    dt.hours = timeinfo.tm_hour;
    dt.day = timeinfo.tm_mday;
    dt.month = timeinfo.tm_mon + 1;
    dt.year = timeinfo.tm_year + 1900;
    dt.dayOfWeek = timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday;  // Convert 0-6 to 1-7

    return setDateTime(dt);
}

bool RTCManager::setTimeFromNTP() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 1000)) {
        Serial.println("[RTC] Failed to get NTP time");
        return false;
    }

    RTCDateTime dt;
    dt.seconds = timeinfo.tm_sec;
    dt.minutes = timeinfo.tm_min;
    dt.hours = timeinfo.tm_hour;
    dt.day = timeinfo.tm_mday;
    dt.month = timeinfo.tm_mon + 1;
    dt.year = timeinfo.tm_year + 1900;
    dt.dayOfWeek = timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday;

    if (setDateTime(dt)) {
        _lastNTPSync = millis();
        Serial.println("[RTC] Time synchronized from NTP");
        return true;
    }
    return false;
}

time_t RTCManager::getEpochTime() {
    RTCDateTime dt;
    if (!getDateTime(dt)) {
        return 0;
    }

    struct tm timeinfo;
    timeinfo.tm_sec = dt.seconds;
    timeinfo.tm_min = dt.minutes;
    timeinfo.tm_hour = dt.hours;
    timeinfo.tm_mday = dt.day;
    timeinfo.tm_mon = dt.month - 1;
    timeinfo.tm_year = dt.year - 1900;
    timeinfo.tm_isdst = 0;

    return mktime(&timeinfo);
}

String RTCManager::getFormattedDate() {
    char buf[12];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             status.currentTime.year, status.currentTime.month, status.currentTime.day);
    return String(buf);
}

String RTCManager::getFormattedTime() {
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             status.currentTime.hours, status.currentTime.minutes, status.currentTime.seconds);
    return String(buf);
}

String RTCManager::getFormattedDateTime() {
    char buf[22];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             status.currentTime.year, status.currentTime.month, status.currentTime.day,
             status.currentTime.hours, status.currentTime.minutes, status.currentTime.seconds);
    return String(buf);
}

String RTCManager::getIsoTimestamp() {
    char buf[26];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d%+03d:00",
             status.currentTime.year, status.currentTime.month, status.currentTime.day,
             status.currentTime.hours, status.currentTime.minutes, status.currentTime.seconds,
             config.timezoneOffset);
    return String(buf);
}

bool RTCManager::setSquareWave(uint8_t mode) {
    uint8_t control = 0;

    switch (mode) {
        case DS1307_SQW_1HZ:
            control = 0x10;  // SQWE=1, RS=00 (1Hz)
            break;
        case DS1307_SQW_4KHZ:
            control = 0x11;  // SQWE=1, RS=01 (4.096kHz)
            break;
        case DS1307_SQW_8KHZ:
            control = 0x12;  // SQWE=1, RS=10 (8.192kHz)
            break;
        case DS1307_SQW_32KHZ:
            control = 0x13;  // SQWE=1, RS=11 (32.768kHz)
            break;
        default:
            control = 0x00;  // SQWE=0 (off)
            break;
    }

    return writeRegister(DS1307_REG_CONTROL, control);
}

uint8_t RTCManager::getSquareWaveMode() {
    uint8_t control = readRegister(DS1307_REG_CONTROL);
    if (!(control & 0x10)) {
        return DS1307_SQW_OFF;
    }
    return (control & 0x03) + 0x10;
}

void RTCManager::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("rtc")) {
        Serial.println("[RTC] No config in JSON, using defaults");
        return;
    }

    JsonObjectConst rtc = doc["rtc"];

    config.enabled = rtc["enabled"] | RTC_ENABLED;
    config.i2cAddress = rtc["i2cAddress"] | RTC_ADDRESS;
    config.sdaPin = rtc["sdaPin"] | RTC_SDA;
    config.sclPin = rtc["sclPin"] | RTC_SCL;
    config.syncWithNTP = rtc["syncWithNTP"] | true;
    config.syncInterval = rtc["syncInterval"] | 3600;
    config.squareWaveMode = rtc["squareWaveMode"] | DS1307_SQW_OFF;
    config.timezoneOffset = rtc["timezoneOffset"] | -3;

    Serial.printf("[RTC] Config loaded: enabled=%d, addr=0x%02X, tz=%d\n",
                  config.enabled, config.i2cAddress, config.timezoneOffset);
}

bool RTCManager::saveConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[RTC] Failed to open config file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[RTC] JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Update RTC section
    JsonObject rtc = doc["rtc"].to<JsonObject>();
    rtc["enabled"] = config.enabled;
    rtc["i2cAddress"] = config.i2cAddress;
    rtc["sdaPin"] = config.sdaPin;
    rtc["sclPin"] = config.sclPin;
    rtc["syncWithNTP"] = config.syncWithNTP;
    rtc["syncInterval"] = config.syncInterval;
    rtc["squareWaveMode"] = config.squareWaveMode;
    rtc["timezoneOffset"] = config.timezoneOffset;

    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[RTC] Failed to open config file for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println("[RTC] Configuration saved");
    return true;
}

String RTCManager::getStatusJson() {
    JsonDocument doc;

    doc["available"] = status.available;
    doc["oscillatorRunning"] = status.oscillatorRunning;
    doc["timeSynced"] = status.timeSynced;
    doc["lastSyncTime"] = status.lastSyncTime;
    doc["readCount"] = status.readCount;
    doc["writeCount"] = status.writeCount;
    doc["errorCount"] = status.errorCount;

    JsonObject time = doc["currentTime"].to<JsonObject>();
    time["year"] = status.currentTime.year;
    time["month"] = status.currentTime.month;
    time["day"] = status.currentTime.day;
    time["hours"] = status.currentTime.hours;
    time["minutes"] = status.currentTime.minutes;
    time["seconds"] = status.currentTime.seconds;
    time["dayOfWeek"] = status.currentTime.dayOfWeek;
    time["dayName"] = getDayName(status.currentTime.dayOfWeek);

    doc["formattedDate"] = getFormattedDate();
    doc["formattedTime"] = getFormattedTime();
    doc["formattedDateTime"] = getFormattedDateTime();
    doc["isoTimestamp"] = getIsoTimestamp();
    doc["epochTime"] = getEpochTime();

    String response;
    serializeJson(doc, response);
    return response;
}

const char* RTCManager::getDayName(uint8_t dayOfWeek) {
    if (dayOfWeek >= 1 && dayOfWeek <= 7) {
        return DAY_NAMES[dayOfWeek];
    }
    return "???";
}

const char* RTCManager::getMonthName(uint8_t month) {
    if (month >= 1 && month <= 12) {
        return MONTH_NAMES[month];
    }
    return "???";
}

uint8_t RTCManager::calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
    // Zeller's congruence
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    // Convert to 1=Sunday, 7=Saturday
    return ((h + 6) % 7) + 1;
}

// I2C helper functions with timeout protection
bool RTCManager::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(config.i2cAddress);
    Wire.write(reg);
    Wire.write(value);
    uint8_t result = Wire.endTransmission(true);  // Send stop
    if (result != 0) {
        Serial.printf("[RTC] I2C write error: %d\n", result);
        return false;
    }
    return true;
}

uint8_t RTCManager::readRegister(uint8_t reg) {
    Wire.beginTransmission(config.i2cAddress);
    Wire.write(reg);
    uint8_t result = Wire.endTransmission(false);  // Repeated start
    if (result != 0) {
        Serial.printf("[RTC] I2C write error in readReg: %d\n", result);
        return 0;
    }

    uint8_t received = Wire.requestFrom(config.i2cAddress, (uint8_t)1);
    if (received != 1) {
        Serial.printf("[RTC] I2C read error: expected 1, got %d\n", received);
        return 0;
    }

    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

bool RTCManager::writeRegisters(uint8_t startReg, uint8_t* data, uint8_t len) {
    Wire.beginTransmission(config.i2cAddress);
    Wire.write(startReg);
    for (uint8_t i = 0; i < len; i++) {
        Wire.write(data[i]);
    }
    uint8_t result = Wire.endTransmission(true);
    if (result != 0) {
        Serial.printf("[RTC] I2C multi-write error: %d\n", result);
        return false;
    }
    return true;
}

bool RTCManager::readRegisters(uint8_t startReg, uint8_t* data, uint8_t len) {
    Wire.beginTransmission(config.i2cAddress);
    Wire.write(startReg);
    uint8_t result = Wire.endTransmission(false);  // Repeated start
    if (result != 0) {
        Serial.printf("[RTC] I2C write error in readRegs: %d\n", result);
        return false;
    }

    uint8_t received = Wire.requestFrom(config.i2cAddress, len);
    if (received != len) {
        Serial.printf("[RTC] I2C multi-read error: expected %d, got %d\n", len, received);
        // Read whatever is available anyway
    }

    uint8_t count = 0;
    unsigned long timeout = millis() + 100;  // 100ms timeout
    while (count < len && millis() < timeout) {
        if (Wire.available()) {
            data[count++] = Wire.read();
        }
        yield();  // Prevent watchdog
    }

    return (count == len);
}
