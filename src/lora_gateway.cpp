#include "lora_gateway.h"
#include <LittleFS.h>

// Global instance
LoRaGateway loraGateway;

// Static interrupt flag
volatile bool LoRaGateway::dio0Flag = false;

// Interrupt handler
void IRAM_ATTR LoRaGateway::onDio0Rise() {
    dio0Flag = true;
}

LoRaGateway::LoRaGateway()
    : radio(nullptr)
    , spi(nullptr)
    , available(false)
    , receiving(false)
    , queueHead(0)
    , queueTail(0) {

    memset(&stats, 0, sizeof(stats));
    setDefaultConfig();
}

void LoRaGateway::setDefaultConfig() {
    config.enabled = true;
    config.frequency = LORA_FREQUENCY_DEFAULT;
    config.spreadingFactor = LORA_SF_DEFAULT;
    config.bandwidth = LORA_BW_DEFAULT;
    config.codingRate = LORA_CR_DEFAULT;
    config.txPower = LORA_POWER_DEFAULT;
    config.syncWord = LORA_SYNC_WORD_DEFAULT;

    config.pinMiso = LORA_MISO;
    config.pinMosi = LORA_MOSI;
    config.pinSck = LORA_SCK;
    config.pinNss = LORA_NSS;
    config.pinRst = LORA_RST;
    config.pinDio0 = LORA_DIO0;
}

bool LoRaGateway::begin() {
    Serial.println("[LoRa] Initializing gateway...");

    // Initialize Vext if available (Heltec boards)
    #if VEXT_PIN >= 0
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);  // Enable Vext (active LOW on Heltec)
    delay(100);
    #endif

    return initRadio();
}

