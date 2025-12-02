#include "udp_forwarder.h"
#include <WiFi.h>  // Para WiFi.macAddress() no fallback de EUI
#include <LittleFS.h>
#include <time.h>
#include "ntp_manager.h"

// Global instance
UDPForwarder udpForwarder;

// Base64 encoding table
const char UDPForwarder::base64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

UDPForwarder::UDPForwarder()
    : connected(false)
    , tokenCounter(0)
    , lastStatTime(0)
    , lastPullTime(0) {

    memset(&stats, 0, sizeof(stats));
    setDefaultConfig();
}

void UDPForwarder::setDefaultConfig() {
    config.enabled = true;
    strlcpy(config.serverHost, NS_HOST_DEFAULT, sizeof(config.serverHost));
    config.serverPortUp = NS_PORT_UP_DEFAULT;
    config.serverPortDown = NS_PORT_DOWN_DEFAULT;
    memset(config.gatewayEui, 0, sizeof(config.gatewayEui));
    strlcpy(config.description, "ESP32 1ch Gateway", sizeof(config.description));
    strlcpy(config.region, REGION_DEFAULT, sizeof(config.region));
    config.latitude = 0.0;
    config.longitude = 0.0;
    config.altitude = 0;
}

bool UDPForwarder::begin() {
    Serial.println("[UDP] Initializing forwarder...");

    // Generate Gateway EUI from WiFi MAC if not configured
    bool euiZero = true;
    for (int i = 0; i < 8; i++) {
        if (config.gatewayEui[i] != 0) {
            euiZero = false;
            break;
        }
    }

    if (euiZero) {
        generateGatewayEui();
    }

    Serial.printf("[UDP] Gateway EUI: %s\n", getGatewayEuiString().c_str());
    Serial.printf("[UDP] Server: %s:%d (up) / %d (down)\n",
                  config.serverHost, config.serverPortUp, config.serverPortDown);

    // Verificar se NetworkManager esta disponivel
    if (!networkManager) {
        Serial.println("[UDP] ERROR: NetworkManager not available");
        return false;
    }

    if (!networkManager->isConnected()) {
        Serial.println("[UDP] WARNING: Network not connected yet");
    }

    if (!networkManager->udpBegin(config.serverPortDown)) {
        Serial.println("[UDP] Failed to start UDP via NetworkManager");
        return false;
    }

    Serial.println("[UDP] Using NetworkManager for UDP");

    connected = true;
    Serial.println("[UDP] Forwarder initialized");

    // Send initial PULL_DATA
    sendPullData();

    return true;
}

