/**
 * @file buzzer_manager.cpp
 * @brief Buzzer Manager implementation for ESP32
 *
 * Gateway LoRa JVTECH v4.1
 */

#include "buzzer_manager.h"
#include <LittleFS.h>

// Global instance
BuzzerManager buzzer;

bool BuzzerManager::begin() {
#if BUZZER_ENABLED
    // Configure LEDC for PWM tone generation
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, BUZZER_LEDC_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);  // Start silent

    _initialized = true;
    _enabled = _config.enabled;

    Serial.print(F("[BUZZER] Initialized on GPIO "));
    Serial.println(BUZZER_PIN);

    // Play startup sound if enabled
    if (_config.startupSound && _enabled) {
        playStartup();
    }

    return true;
#else
    _initialized = false;
    _enabled = false;
    return false;
#endif
}

void BuzzerManager::tone(uint16_t frequency, uint16_t duration) {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled) return;

    if (frequency > 0) {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, frequency);
        _playing = true;

        if (duration > 0) {
            _stopTime = millis() + duration;
        } else {
            _stopTime = 0;  // Continuous
        }
    } else {
        stop();
    }
#endif
}

void BuzzerManager::stop() {
#if BUZZER_ENABLED
    if (!_initialized) return;

    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
    _playing = false;
    _stopTime = 0;
    _beepRemaining = 0;
#endif
}

void BuzzerManager::beep(uint16_t frequency, uint16_t duration) {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled) return;

    tone(frequency, duration);
#endif
}

void BuzzerManager::beepMultiple(uint8_t count, uint16_t frequency,
                                  uint16_t onTime, uint16_t offTime) {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled || count == 0) return;

    _beepCount = count;
    _beepRemaining = count;
    _beepFrequency = frequency;
    _beepOnTime = onTime;
    _beepOffTime = offTime;
    _beepState = true;
    _beepNextTime = millis() + onTime;

    tone(frequency, 0);  // Start first beep
#endif
}

void BuzzerManager::playStartup() {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled) return;

    // Rising tone sequence
    tone(1000, 100);
    delay(120);
    tone(1500, 100);
    delay(120);
    tone(2000, 150);
    delay(170);
    stop();
#endif
}

void BuzzerManager::playSuccess() {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled) return;

    tone(2000, 100);
    delay(120);
    tone(2500, 150);
    delay(170);
    stop();
#endif
}

void BuzzerManager::playError() {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled) return;

    tone(500, 200);
    delay(250);
    tone(400, 300);
    delay(350);
    stop();
#endif
}

void BuzzerManager::playPacketRx() {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled || !_config.packetRxSound) return;

    // Short high beep for received packet
    beep(2500, 50);
#endif
}

void BuzzerManager::playPacketTx() {
#if BUZZER_ENABLED
    if (!_initialized || !_enabled || !_config.packetTxSound) return;

    // Short low beep for transmitted packet
    beep(1500, 50);
#endif
}

void BuzzerManager::update() {
#if BUZZER_ENABLED
    if (!_initialized) return;

    unsigned long now = millis();

    // Handle single tone with duration
    if (_playing && _stopTime > 0 && now >= _stopTime) {
        stop();
        return;
    }

    // Handle multiple beeps
    if (_beepRemaining > 0 && now >= _beepNextTime) {
        if (_beepState) {
            // Was ON, turn OFF
            ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
            _beepState = false;
            _beepNextTime = now + _beepOffTime;
            _beepRemaining--;

            if (_beepRemaining == 0) {
                _playing = false;
            }
        } else {
            // Was OFF, turn ON
            if (_beepRemaining > 0) {
                ledcWriteTone(BUZZER_LEDC_CHANNEL, _beepFrequency);
                _beepState = true;
                _beepNextTime = now + _beepOnTime;
            }
        }
    }
#endif
}

bool BuzzerManager::isPlaying() const {
    return _playing || _beepRemaining > 0;
}

void BuzzerManager::setEnabled(bool enabled) {
    _enabled = enabled;
    _config.enabled = enabled;
    if (!enabled) {
        stop();
    }
}

bool BuzzerManager::isEnabled() const {
    return _enabled;
}

BuzzerConfig& BuzzerManager::getConfig() {
    return _config;
}

void BuzzerManager::loadConfig(const JsonDocument& doc) {
#if BUZZER_ENABLED
    if (!doc.containsKey("buzzer")) {
        Serial.println(F("[BUZZER] No buzzer config in JSON, using defaults"));
        return;
    }

    JsonObjectConst buzzerCfg = doc["buzzer"];

    _config.enabled = buzzerCfg["enabled"] | true;
    _config.startupSound = buzzerCfg["startup_sound"] | true;
    _config.packetRxSound = buzzerCfg["packet_rx_sound"] | true;
    _config.packetTxSound = buzzerCfg["packet_tx_sound"] | false;
    _config.volume = buzzerCfg["volume"] | 75;

    _enabled = _config.enabled;

    Serial.println(F("[BUZZER] Config loaded"));
#endif
}

bool BuzzerManager::saveConfig() {
#if BUZZER_ENABLED
    // Read existing config
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println(F("[BUZZER] Cannot read config file"));
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("[BUZZER] JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }

    // Update buzzer section
    JsonObject buzzerCfg = doc.createNestedObject("buzzer");
    buzzerCfg["enabled"] = _config.enabled;
    buzzerCfg["startup_sound"] = _config.startupSound;
    buzzerCfg["packet_rx_sound"] = _config.packetRxSound;
    buzzerCfg["packet_tx_sound"] = _config.packetTxSound;
    buzzerCfg["volume"] = _config.volume;

    // Write back
    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println(F("[BUZZER] Cannot write config file"));
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    Serial.println(F("[BUZZER] Config saved"));
    return true;
#else
    return false;
#endif
}