bool LoRaGateway::initRadio() {
    Serial.printf("[LoRa] Pins: NSS=%d, DIO0=%d, RST=%d\n",
                  config.pinNss, config.pinDio0, config.pinRst);
    Serial.printf("[LoRa] SPI: SCK=%d, MISO=%d, MOSI=%d\n",
                  config.pinSck, config.pinMiso, config.pinMosi);

    // === DISABLE OTHER SPI DEVICES ON SHARED BUS ===
    // SD Card CS (CS1) - try common GPIOs used for SD card
    // GPIO 5 is default VSPI SS, often used for SD card
    #ifndef SD_CS_PIN
    #define SD_CS_PIN 5
    #endif

    Serial.printf("[LoRa] Disabling SD Card CS (GPIO %d)...\n", SD_CS_PIN);
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);  // Deselect SD card

    // === SPI DIAGNOSTIC TEST ===
    Serial.println("[LoRa] Running SPI diagnostic...");

    // Configure pins manually for diagnostic
    pinMode(config.pinNss, OUTPUT);
    pinMode(config.pinRst, OUTPUT);
    digitalWrite(config.pinNss, HIGH);  // Deselect LoRa chip

    // Manual reset sequence with longer timing
    Serial.println("[LoRa] Performing manual reset...");
    digitalWrite(config.pinRst, LOW);
    delay(20);  // Increased from 10ms
    digitalWrite(config.pinRst, HIGH);
    delay(50);  // Increased from 10ms - give chip time to boot

    // Initialize SPI for diagnostic - try slower speed first
    spi = new SPIClass(VSPI);
    spi->begin(config.pinSck, config.pinMiso, config.pinMosi);

    // Test at multiple SPI frequencies
    uint32_t spiFreqs[] = {100000, 500000, 1000000};  // 100kHz, 500kHz, 1MHz
    uint8_t version = 0;

    for (int i = 0; i < 3; i++) {
        spi->beginTransaction(SPISettings(spiFreqs[i], MSBFIRST, SPI_MODE0));

        // Try to read version register (0x42) - should return 0x12 for SX1276
        digitalWrite(config.pinNss, LOW);
        delayMicroseconds(100);  // Longer delay for stability
        spi->transfer(0x42);  // Read register 0x42 (version)
        version = spi->transfer(0x00);
        digitalWrite(config.pinNss, HIGH);
        spi->endTransaction();

        Serial.printf("[LoRa] SPI @ %d Hz - Version (0x42): 0x%02X\n", spiFreqs[i], version);

        if (version == 0x12 || version == 0x22) {
            break;  // Found valid chip
        }
        delay(10);
    }

    Serial.printf("[LoRa] Expected: 0x12 for SX1276/RFM95W, 0x22 for SX1272\n");

    // Read additional registers for diagnostics
    spi->beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));

    // Read RegOpMode (0x01)
    digitalWrite(config.pinNss, LOW);
    delayMicroseconds(100);
    spi->transfer(0x01);
    uint8_t opMode = spi->transfer(0x00);
    digitalWrite(config.pinNss, HIGH);

    // Read RegFrfMsb (0x06) - frequency MSB
    digitalWrite(config.pinNss, LOW);
    delayMicroseconds(100);
    spi->transfer(0x06);
    uint8_t frfMsb = spi->transfer(0x00);
    digitalWrite(config.pinNss, HIGH);

    spi->endTransaction();

    Serial.printf("[LoRa] RegOpMode (0x01): 0x%02X\n", opMode);
    Serial.printf("[LoRa] RegFrfMsb (0x06): 0x%02X\n", frfMsb);

    if (version == 0x00 || version == 0xFF) {
        Serial.println("[LoRa] ==========================================");
        Serial.println("[LoRa] WARNING: SPI communication FAILED!");
        Serial.println("[LoRa] ==========================================");
        Serial.println("[LoRa] Possible causes:");
        Serial.println("[LoRa]   1. Module not receiving 3.3V power");
        Serial.println("[LoRa]   2. SPI wiring incorrect (MISO/MOSI swapped?)");
        Serial.println("[LoRa]   3. NSS/CS pin wrong or not connected");
        Serial.println("[LoRa]   4. RESET pin not connected");
        Serial.println("[LoRa]   5. Cold solder joints");
        Serial.println("[LoRa]   6. SD Card interfering (remove if inserted)");
        Serial.println("[LoRa] ==========================================");
    } else if (version == 0x12) {
        Serial.println("[LoRa] SPI OK - SX1276/RFM95W detected!");
    } else if (version == 0x22) {
        Serial.println("[LoRa] SPI OK - SX1272 detected");
    } else {
        Serial.printf("[LoRa] Unexpected chip version: 0x%02X\n", version);
    }
    // === END DIAGNOSTIC ===

    // Create radio module (DIO1 not used, set to RADIOLIB_NC)
    Module* mod = new Module(config.pinNss, config.pinDio0, config.pinRst, RADIOLIB_NC, *spi);
    radio = new SX1276(mod);

    // Initialize radio
    Serial.printf("[LoRa] Initializing SX1276 at %.2f MHz, SF%d, BW%.0f kHz...\n",
                  config.frequency / 1000000.0, config.spreadingFactor, config.bandwidth);

    int state = radio->begin(
        config.frequency / 1000000.0,  // frequency in MHz
        config.bandwidth,               // bandwidth in kHz
        config.spreadingFactor,         // spreading factor
        config.codingRate,              // coding rate
        config.syncWord,                // sync word
        config.txPower,                 // output power
        8,                              // preamble length
        0                               // gain (0 = auto AGC)
    );

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Init failed, code: %d\n", state);
        available = false;
        return false;
    }

    // Set up interrupt on DIO0
    radio->setDio0Action(onDio0Rise, RISING);

    // Enable CRC checking
    radio->setCRC(true);

    available = true;
    Serial.println("[LoRa] Radio initialized successfully");

    return true;
}

bool LoRaGateway::applyConfig() {
    if (!available || !radio) return false;

    // Stop receiving before reconfiguration
    radio->standby();
    receiving = false;

    int state = radio->setFrequency(config.frequency / 1000000.0);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setFrequency failed: %d\n", state);
        return false;
    }

    state = radio->setBandwidth(config.bandwidth);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setBandwidth failed: %d\n", state);
        return false;
    }

    state = radio->setSpreadingFactor(config.spreadingFactor);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setSpreadingFactor failed: %d\n", state);
        return false;
    }

    state = radio->setCodingRate(config.codingRate);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setCodingRate failed: %d\n", state);
        return false;
    }

    state = radio->setSyncWord(config.syncWord);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setSyncWord failed: %d\n", state);
        return false;
    }

    state = radio->setOutputPower(config.txPower);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] setOutputPower failed: %d\n", state);
        return false;
    }

    Serial.printf("[LoRa] Config applied: %.2f MHz, SF%d, BW%.0f kHz, CR4/%d, %d dBm\n",
                  config.frequency / 1000000.0, config.spreadingFactor,
                  config.bandwidth, config.codingRate, config.txPower);

    return true;
}