void UDPForwarder::generateGatewayEui() {
    // Get MAC address from NetworkManager (active interface)
    uint8_t mac[6];
    if (networkManager && networkManager->getActiveInterface()) {
        networkManager->getActiveInterface()->macAddress(mac);
    } else {
        // Fallback: use WiFi MAC directly (before NetworkManager is ready)
        WiFi.macAddress(mac);
    }

    // Create EUI-64 from MAC-48 by inserting FFFE in the middle
    // MAC: AA:BB:CC:DD:EE:FF -> EUI: AA:BB:CC:FF:FE:DD:EE:FF
    config.gatewayEui[0] = mac[0];
    config.gatewayEui[1] = mac[1];
    config.gatewayEui[2] = mac[2];
    config.gatewayEui[3] = 0xFF;
    config.gatewayEui[4] = 0xFE;
    config.gatewayEui[5] = mac[3];
    config.gatewayEui[6] = mac[4];
    config.gatewayEui[7] = mac[5];

    Serial.printf("[UDP] Generated EUI from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

String UDPForwarder::getGatewayEuiString() {
    char buf[24];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X",
             config.gatewayEui[0], config.gatewayEui[1],
             config.gatewayEui[2], config.gatewayEui[3],
             config.gatewayEui[4], config.gatewayEui[5],
             config.gatewayEui[6], config.gatewayEui[7]);
    return String(buf);
}

void UDPForwarder::loadConfig(const JsonDocument& doc) {
    if (!doc.containsKey("server")) {
        Serial.println("[UDP] No server config in JSON, using defaults");
        return;
    }

    JsonObjectConst server = doc["server"];

    config.enabled = server["enabled"] | true;
    strlcpy(config.serverHost, server["host"] | NS_HOST_DEFAULT, sizeof(config.serverHost));
    config.serverPortUp = server["port_up"] | NS_PORT_UP_DEFAULT;
    config.serverPortDown = server["port_down"] | NS_PORT_DOWN_DEFAULT;

    // Gateway EUI (optional - hex string)
    if (server.containsKey("gateway_eui")) {
        const char* euiStr = server["gateway_eui"];
        if (strlen(euiStr) == 16) {
            for (int i = 0; i < 8; i++) {
                char hex[3] = {euiStr[i*2], euiStr[i*2+1], '\0'};
                config.gatewayEui[i] = strtol(hex, nullptr, 16);
            }
        }
    }

    // Gateway info
    strlcpy(config.description, server["description"] | "ESP32 1ch Gateway",
            sizeof(config.description));
    strlcpy(config.region, server["region"] | REGION_DEFAULT,
            sizeof(config.region));
    config.latitude = server["latitude"] | 0.0;
    config.longitude = server["longitude"] | 0.0;
    config.altitude = server["altitude"] | 0;

    Serial.printf("[UDP] Config loaded: %s:%d (region: %s)\n",
                  config.serverHost, config.serverPortUp, config.region);
}

bool UDPForwarder::saveConfig() {
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[UDP] Cannot open config for reading");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("[UDP] Failed to parse config");
        return false;
    }

    // Update server section
    JsonObject server = doc.createNestedObject("server");
    server["enabled"] = config.enabled;
    server["host"] = config.serverHost;
    server["port_up"] = config.serverPortUp;
    server["port_down"] = config.serverPortDown;
    server["gateway_eui"] = getGatewayEuiString();
    server["description"] = config.description;
    server["region"] = config.region;
    server["latitude"] = config.latitude;
    server["longitude"] = config.longitude;
    server["altitude"] = config.altitude;

    file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[UDP] Cannot open config for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();

    return true;
}

void UDPForwarder::update() {
    if (!connected || !config.enabled) return;

    unsigned long now = millis();

    // Send PULL_DATA periodically
    if (now - lastPullTime >= PULL_INTERVAL) {
        sendPullData();
        lastPullTime = now;
    }

    // Send statistics periodically
    if (now - lastStatTime >= STAT_INTERVAL) {
        sendStatistics();
        lastStatTime = now;
    }

    // Check for incoming packets (PULL_ACK, PULL_RESP)
    receivePackets();
}

bool UDPForwarder::forwardPacket(const LoRaPacket& packet) {
    if (!connected || !config.enabled || !packet.valid) return false;

    String jsonData = buildRxpkJson(packet);

    Serial.printf("[UDP] Forwarding packet (%d bytes payload)\n", packet.length);

    if (sendPushData(jsonData.c_str(), jsonData.length())) {
        stats.pushDataSent++;
        stats.lastPushTime = millis();
        return true;
    }

    return false;
}

