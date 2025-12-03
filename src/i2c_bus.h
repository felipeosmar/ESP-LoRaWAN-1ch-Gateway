#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <Arduino.h>
#include <Wire.h>

// =============================================================================
// I2C Bus Configuration
// =============================================================================
// Centralized I2C bus management for all I2C devices (LCD, RTC, sensors, etc.)

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif

#ifndef I2C_FREQUENCY
#define I2C_FREQUENCY 100000  // 100kHz standard mode
#endif

class I2CBus {
public:
    I2CBus();

    // Initialize the I2C bus (call once at startup)
    bool begin(int sdaPin = I2C_SDA_PIN, int sclPin = I2C_SCL_PIN, uint32_t frequency = I2C_FREQUENCY);

    // Check if bus is initialized
    bool isInitialized() const { return _initialized; }

    // Get current pin configuration
    int getSDA() const { return _sdaPin; }
    int getSCL() const { return _sclPin; }
    uint32_t getFrequency() const { return _frequency; }

    // Scan for devices on the bus
    void scan();

    // Check if a specific device is present
    bool devicePresent(uint8_t address);

    // Get list of detected devices (call after scan)
    uint8_t getDeviceCount() const { return _deviceCount; }
    uint8_t getDeviceAddress(uint8_t index) const;

private:
    bool _initialized;
    int _sdaPin;
    int _sclPin;
    uint32_t _frequency;
    uint8_t _devices[16];  // Max 16 devices tracked
    uint8_t _deviceCount;
};

// Global instance
extern I2CBus i2cBus;

#endif // I2C_BUS_H
