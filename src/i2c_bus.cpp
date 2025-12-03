#include "i2c_bus.h"

// Global instance
I2CBus i2cBus;

I2CBus::I2CBus() : _initialized(false), _sdaPin(I2C_SDA_PIN), _sclPin(I2C_SCL_PIN),
                   _frequency(I2C_FREQUENCY), _deviceCount(0) {
    memset(_devices, 0, sizeof(_devices));
}

bool I2CBus::begin(int sdaPin, int sclPin, uint32_t frequency) {
    if (_initialized) {
        Serial.println("[I2C] Bus already initialized");
        return true;
    }

    _sdaPin = sdaPin;
    _sclPin = sclPin;
    _frequency = frequency;

    Serial.println("[I2C] Initializing I2C bus...");
    Serial.printf("[I2C] SDA: GPIO%d, SCL: GPIO%d, Frequency: %lu Hz\n",
                  _sdaPin, _sclPin, _frequency);

    // Initialize Wire with specified pins
    if (!Wire.begin(_sdaPin, _sclPin)) {
        Serial.println("[I2C] Failed to initialize Wire!");
        return false;
    }

    // Set clock frequency
    Wire.setClock(_frequency);

    _initialized = true;
    Serial.println("[I2C] Bus initialized successfully");

    // Scan for devices
    scan();

    return true;
}

void I2CBus::scan() {
    if (!_initialized) {
        Serial.println("[I2C] Bus not initialized, cannot scan");
        return;
    }

    Serial.println("[I2C] Scanning for devices...");
    _deviceCount = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            if (_deviceCount < sizeof(_devices)) {
                _devices[_deviceCount++] = addr;
            }
            Serial.printf("[I2C] Device found at 0x%02X", addr);

            // Identify common devices
            switch (addr) {
                case 0x27:
                case 0x3F:
                    Serial.print(" (LCD PCF8574)");
                    break;
                case 0x68:
                    Serial.print(" (DS1307/DS3231 RTC)");
                    break;
                case 0x3C:
                case 0x3D:
                    Serial.print(" (OLED SSD1306)");
                    break;
                case 0x50:
                case 0x57:
                    Serial.print(" (EEPROM AT24C32)");
                    break;
                case 0x76:
                case 0x77:
                    Serial.print(" (BME280/BMP280)");
                    break;
                default:
                    break;
            }
            Serial.println();
        }
    }

    if (_deviceCount == 0) {
        Serial.println("[I2C] No devices found!");
    } else {
        Serial.printf("[I2C] Found %d device(s)\n", _deviceCount);
    }
}

bool I2CBus::devicePresent(uint8_t address) {
    if (!_initialized) {
        return false;
    }

    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}

uint8_t I2CBus::getDeviceAddress(uint8_t index) const {
    if (index < _deviceCount) {
        return _devices[index];
    }
    return 0;
}