String UDPForwarder::buildRxpkJson(const LoRaPacket& packet) {
    DynamicJsonDocument doc(1024);
    JsonObject rxpk = doc.createNestedArray("rxpk").createNestedObject();

    // Timestamp (internal counter, microseconds)
    rxpk["tmst"] = packet.timestamp;

    // Time (ISO 8601 format)
    rxpk["time"] = getIsoTimestamp();

    // Channel (always 0 for single channel gateway)
    rxpk["chan"] = 0;

    // RF chain (always 0)
    rxpk["rfch"] = 0;

    // Frequency in MHz
    rxpk["freq"] = packet.frequency / 1000000.0;

    // Signal status (1 = CRC OK)
    rxpk["stat"] = 1;

    // Modulation (always LORA for this gateway)
    rxpk["modu"] = "LORA";

    // Data rate identifier
    char datr[16];
    snprintf(datr, sizeof(datr), "SF%dBW%d",
             packet.spreadingFactor, (int)packet.bandwidth);
    rxpk["datr"] = datr;

    // Coding rate
    char codr[8];
    snprintf(codr, sizeof(codr), "4/%d", packet.codingRate);
    rxpk["codr"] = codr;

    // RSSI
    rxpk["rssi"] = (int)packet.rssi;

    // SNR
    rxpk["lsnr"] = packet.snr;

    // Payload size
    rxpk["size"] = packet.length;

    // Payload (Base64 encoded)
    rxpk["data"] = base64Encode(packet.data, packet.length);

    String output;
    serializeJson(doc, output);
    return output;
}

bool UDPForwarder::sendPushData(const char* jsonData, size_t length) {
    // Build PUSH_DATA packet
    // [0]: Protocol version
    // [1-2]: Random token
    // [3]: Packet type (PUSH_DATA = 0x00)
    // [4-11]: Gateway EUI
    // [12+]: JSON data

    uint16_t token = getNextToken();

    size_t packetLen = 12 + length;
    if (packetLen > UDP_BUFFER_SIZE) {
        Serial.println("[UDP] Packet too large");
        return false;
    }

    udpBuffer[0] = PROTOCOL_VERSION;
    udpBuffer[1] = (token >> 8) & 0xFF;
    udpBuffer[2] = token & 0xFF;
    udpBuffer[3] = PKT_PUSH_DATA;
    memcpy(&udpBuffer[4], config.gatewayEui, 8);
    memcpy(&udpBuffer[12], jsonData, length);

    // Send to server via NetworkManager
    if (!networkManager->udpBeginPacket(config.serverHost, config.serverPortUp)) {
        Serial.println("[UDP] Failed to begin PUSH_DATA packet");
        return false;
    }
    networkManager->udpWrite(udpBuffer, packetLen);
    if (!networkManager->udpEndPacket()) {
        Serial.println("[UDP] Failed to send PUSH_DATA");
        return false;
    }

    Serial.printf("[UDP] PUSH_DATA sent (token=%04X, %d bytes)\n", token, packetLen);
    return true;
}

bool UDPForwarder::sendPullData() {
    // Build PULL_DATA packet
    // [0]: Protocol version
    // [1-2]: Random token
    // [3]: Packet type (PULL_DATA = 0x02)
    // [4-11]: Gateway EUI

    uint16_t token = getNextToken();

    udpBuffer[0] = PROTOCOL_VERSION;
    udpBuffer[1] = (token >> 8) & 0xFF;
    udpBuffer[2] = token & 0xFF;
    udpBuffer[3] = PKT_PULL_DATA;
    memcpy(&udpBuffer[4], config.gatewayEui, 8);

    // Send via NetworkManager
    if (!networkManager->udpBeginPacket(config.serverHost, config.serverPortUp)) {
        Serial.println("[UDP] Failed to begin PULL_DATA packet");
        return false;
    }
    networkManager->udpWrite(udpBuffer, 12);
    if (!networkManager->udpEndPacket()) {
        Serial.println("[UDP] Failed to send PULL_DATA");
        return false;
    }

    stats.pullDataSent++;
    Serial.printf("[UDP] PULL_DATA sent (token=%04X)\n", token);
    return true;
}

void UDPForwarder::sendStatistics() {
    String jsonData = buildStatJson();
    sendPushData(jsonData.c_str(), jsonData.length());
}