void LoRaGateway::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("lora")) {
        Serial.println("[LoRa] No LoRa config in JSON, using defaults");
        return;
    }

    JsonObjectConst lora = doc["lora"];

    config.enabled = lora["enabled"] | true;
    config.frequency = lora["frequency"] | LORA_FREQUENCY_DEFAULT;
    config.spreadingFactor = lora["spreading_factor"] | LORA_SF_DEFAULT;
    config.bandwidth = lora["bandwidth"] | LORA_BW_DEFAULT;
    config.codingRate = lora["coding_rate"] | LORA_CR_DEFAULT;
    config.txPower = lora["tx_power"] | LORA_POWER_DEFAULT;
    config.syncWord = lora["sync_word"] | LORA_SYNC_WORD_DEFAULT;

    // Pin configuration (optional)
    if (lora.containsKey("pins")) {
        JsonObjectConst pins = lora["pins"];
        config.pinMiso = pins["miso"] | LORA_MISO;
        config.pinMosi = pins["mosi"] | LORA_MOSI;
        config.pinSck = pins["sck"] | LORA_SCK;
        config.pinNss = pins["nss"] | LORA_NSS;
        config.pinRst = pins["rst"] | LORA_RST;
        config.pinDio0 = pins["dio0"] | LORA_DIO0;
    }

    Serial.printf("[LoRa] Config loaded: enabled=%d, freq=%.2f MHz, SF%d\n",
                  config.enabled, config.frequency / 1000000.0, config.spreadingFactor);
}

bool LoRaGateway::saveConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[LoRa] Cannot open config for reading");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("[LoRa] Failed to parse config");
        return false;
    }

    // Preserve existing pin configuration before updating
    JsonObject existingPins;
    bool hasPins = false;
    if (doc.containsKey("lora") && doc["lora"].containsKey("pins")) {
        hasPins = true;
        existingPins = doc["lora"]["pins"];
    }

    // Update LoRa section (only radio parameters, not pins)
    JsonObject lora = doc.createNestedObject("lora");
    lora["enabled"] = config.enabled;
    lora["frequency"] = config.frequency;
    lora["spreading_factor"] = config.spreadingFactor;
    lora["bandwidth"] = config.bandwidth;
    lora["coding_rate"] = config.codingRate;
    lora["tx_power"] = config.txPower;
    lora["sync_word"] = config.syncWord;

    // Restore pin configuration from file (not from memory defaults)
    JsonObject pins = lora.createNestedObject("pins");
    if (hasPins) {
        // Preserve existing pins from config file
        pins["miso"] = existingPins["miso"] | LORA_MISO;
        pins["mosi"] = existingPins["mosi"] | LORA_MOSI;
        pins["sck"] = existingPins["sck"] | LORA_SCK;
        pins["nss"] = existingPins["nss"] | LORA_NSS;
        pins["rst"] = existingPins["rst"] | LORA_RST;
        pins["dio0"] = existingPins["dio0"] | LORA_DIO0;
    } else {
        // No existing pins, use current config values
        pins["miso"] = config.pinMiso;
        pins["mosi"] = config.pinMosi;
        pins["sck"] = config.pinSck;
        pins["nss"] = config.pinNss;
        pins["rst"] = config.pinRst;
        pins["dio0"] = config.pinDio0;
    }

    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[LoRa] Cannot open config for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    return true;
}

bool LoRaGateway::startReceive() {
    if (!available || !config.enabled) return false;

    int state = radio->startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] startReceive failed: %d\n", state);
        return false;
    }

    receiving = true;
    dio0Flag = false;
    return true;
}

void LoRaGateway::update() {
    if (!available || !config.enabled || !receiving) return;

    // Check if a packet was received (via interrupt)
    if (dio0Flag) {
        dio0Flag = false;
        processReceivedPacket();
    }
}

