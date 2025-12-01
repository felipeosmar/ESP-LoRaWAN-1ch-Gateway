/**
 * @file buzzer_manager.h
 * @brief Buzzer Manager for ESP32
 *
 * Gateway LoRa JVTECH v4.1
 *
 * Controls the buzzer connected to GPIO26 on ESP32.
 * Uses ESP32 LEDC PWM for tone generation.
 */

#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// Default buzzer frequencies for common sounds
#define BUZZER_FREQ_LOW     1000
#define BUZZER_FREQ_MED     2000
#define BUZZER_FREQ_HIGH    3000
#define BUZZER_FREQ_ERROR   500
#define BUZZER_FREQ_SUCCESS 2500

// LEDC configuration for buzzer
#define BUZZER_LEDC_CHANNEL  0
#define BUZZER_LEDC_RESOLUTION 8

// Buzzer configuration structure
struct BuzzerConfig {
    bool enabled = true;
    bool startupSound = true;
    bool packetRxSound = true;
    bool packetTxSound = false;
    uint8_t volume = 75;  // 0-100%
};

class BuzzerManager {
public:
    /**
     * @brief Initialize the buzzer
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Play a tone
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds (0 = continuous)
     */
    void tone(uint16_t frequency, uint16_t duration = 0);

    /**
     * @brief Stop the buzzer
     */
    void stop();

    /**
     * @brief Play a beep (non-blocking)
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     */
    void beep(uint16_t frequency = BUZZER_FREQ_MED, uint16_t duration = 100);

    /**
     * @brief Play multiple beeps
     * @param count Number of beeps
     * @param frequency Frequency in Hz
     * @param onTime On duration in ms
     * @param offTime Off duration in ms
     */
    void beepMultiple(uint8_t count, uint16_t frequency = BUZZER_FREQ_MED,
                      uint16_t onTime = 100, uint16_t offTime = 100);

    /**
     * @brief Play startup sound
     */
    void playStartup();

    /**
     * @brief Play success sound
     */
    void playSuccess();

    /**
     * @brief Play error sound
     */
    void playError();

    /**
     * @brief Play packet received sound
     */
    void playPacketRx();

    /**
     * @brief Play packet transmitted sound
     */
    void playPacketTx();

    /**
     * @brief Update buzzer state (call from loop for non-blocking beeps)
     */
    void update();

    /**
     * @brief Check if buzzer is currently playing
     * @return true if playing
     */
    bool isPlaying() const;

    /**
     * @brief Enable or disable buzzer
     * @param enabled true to enable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if buzzer is enabled
     * @return true if enabled
     */
    bool isEnabled() const;

    /**
     * @brief Get configuration reference
     * @return Reference to BuzzerConfig
     */
    BuzzerConfig& getConfig();

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

private:
    bool _initialized = false;
    bool _enabled = true;
    bool _playing = false;
    unsigned long _stopTime = 0;
    BuzzerConfig _config;

    // For multiple beeps
    uint8_t _beepCount = 0;
    uint8_t _beepRemaining = 0;
    uint16_t _beepFrequency = 0;
    uint16_t _beepOnTime = 0;
    uint16_t _beepOffTime = 0;
    bool _beepState = false;
    unsigned long _beepNextTime = 0;
};

// Global instance
extern BuzzerManager buzzer;

#endif // BUZZER_MANAGER_H