String UDPForwarder::buildStatJson() {
    DynamicJsonDocument doc(512);
    JsonObject stat = doc.createNestedObject("stat");

    // Time
    stat["time"] = getIsoTimestamp();

    // GPS coordinates (if configured)
    if (config.latitude != 0.0 || config.longitude != 0.0) {
        stat["lati"] = config.latitude;
        stat["long"] = config.longitude;
        stat["alti"] = config.altitude;
    }

    // Packets received/transmitted
    stat["rxnb"] = stats.pushDataSent;
    stat["rxok"] = stats.pushDataSent;
    stat["rxfw"] = stats.pushDataSent;
    stat["ackr"] = stats.pushAckReceived > 0 ?
                   (float)stats.pushAckReceived / stats.pushDataSent * 100.0 : 0;
    stat["dwnb"] = stats.downlinksReceived;
    stat["txnb"] = stats.downlinksSent;

    // Gateway description
    stat["desc"] = config.description;

    String output;
    serializeJson(doc, output);
    return output;
}

void UDPForwarder::receivePackets() {
    int packetSize = networkManager->udpParsePacket();
    if (packetSize <= 0) return;

    if (packetSize > UDP_BUFFER_SIZE) {
        Serial.println("[UDP] Received packet too large");
        return;
    }

    int len = networkManager->udpRead(udpBuffer, UDP_BUFFER_SIZE);
    if (len < 4) return;

    // Parse header
    uint8_t version = udpBuffer[0];
    uint16_t token = (udpBuffer[1] << 8) | udpBuffer[2];
    uint8_t type = udpBuffer[3];

    if (version != PROTOCOL_VERSION) {
        Serial.printf("[UDP] Unknown protocol version: %d\n", version);
        return;
    }

    switch (type) {
        case PKT_PUSH_ACK:
            stats.pushAckReceived++;
            stats.lastAckTime = millis();
            Serial.printf("[UDP] PUSH_ACK received (token=%04X)\n", token);
            break;

        case PKT_PULL_ACK:
            stats.pullAckReceived++;
            stats.lastAckTime = millis();
            Serial.printf("[UDP] PULL_ACK received (token=%04X)\n", token);
            break;

        case PKT_PULL_RESP:
            stats.pullRespReceived++;
            Serial.printf("[UDP] PULL_RESP received (token=%04X, %d bytes)\n", token, len);
            handlePullResp(udpBuffer + 4, len - 4, token);
            break;

        default:
            Serial.printf("[UDP] Unknown packet type: %02X\n", type);
            break;
    }
}

void UDPForwarder::handlePullResp(const uint8_t* data, size_t length, uint16_t token) {
    // Parse JSON payload
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, length);

    if (error) {
        Serial.printf("[UDP] Failed to parse PULL_RESP JSON: %s\n", error.c_str());
        sendTxAck(token, "JSON_ERROR");
        return;
    }

    if (!doc.containsKey("txpk")) {
        Serial.println("[UDP] PULL_RESP missing txpk");
        sendTxAck(token, "TX_PARAM_ERROR");
        return;
    }

    stats.downlinksReceived++;
    processTxPacket(doc);

    // Send TX_ACK
    sendTxAck(token);
}