void LoRaGateway::processReceivedPacket() {
    // Create packet structure
    LoRaPacket packet;
    memset(&packet, 0, sizeof(packet));

    // Read the data
    int state = radio->readData(packet.data, MAX_PACKET_SIZE);
    packet.length = radio->getPacketLength();

    if (state == RADIOLIB_ERR_NONE) {
        // Packet received successfully
        packet.rssi = radio->getRSSI();
        packet.snr = radio->getSNR();
        packet.frequency = config.frequency;
        packet.spreadingFactor = config.spreadingFactor;
        packet.bandwidth = config.bandwidth;
        packet.codingRate = config.codingRate;
        packet.timestamp = micros();
        packet.valid = true;

        // Update statistics
        stats.rxPacketsReceived++;
        stats.lastPacketTime = millis();
        stats.lastRssi = packet.rssi;
        stats.lastSnr = packet.snr;

        Serial.printf("[LoRa] RX: %d bytes, RSSI: %.1f dBm, SNR: %.1f dB\n",
                      packet.length, packet.rssi, packet.snr);

        // Queue the packet
        if (queuePacket(packet)) {
            Serial.println("[LoRa] Packet queued for forwarding");
        } else {
            Serial.println("[LoRa] Queue full, packet dropped!");
        }

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        stats.rxPacketsCrcError++;
        Serial.println("[LoRa] CRC error");
    } else {
        Serial.printf("[LoRa] Receive error: %d\n", state);
    }

    // Restart receiving
    startReceive();
}

bool LoRaGateway::queuePacket(const LoRaPacket& packet) {
    uint8_t nextHead = (queueHead + 1) % MAX_PACKET_QUEUE;
    if (nextHead == queueTail) {
        // Queue is full
        return false;
    }

    packetQueue[queueHead] = packet;
    queueHead = nextHead;
    return true;
}

bool LoRaGateway::hasPacket() {
    return queueHead != queueTail;
}

LoRaPacket LoRaGateway::getPacket() {
    LoRaPacket packet;
    memset(&packet, 0, sizeof(packet));
    packet.valid = false;

    if (queueHead == queueTail) {
        return packet;  // Queue empty
    }

    packet = packetQueue[queueTail];
    queueTail = (queueTail + 1) % MAX_PACKET_QUEUE;
    return packet;
}

bool LoRaGateway::transmit(const uint8_t* data, size_t length, uint32_t frequency,
                           uint8_t sf, float bw, uint8_t cr) {
    if (!available || !config.enabled) return false;

    // Stop receiving
    radio->standby();
    receiving = false;

    // Apply temporary settings if specified
    bool tempSettings = (frequency != 0 || sf != 0 || bw != 0 || cr != 0);
    if (tempSettings) {
        if (frequency != 0) radio->setFrequency(frequency / 1000000.0);
        if (sf != 0) radio->setSpreadingFactor(sf);
        if (bw != 0) radio->setBandwidth(bw);
        if (cr != 0) radio->setCodingRate(cr);
    }

    Serial.printf("[LoRa] TX: %d bytes\n", length);

    // Transmit
    int state = radio->transmit(const_cast<uint8_t*>(data), length);

    // Restore original settings if temporary were used
    if (tempSettings) {
        applyConfig();
    }

    if (state == RADIOLIB_ERR_NONE) {
        stats.txPacketsSent++;
        Serial.println("[LoRa] TX success");

        // Restart receiving
        startReceive();
        return true;
    } else {
        stats.txPacketsFailed++;
        Serial.printf("[LoRa] TX failed: %d\n", state);

        // Restart receiving anyway
        startReceive();
        return false;
    }
}

String LoRaGateway::getStatusJson() {
    DynamicJsonDocument doc(1024);

    doc["available"] = available;
    doc["enabled"] = config.enabled;
    doc["receiving"] = receiving;

    JsonObject cfg = doc.createNestedObject("config");
    cfg["frequency"] = config.frequency;
    cfg["spreading_factor"] = config.spreadingFactor;
    cfg["bandwidth"] = config.bandwidth;
    cfg["coding_rate"] = config.codingRate;
    cfg["tx_power"] = config.txPower;

    JsonObject st = doc.createNestedObject("stats");
    st["rx_received"] = stats.rxPacketsReceived;
    st["rx_forwarded"] = stats.rxPacketsForwarded;
    st["rx_crc_error"] = stats.rxPacketsCrcError;
    st["tx_sent"] = stats.txPacketsSent;
    st["tx_acked"] = stats.txPacketsAcked;
    st["tx_failed"] = stats.txPacketsFailed;
    st["last_rssi"] = stats.lastRssi;
    st["last_snr"] = stats.lastSnr;

    if (stats.lastPacketTime > 0) {
        unsigned long ago = (millis() - stats.lastPacketTime) / 1000;
        st["last_packet_ago"] = ago;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

void LoRaGateway::resetStats() {
    memset(&stats, 0, sizeof(stats));
}