void UDPForwarder::processTxPacket(const JsonDocument& doc) {
    JsonObjectConst txpk = doc["txpk"];

    // Extract TX parameters
    uint32_t tmst = txpk["tmst"] | 0;
    float freq = txpk["freq"] | 0.0;
    int8_t powe = txpk["powe"] | 14;
    const char* modu = txpk["modu"] | "LORA";
    const char* datr = txpk["datr"] | "SF7BW125";
    const char* codr = txpk["codr"] | "4/5";
    bool ipol = txpk["ipol"] | true;
    const char* data = txpk["data"] | "";

    // Parse data rate (e.g., "SF7BW125")
    uint8_t sf = 7;
    float bw = 125.0;
    if (strlen(datr) >= 7) {
        sscanf(datr, "SF%hhuBW%f", &sf, &bw);
    }

    // Parse coding rate (e.g., "4/5")
    uint8_t cr = 5;
    if (strlen(codr) >= 3) {
        cr = codr[2] - '0';
    }

    // Decode Base64 payload
    uint8_t payload[MAX_PACKET_SIZE];
    size_t payloadLen = base64Decode(data, payload, MAX_PACKET_SIZE);

    if (payloadLen == 0) {
        Serial.println("[UDP] Failed to decode TX payload");
        return;
    }

    Serial.printf("[UDP] TX: freq=%.2f MHz, SF%d, BW%.0f, %d bytes\n",
                  freq, sf, bw, payloadLen);

    // Schedule transmission
    // Note: Single channel gateway cannot do proper timing, send immediately
    // TODO: Implement timing for Class A devices

    if (loraGateway.transmit(payload, payloadLen,
                             (uint32_t)(freq * 1000000),
                             sf, bw, cr)) {
        stats.downlinksSent++;
        Serial.println("[UDP] Downlink transmitted");
    } else {
        Serial.println("[UDP] Downlink transmission failed");
    }
}

bool UDPForwarder::sendTxAck(uint16_t token, const char* error) {
    // Build TX_ACK packet
    // [0]: Protocol version
    // [1-2]: Token (same as PULL_RESP)
    // [3]: Packet type (TX_ACK = 0x05)
    // [4-11]: Gateway EUI
    // [12+]: JSON with error (optional)

    size_t packetLen = 12;

    udpBuffer[0] = PROTOCOL_VERSION;
    udpBuffer[1] = (token >> 8) & 0xFF;
    udpBuffer[2] = token & 0xFF;
    udpBuffer[3] = PKT_TX_ACK;
    memcpy(&udpBuffer[4], config.gatewayEui, 8);

    // Add error JSON if present
    if (error != nullptr) {
        DynamicJsonDocument doc(128);
        doc["txpk_ack"]["error"] = error;
        String json;
        serializeJson(doc, json);
        size_t jsonLen = json.length();
        if (packetLen + jsonLen < UDP_BUFFER_SIZE) {
            memcpy(&udpBuffer[12], json.c_str(), jsonLen);
            packetLen += jsonLen;
        }
    }

    // Send via NetworkManager
    if (!networkManager->udpBeginPacket(config.serverHost, config.serverPortUp)) {
        Serial.println("[UDP] Failed to begin TX_ACK packet");
        return false;
    }
    networkManager->udpWrite(udpBuffer, packetLen);
    if (!networkManager->udpEndPacket()) {
        Serial.println("[UDP] Failed to send TX_ACK");
        return false;
    }

    stats.txAckSent++;
    Serial.printf("[UDP] TX_ACK sent (token=%04X)\n", token);
    return true;
}

bool UDPForwarder::isHealthy(uint32_t timeout) const {
    // If no ACK ever received, consider unhealthy
    if (stats.lastAckTime == 0) {
        return false;
    }

    // Check if lastAckTime is within timeout window
    uint32_t now = millis();
    return (now - stats.lastAckTime) < timeout;
}

uint16_t UDPForwarder::getNextToken() {
    tokenCounter++;
    if (tokenCounter == 0) tokenCounter = 1;  // Avoid zero token
    return tokenCounter;
}

String UDPForwarder::getIsoTimestamp() {
    time_t now;
    struct tm timeinfo;
    char buf[32];

    time(&now);
    gmtime_r(&now, &timeinfo);

    // Formato Semtech esperado pelo ChirpStack Gateway Bridge
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S GMT", &timeinfo);
    return String(buf);
}

uint32_t UDPForwarder::getCompactTimestamp() {
    return micros();
}

String UDPForwarder::base64Encode(const uint8_t* data, size_t length) {
    String result;
    result.reserve((length + 2) / 3 * 4);

    size_t i = 0;
    while (i < length) {
        uint32_t octet_a = i < length ? data[i++] : 0;
        uint32_t octet_b = i < length ? data[i++] : 0;
        uint32_t octet_c = i < length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        result += base64Chars[(triple >> 18) & 0x3F];
        result += base64Chars[(triple >> 12) & 0x3F];
        result += (i > length + 1) ? '=' : base64Chars[(triple >> 6) & 0x3F];
        result += (i > length) ? '=' : base64Chars[triple & 0x3F];
    }

    return result;
}

size_t UDPForwarder::base64Decode(const char* encoded, uint8_t* output, size_t maxLen) {
    size_t len = strlen(encoded);
    if (len == 0) return 0;

    // Calculate output length
    size_t padding = 0;
    if (encoded[len - 1] == '=') padding++;
    if (encoded[len - 2] == '=') padding++;

    size_t outputLen = (len / 4) * 3 - padding;
    if (outputLen > maxLen) return 0;

    // Build decode table
    static int8_t decodeTable[256] = {-1};
    static bool tableInit = false;
    if (!tableInit) {
        memset(decodeTable, -1, sizeof(decodeTable));
        for (int i = 0; i < 64; i++) {
            decodeTable[(uint8_t)base64Chars[i]] = i;
        }
        tableInit = true;
    }

    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        uint32_t sextet_a = encoded[i] == '=' ? 0 : decodeTable[(uint8_t)encoded[i]];
        uint32_t sextet_b = encoded[i + 1] == '=' ? 0 : decodeTable[(uint8_t)encoded[i + 1]];
        uint32_t sextet_c = encoded[i + 2] == '=' ? 0 : decodeTable[(uint8_t)encoded[i + 2]];
        uint32_t sextet_d = encoded[i + 3] == '=' ? 0 : decodeTable[(uint8_t)encoded[i + 3]];

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < outputLen) output[j++] = (triple >> 16) & 0xFF;
        if (j < outputLen) output[j++] = (triple >> 8) & 0xFF;
        if (j < outputLen) output[j++] = triple & 0xFF;
    }

    return outputLen;
}

String UDPForwarder::getStatusJson() {
    DynamicJsonDocument doc(1024);

    doc["connected"] = connected;
    doc["enabled"] = config.enabled;

    // Adicionar info da interface de rede via NetworkManager
    if (networkManager && networkManager->getActiveInterface()) {
        doc["network_interface"] = networkManager->getActiveInterface()->getName();
        doc["network_ip"] = networkManager->getActiveInterface()->localIP().toString();
    } else {
        doc["network_interface"] = "none";
        doc["network_ip"] = "";
    }

    JsonObject cfg = doc.createNestedObject("config");
    cfg["server"] = config.serverHost;
    cfg["port_up"] = config.serverPortUp;
    cfg["port_down"] = config.serverPortDown;
    cfg["gateway_eui"] = getGatewayEuiString();
    cfg["description"] = config.description;
    cfg["region"] = config.region;
    cfg["latitude"] = config.latitude;
    cfg["longitude"] = config.longitude;
    cfg["altitude"] = config.altitude;

    JsonObject st = doc.createNestedObject("stats");
    st["push_data_sent"] = stats.pushDataSent;
    st["push_ack_received"] = stats.pushAckReceived;
    st["pull_data_sent"] = stats.pullDataSent;
    st["pull_ack_received"] = stats.pullAckReceived;
    st["pull_resp_received"] = stats.pullRespReceived;
    st["tx_ack_sent"] = stats.txAckSent;
    st["downlinks_received"] = stats.downlinksReceived;
    st["downlinks_sent"] = stats.downlinksSent;

    if (stats.lastAckTime > 0) {
        unsigned long ago = (millis() - stats.lastAckTime) / 1000;
        st["last_ack_ago"] = ago;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

void UDPForwarder::resetStats() {
    memset(&stats, 0, sizeof(stats));
}
