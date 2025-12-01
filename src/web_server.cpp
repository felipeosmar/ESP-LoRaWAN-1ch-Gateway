#include "web_server.h"
#include <LittleFS.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <vector>
#include "config.h"
#include "lora_gateway.h"
#include "udp_forwarder.h"
#include "ntp_manager.h"
#include "lcd_manager.h"
#include "buzzer_manager.h"
#include "gps_manager.h"
#include "rtc_manager.h"
#include "network_manager.h"

// Global instance
WebServerManager webServer;

// WiFi network structure (must match main.cpp)
struct WiFiNetwork {
    String ssid;
    String password;
};

// External references
extern bool wifiConnectedToInternet;
extern String wifiHostname;
extern String wifiSSID;
extern String wifiPassword;
extern bool wifiAPMode;
extern std::vector<WiFiNetwork> wifiNetworks;

WebServerManager::WebServerManager()
    : server(80)
    , ws("/ws")
    , otaUploadInProgress(false) {
}

void WebServerManager::begin() {
    Serial.println("[Web] Starting web server...");

    // Setup WebSocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);

    // Setup routes
    setupRoutes();

    // Start server
    server.begin();
    Serial.println("[Web] Server started on port 80");
}

void WebServerManager::setupRoutes() {
    // Serve static files from LittleFS
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    // Static assets (compressed)
    server.on("/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveCompressedFile(request, "/web/app.js.gz", "application/javascript");
    });

    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveCompressedFile(request, "/web/style.css.gz", "text/css");
    });

    // API Routes
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });

    server.on("/api/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStats(request);
    });

    server.on("/api/stats/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleResetStats(request);
    });

    // LoRa configuration
    server.on("/api/lora/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLoRaConfig(request);
    });

    server.on("/api/lora/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleLoRaConfigPost(request, data, len, index, total);
        }
    );

    // Server configuration
    server.on("/api/server/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleServerConfig(request);
    });

    server.on("/api/server/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleServerConfigPost(request, data, len, index, total);
        }
    );

    // WiFi configuration
    server.on("/api/wifi/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleWiFiConfig(request);
    });

    server.on("/api/wifi/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleWiFiConfigPost(request, data, len, index, total);
        }
    );

    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleWiFiScan(request);
    });

    // NTP configuration
    server.on("/api/ntp/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleNTPConfig(request);
    });

    server.on("/api/ntp/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleNTPConfigPost(request, data, len, index, total);
        }
    );

    server.on("/api/ntp/sync", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleNTPSync(request);
    });

    // LCD configuration
    server.on("/api/lcd/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLCDConfig(request);
    });

    server.on("/api/lcd/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleLCDConfigPost(request, data, len, index, total);
        }
    );

    // Buzzer configuration
    server.on("/api/buzzer/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleBuzzerConfig(request);
    });

    server.on("/api/buzzer/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleBuzzerConfigPost(request, data, len, index, total);
        }
    );

    server.on("/api/buzzer/test", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleBuzzerTest(request, data, len, index, total);
        }
    );

    // GPS configuration
    server.on("/api/gps/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGPSConfig(request);
    });

    server.on("/api/gps/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleGPSConfigPost(request, data, len, index, total);
        }
    );

    // RTC configuration
    server.on("/api/rtc/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRTCConfig(request);
    });

    server.on("/api/rtc/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleRTCConfigPost(request, data, len, index, total);
        }
    );

    server.on("/api/rtc/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRTCStatus(request);
    });

    server.on("/api/rtc/sync", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleRTCSync(request);
    });

    server.on("/api/rtc/settime", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleRTCSetTime(request, data, len, index, total);
        }
    );

    // Network Manager API
    server.on("/api/network/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleNetworkStatus(request);
    });

    server.on("/api/network/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleNetworkConfig(request);
    });

    server.on("/api/network/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleNetworkConfigPost(request, data, len, index, total);
        }
    );

    server.on("/api/network/force", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleNetworkForce(request, data, len, index, total);
        }
    );

    server.on("/api/network/reconnect", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleNetworkReconnect(request);
    });

    // File Manager API
    server.on("/api/files/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleFileList(request);
    });

    server.on("/api/files/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleFileDownload(request);
    });

    server.on("/api/files/view", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleFileView(request);
    });

    server.on("/api/files/read", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleFileRead(request);
    });

    server.on("/api/files/write", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleFileWrite(request);
    });

    server.on("/api/files/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleFileDelete(request);
    });

    server.on("/api/files/mkdir", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleFileCreateDir(request);
    });

    server.on("/api/files/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index,
               uint8_t *data, size_t len, bool final) {
            handleFileUpload(request, filename, index, data, len, final);
        }
    );

    // System
    server.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleRestart(request);
    });

    // OTA Update
    server.on("/api/ota", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            // Final response after upload completes
            if (otaUploadInProgress) {
                request->send(500, "application/json", "{\"error\":\"Upload in progress\"}");
                return;
            }

            if (!otaUploadError.isEmpty()) {
                String response = "{\"success\":false,\"error\":\"" + otaUploadError + "\"}";
                request->send(400, "application/json", response);
                otaUploadError = "";
                return;
            }

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Firmware updated, restarting...\"}");

            // Restart after response is sent
            delay(500);
            ESP.restart();
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index,
               uint8_t *data, size_t len, bool final) {
            handleOTA(request, filename, index, data, len, final);
        }
    );

    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

void WebServerManager::handleRoot(AsyncWebServerRequest *request) {
    // Try to serve compressed index.html
    if (LittleFS.exists("/web/index.html.gz")) {
        serveCompressedFile(request, "/web/index.html.gz", "text/html");
    } else if (LittleFS.exists("/web/index.html")) {
        request->send(LittleFS, "/web/index.html", "text/html");
    } else {
        // Fallback: send a basic HTML page
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>LoRa Gateway</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #1a1a2e; color: #eee; }
        h1 { color: #00d4ff; }
        .card { background: #16213e; padding: 20px; border-radius: 8px; margin: 10px 0; }
        a { color: #00d4ff; }
    </style>
</head>
<body>
    <h1>ESP32 LoRa Gateway</h1>
    <div class="card">
        <h3>Web Interface Not Found</h3>
        <p>The web interface files are not installed. Please upload the web files to LittleFS.</p>
        <p>API Status: <a href="/api/status">/api/status</a></p>
    </div>
</body>
</html>
)";
        request->send(200, "text/html", html);
    }
}

void WebServerManager::serveCompressedFile(AsyncWebServerRequest *request,
                                            const char* filepath,
                                            const char* contentType,
                                            bool isGzipped) {
    if (!LittleFS.exists(filepath)) {
        request->send(404, "text/plain", "File not found");
        return;
    }

    AsyncWebServerResponse *response = request->beginResponse(LittleFS, filepath, contentType);
    if (isGzipped) {
        response->addHeader("Content-Encoding", "gzip");
    }
    response->addHeader("Cache-Control", "max-age=86400");
    request->send(response);
}

void WebServerManager::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            break;

        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA:
            // Handle incoming WebSocket data if needed
            break;

        default:
            break;
    }
}

void WebServerManager::broadcastLog(const String& message) {
    if (ws.count() > 0) {
        DynamicJsonDocument doc(512);
        doc["type"] = "log";
        doc["message"] = message;
        doc["timestamp"] = millis();

        String json;
        serializeJson(doc, json);
        ws.textAll(json);
    }
}

void WebServerManager::handleStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);

    // System info
    doc["system"]["uptime"] = millis() / 1000;
    doc["system"]["heap_free"] = ESP.getFreeHeap();
    doc["system"]["heap_total"] = ESP.getHeapSize();
    doc["system"]["chip_model"] = ESP.getChipModel();
    doc["system"]["cpu_freq"] = ESP.getCpuFreqMHz();

    // WiFi info
    doc["wifi"]["connected"] = wifiConnectedToInternet;
    doc["wifi"]["ap_mode"] = wifiAPMode;
    doc["wifi"]["ssid"] = wifiAPMode ? WiFi.softAPSSID() : WiFi.SSID();
    doc["wifi"]["ip"] = wifiAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["wifi"]["rssi"] = wifiAPMode ? 0 : WiFi.RSSI();
    doc["wifi"]["mac"] = WiFi.macAddress();

    // Gateway info
    doc["gateway"]["eui"] = udpForwarder.getGatewayEuiString();
    doc["gateway"]["server_connected"] = udpForwarder.isConnected();

    // LoRa info
    doc["lora"]["available"] = loraGateway.isAvailable();
    doc["lora"]["receiving"] = loraGateway.isReceiving();

    GatewayConfig& loraCfg = loraGateway.getConfig();
    doc["lora"]["frequency"] = loraCfg.frequency;
    doc["lora"]["spreading_factor"] = loraCfg.spreadingFactor;
    doc["lora"]["bandwidth"] = loraCfg.bandwidth;

    GatewayStats& loraStats = loraGateway.getStats();
    doc["lora"]["rx_packets"] = loraStats.rxPacketsReceived;
    doc["lora"]["last_rssi"] = loraStats.lastRssi;
    doc["lora"]["last_snr"] = loraStats.lastSnr;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleStats(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);

    GatewayStats& loraStats = loraGateway.getStats();
    doc["lora"]["rx_received"] = loraStats.rxPacketsReceived;
    doc["lora"]["rx_forwarded"] = loraStats.rxPacketsForwarded;
    doc["lora"]["rx_crc_error"] = loraStats.rxPacketsCrcError;
    doc["lora"]["tx_sent"] = loraStats.txPacketsSent;
    doc["lora"]["tx_failed"] = loraStats.txPacketsFailed;
    doc["lora"]["last_rssi"] = loraStats.lastRssi;
    doc["lora"]["last_snr"] = loraStats.lastSnr;

    ForwarderStats& fwdStats = udpForwarder.getStats();
    doc["forwarder"]["push_sent"] = fwdStats.pushDataSent;
    doc["forwarder"]["push_ack"] = fwdStats.pushAckReceived;
    doc["forwarder"]["pull_sent"] = fwdStats.pullDataSent;
    doc["forwarder"]["pull_ack"] = fwdStats.pullAckReceived;
    doc["forwarder"]["downlinks"] = fwdStats.downlinksReceived;
    doc["forwarder"]["downlinks_sent"] = fwdStats.downlinksSent;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleResetStats(AsyncWebServerRequest *request) {
    loraGateway.resetStats();
    udpForwarder.resetStats();
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleLoRaConfig(AsyncWebServerRequest *request) {
    GatewayConfig& cfg = loraGateway.getConfig();

    DynamicJsonDocument doc(512);
    doc["enabled"] = cfg.enabled;
    doc["frequency"] = cfg.frequency;
    doc["spreading_factor"] = cfg.spreadingFactor;
    doc["bandwidth"] = cfg.bandwidth;
    doc["coding_rate"] = cfg.codingRate;
    doc["tx_power"] = cfg.txPower;
    doc["sync_word"] = cfg.syncWord;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleLoRaConfigPost(AsyncWebServerRequest *request,
                                             uint8_t *data, size_t len,
                                             size_t index, size_t total) {
    if (index + len != total) return;  // Wait for complete body

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    GatewayConfig& cfg = loraGateway.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("frequency")) cfg.frequency = doc["frequency"];
    if (doc.containsKey("spreading_factor")) cfg.spreadingFactor = doc["spreading_factor"];
    if (doc.containsKey("bandwidth")) cfg.bandwidth = doc["bandwidth"];
    if (doc.containsKey("coding_rate")) cfg.codingRate = doc["coding_rate"];
    if (doc.containsKey("tx_power")) cfg.txPower = doc["tx_power"];
    if (doc.containsKey("sync_word")) cfg.syncWord = doc["sync_word"];

    if (loraGateway.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"LoRa config saved. Restart to apply.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
}

void WebServerManager::handleServerConfig(AsyncWebServerRequest *request) {
    ForwarderConfig& cfg = udpForwarder.getConfig();

    DynamicJsonDocument doc(512);
    doc["enabled"] = cfg.enabled;
    doc["host"] = cfg.serverHost;
    doc["port_up"] = cfg.serverPortUp;
    doc["port_down"] = cfg.serverPortDown;
    doc["gateway_eui"] = udpForwarder.getGatewayEuiString();
    doc["description"] = cfg.description;
    doc["region"] = cfg.region;
    doc["latitude"] = cfg.latitude;
    doc["longitude"] = cfg.longitude;
    doc["altitude"] = cfg.altitude;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleServerConfigPost(AsyncWebServerRequest *request,
                                               uint8_t *data, size_t len,
                                               size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    ForwarderConfig& cfg = udpForwarder.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("host")) strlcpy(cfg.serverHost, doc["host"], sizeof(cfg.serverHost));
    if (doc.containsKey("port_up")) cfg.serverPortUp = doc["port_up"];
    if (doc.containsKey("port_down")) cfg.serverPortDown = doc["port_down"];
    if (doc.containsKey("description")) strlcpy(cfg.description, doc["description"], sizeof(cfg.description));
    if (doc.containsKey("region")) strlcpy(cfg.region, doc["region"], sizeof(cfg.region));
    if (doc.containsKey("latitude")) cfg.latitude = doc["latitude"];
    if (doc.containsKey("longitude")) cfg.longitude = doc["longitude"];
    if (doc.containsKey("altitude")) cfg.altitude = doc["altitude"];

    // Gateway EUI (hex string)
    if (doc.containsKey("gateway_eui")) {
        const char* euiStr = doc["gateway_eui"];
        if (strlen(euiStr) == 16) {
            for (int i = 0; i < 8; i++) {
                char hex[3] = {euiStr[i*2], euiStr[i*2+1], '\0'};
                cfg.gatewayEui[i] = strtol(hex, nullptr, 16);
            }
        }
    }

    if (udpForwarder.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Server config saved. Restart to apply.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
}

void WebServerManager::handleWiFiConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);

    // Current connection status
    doc["hostname"] = wifiHostname;
    doc["current_ssid"] = wifiSSID;
    doc["ap_mode"] = wifiAPMode;
    doc["connected"] = wifiConnectedToInternet;
    doc["ip"] = wifiAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["rssi"] = wifiAPMode ? 0 : WiFi.RSSI();

    // List of configured networks
    JsonArray networks = doc.createNestedArray("networks");
    for (size_t i = 0; i < wifiNetworks.size(); i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = wifiNetworks[i].ssid;
        // Don't send passwords for security
        network["has_password"] = wifiNetworks[i].password.length() > 0;
    }

    doc["max_networks"] = WIFI_MAX_NETWORKS;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleWiFiConfigPost(AsyncWebServerRequest *request,
                                             uint8_t *data, size_t len,
                                             size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    // Read current config
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Cannot read config\"}");
        return;
    }

    DynamicJsonDocument configDoc(JSON_BUFFER_SIZE);
    deserializeJson(configDoc, file);
    file.close();

    // Update hostname if provided
    if (doc.containsKey("hostname")) {
        String newHostname = doc["hostname"].as<String>();
        if (newHostname.length() > 0) {
            configDoc["wifi"]["hostname"] = newHostname;
        }
    }

    // Update ap_mode if provided
    if (doc.containsKey("ap_mode")) {
        configDoc["wifi"]["ap_mode"] = doc["ap_mode"];
    }

    // Check for action-based operations
    if (doc.containsKey("action")) {
        String action = doc["action"].as<String>();

        if (action == "add") {
            // Add a new network
            if (!doc.containsKey("ssid")) {
                request->send(400, "application/json", "{\"error\":\"Missing ssid\"}");
                return;
            }

            String newSsid = doc["ssid"].as<String>();
            String newPassword = doc["password"] | "";

            // Check if we have room
            if (!configDoc["wifi"].containsKey("networks")) {
                configDoc["wifi"].createNestedArray("networks");
            }
            JsonArray networks = configDoc["wifi"]["networks"].as<JsonArray>();

            if (networks.size() >= WIFI_MAX_NETWORKS) {
                request->send(400, "application/json", "{\"error\":\"Maximum networks reached\"}");
                return;
            }

            // Check for duplicate
            for (JsonObject net : networks) {
                if (net["ssid"].as<String>() == newSsid) {
                    // Update password for existing network
                    if (newPassword.length() > 0) {
                        net["password"] = newPassword;
                    }
                    goto save_config;
                }
            }

            // Add new network
            JsonObject newNet = networks.createNestedObject();
            newNet["ssid"] = newSsid;
            newNet["password"] = newPassword;

        } else if (action == "remove") {
            // Remove a network by SSID
            if (!doc.containsKey("ssid")) {
                request->send(400, "application/json", "{\"error\":\"Missing ssid\"}");
                return;
            }

            String removeSsid = doc["ssid"].as<String>();

            if (configDoc["wifi"].containsKey("networks")) {
                JsonArray networks = configDoc["wifi"]["networks"].as<JsonArray>();
                for (size_t i = 0; i < networks.size(); i++) {
                    if (networks[i]["ssid"].as<String>() == removeSsid) {
                        networks.remove(i);
                        break;
                    }
                }
            }

        } else if (action == "reorder") {
            // Reorder networks based on provided order
            if (!doc.containsKey("order") || !doc["order"].is<JsonArray>()) {
                request->send(400, "application/json", "{\"error\":\"Missing order array\"}");
                return;
            }

            JsonArray order = doc["order"].as<JsonArray>();
            JsonArray oldNetworks = configDoc["wifi"]["networks"].as<JsonArray>();

            // Create temporary storage for reordered networks
            DynamicJsonDocument tempDoc(512);
            JsonArray newNetworks = tempDoc.createNestedArray("networks");

            for (JsonVariant ssid : order) {
                String targetSsid = ssid.as<String>();
                for (JsonObject net : oldNetworks) {
                    if (net["ssid"].as<String>() == targetSsid) {
                        JsonObject newNet = newNetworks.createNestedObject();
                        newNet["ssid"] = net["ssid"];
                        newNet["password"] = net["password"];
                        break;
                    }
                }
            }

            // Replace networks array
            configDoc["wifi"]["networks"] = newNetworks;
        }

    } else if (doc.containsKey("networks") && doc["networks"].is<JsonArray>()) {
        // Replace entire networks list
        JsonArray newNetworks = doc["networks"].as<JsonArray>();

        // Clear and recreate networks array
        if (!configDoc["wifi"].containsKey("networks")) {
            configDoc["wifi"].createNestedArray("networks");
        }
        JsonArray configNetworks = configDoc["wifi"]["networks"];
        configNetworks.clear();

        int count = 0;
        for (JsonObject net : newNetworks) {
            if (count >= WIFI_MAX_NETWORKS) break;

            String ssid = net["ssid"] | "";
            if (ssid.length() == 0) continue;

            JsonObject newNet = configNetworks.createNestedObject();
            newNet["ssid"] = ssid;
            newNet["password"] = net["password"] | "";
            count++;
        }
    }

save_config:
    // Save config
    file = LittleFS.open("/config.json", "w");
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Cannot write config\"}");
        return;
    }

    serializeJsonPretty(configDoc, file);
    file.close();

    request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi config saved. Restart to apply.\"}");
}

void WebServerManager::handleWiFiScan(AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);  // Start async scan
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    if (n == WIFI_SCAN_RUNNING) {
        request->send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");

    for (int i = 0; i < n; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryption"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        network["channel"] = WiFi.channel(i);
    }

    WiFi.scanDelete();  // Clear scan results

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleNTPConfig(AsyncWebServerRequest *request) {
    NTPConfig& cfg = ntpManager.getConfig();
    NTPStatus& st = ntpManager.getStatus();

    DynamicJsonDocument doc(512);
    doc["enabled"] = cfg.enabled;
    doc["server1"] = cfg.server1;
    doc["server2"] = cfg.server2;
    doc["timezone_offset"] = cfg.timezoneOffset;
    doc["daylight_offset"] = cfg.daylightOffset;
    doc["sync_interval"] = cfg.syncInterval;
    doc["synced"] = st.synced;

    if (st.synced) {
        doc["current_time"] = ntpManager.getFormattedTime();
        doc["epoch"] = (unsigned long)ntpManager.getEpochTime();
    }

    JsonObject status = doc.createNestedObject("status");
    status["sync_count"] = st.syncCount;
    status["fail_count"] = st.failCount;
    if (st.lastSyncTime > 0) {
        status["last_sync_ago"] = (millis() - st.lastSyncTime) / 1000;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleNTPConfigPost(AsyncWebServerRequest *request,
                                            uint8_t *data, size_t len,
                                            size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    NTPConfig& cfg = ntpManager.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("server1")) strlcpy(cfg.server1, doc["server1"], sizeof(cfg.server1));
    if (doc.containsKey("server2")) strlcpy(cfg.server2, doc["server2"], sizeof(cfg.server2));
    if (doc.containsKey("timezone_offset")) cfg.timezoneOffset = doc["timezone_offset"];
    if (doc.containsKey("daylight_offset")) cfg.daylightOffset = doc["daylight_offset"];
    if (doc.containsKey("sync_interval")) cfg.syncInterval = doc["sync_interval"];

    if (ntpManager.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"NTP config saved. Restart to apply.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
}

void WebServerManager::handleNTPSync(AsyncWebServerRequest *request) {
    if (ntpManager.sync()) {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "Time synchronized";
        doc["current_time"] = ntpManager.getFormattedTime();
        doc["epoch"] = (unsigned long)ntpManager.getEpochTime();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Sync failed\"}");
    }
}

void WebServerManager::handleLCDConfig(AsyncWebServerRequest *request) {
    LCDConfig& cfg = lcdManager.getConfig();

    DynamicJsonDocument doc(512);
    doc["enabled"] = cfg.enabled;
    doc["address"] = cfg.address;
    doc["address_hex"] = String("0x") + String(cfg.address, HEX);
    doc["cols"] = cfg.cols;
    doc["rows"] = cfg.rows;
    doc["sda"] = cfg.sda;
    doc["scl"] = cfg.scl;
    doc["backlight"] = cfg.backlightOn;
    doc["rotation_interval"] = cfg.rotationInterval;
    doc["available"] = lcdManager.isAvailable();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleLCDConfigPost(AsyncWebServerRequest *request,
                                            uint8_t *data, size_t len,
                                            size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    LCDConfig& cfg = lcdManager.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("address")) cfg.address = doc["address"];
    if (doc.containsKey("cols")) cfg.cols = doc["cols"];
    if (doc.containsKey("rows")) cfg.rows = doc["rows"];
    if (doc.containsKey("sda")) cfg.sda = doc["sda"];
    if (doc.containsKey("scl")) cfg.scl = doc["scl"];
    if (doc.containsKey("backlight")) {
        cfg.backlightOn = doc["backlight"];
        // Apply backlight change immediately if LCD is available
        if (lcdManager.isAvailable()) {
            lcdManager.backlight(cfg.backlightOn);
        }
    }
    if (doc.containsKey("rotation_interval")) cfg.rotationInterval = doc["rotation_interval"];

    if (lcdManager.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"LCD config saved. Restart to apply pin changes.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
}

// ================== Buzzer Handlers ==================

void WebServerManager::handleBuzzerConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);

#if BUZZER_ENABLED
    doc["enabled"] = buzzer.isEnabled();
    doc["available"] = true;
    doc["pin"] = BUZZER_PIN;
    doc["startup_sound"] = buzzer.getConfig().startupSound;
    doc["packet_rx_sound"] = buzzer.getConfig().packetRxSound;
    doc["packet_tx_sound"] = buzzer.getConfig().packetTxSound;
    doc["volume"] = buzzer.getConfig().volume;
#else
    doc["enabled"] = false;
    doc["available"] = false;
    doc["pin"] = 0;
    doc["startup_sound"] = false;
    doc["packet_rx_sound"] = false;
    doc["packet_tx_sound"] = false;
    doc["volume"] = 0;
#endif

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleBuzzerConfigPost(AsyncWebServerRequest *request,
                                               uint8_t *data, size_t len,
                                               size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

#if BUZZER_ENABLED
    BuzzerConfig& cfg = buzzer.getConfig();

    if (doc.containsKey("enabled")) {
        bool enabled = doc["enabled"];
        buzzer.setEnabled(enabled);
    }
    if (doc.containsKey("startup_sound")) cfg.startupSound = doc["startup_sound"];
    if (doc.containsKey("packet_rx_sound")) cfg.packetRxSound = doc["packet_rx_sound"];
    if (doc.containsKey("packet_tx_sound")) cfg.packetTxSound = doc["packet_tx_sound"];
    if (doc.containsKey("volume")) cfg.volume = doc["volume"];

    if (buzzer.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Buzzer config saved.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
#else
    request->send(400, "application/json", "{\"error\":\"Buzzer not enabled in firmware\"}");
#endif
}

void WebServerManager::handleBuzzerTest(AsyncWebServerRequest *request,
                                         uint8_t *data, size_t len,
                                         size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

#if BUZZER_ENABLED
    String action = doc["action"] | "tone";

    if (action == "tone") {
        uint16_t freq = doc["frequency"] | 2000;
        uint16_t duration = doc["duration"] | 200;
        buzzer.beep(freq, duration);
        request->send(200, "application/json", "{\"success\":true}");
    } else if (action == "startup") {
        buzzer.playStartup();
        request->send(200, "application/json", "{\"success\":true}");
    } else if (action == "success") {
        buzzer.playSuccess();
        request->send(200, "application/json", "{\"success\":true}");
    } else if (action == "error") {
        buzzer.playError();
        request->send(200, "application/json", "{\"success\":true}");
    } else if (action == "stop") {
        buzzer.stop();
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(400, "application/json", "{\"error\":\"Unknown action\"}");
    }
#else
    request->send(400, "application/json", "{\"error\":\"Buzzer not enabled in firmware\"}");
#endif
}

void WebServerManager::handleGPSConfig(AsyncWebServerRequest *request) {
#if GPS_ENABLED
    request->send(200, "application/json", gpsManager.getStatusJson());
#else
    DynamicJsonDocument doc(128);
    doc["enabled"] = false;
    doc["error"] = "GPS not enabled in firmware";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
#endif
}

void WebServerManager::handleGPSConfigPost(AsyncWebServerRequest *request,
                                            uint8_t *data, size_t len,
                                            size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

#if GPS_ENABLED
    GPSConfig& cfg = gpsManager.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("use_fixed")) cfg.useFixedLocation = doc["use_fixed"];
    if (doc.containsKey("rx_pin")) cfg.rxPin = doc["rx_pin"];
    if (doc.containsKey("tx_pin")) cfg.txPin = doc["tx_pin"];
    if (doc.containsKey("baud_rate")) cfg.baudRate = doc["baud_rate"];
    if (doc.containsKey("latitude")) cfg.fixedLatitude = doc["latitude"];
    if (doc.containsKey("longitude")) cfg.fixedLongitude = doc["longitude"];
    if (doc.containsKey("altitude")) cfg.fixedAltitude = doc["altitude"];

    if (gpsManager.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"GPS config saved. Restart required for pin changes.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
#else
    request->send(400, "application/json", "{\"error\":\"GPS not enabled in firmware\"}");
#endif
}

// ================== RTC Handlers ==================

void WebServerManager::handleRTCConfig(AsyncWebServerRequest *request) {
#if RTC_ENABLED
    RTCConfig& cfg = rtcManager.getConfig();
    RTCStatus& st = rtcManager.getStatus();

    DynamicJsonDocument doc(512);
    doc["enabled"] = cfg.enabled;
    doc["i2cAddress"] = cfg.i2cAddress;
    doc["i2cAddressHex"] = String("0x") + String(cfg.i2cAddress, HEX);
    doc["sdaPin"] = cfg.sdaPin;
    doc["sclPin"] = cfg.sclPin;
    doc["syncWithNTP"] = cfg.syncWithNTP;
    doc["syncInterval"] = cfg.syncInterval;
    doc["squareWaveMode"] = cfg.squareWaveMode;
    doc["timezoneOffset"] = cfg.timezoneOffset;
    doc["available"] = st.available;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
#else
    DynamicJsonDocument doc(128);
    doc["enabled"] = false;
    doc["error"] = "RTC not enabled in firmware";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
#endif
}

void WebServerManager::handleRTCConfigPost(AsyncWebServerRequest *request,
                                            uint8_t *data, size_t len,
                                            size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

#if RTC_ENABLED
    RTCConfig& cfg = rtcManager.getConfig();

    if (doc.containsKey("enabled")) cfg.enabled = doc["enabled"];
    if (doc.containsKey("i2cAddress")) cfg.i2cAddress = doc["i2cAddress"];
    if (doc.containsKey("sdaPin")) cfg.sdaPin = doc["sdaPin"];
    if (doc.containsKey("sclPin")) cfg.sclPin = doc["sclPin"];
    if (doc.containsKey("syncWithNTP")) cfg.syncWithNTP = doc["syncWithNTP"];
    if (doc.containsKey("syncInterval")) cfg.syncInterval = doc["syncInterval"];
    if (doc.containsKey("squareWaveMode")) {
        cfg.squareWaveMode = doc["squareWaveMode"];
        rtcManager.setSquareWave(cfg.squareWaveMode);
    }
    if (doc.containsKey("timezoneOffset")) cfg.timezoneOffset = doc["timezoneOffset"];

    if (rtcManager.saveConfig()) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"RTC config saved. Restart for I2C pin changes.\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
#else
    request->send(400, "application/json", "{\"error\":\"RTC not enabled in firmware\"}");
#endif
}

void WebServerManager::handleRTCStatus(AsyncWebServerRequest *request) {
#if RTC_ENABLED
    request->send(200, "application/json", rtcManager.getStatusJson());
#else
    DynamicJsonDocument doc(128);
    doc["enabled"] = false;
    doc["available"] = false;
    doc["error"] = "RTC not enabled in firmware";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
#endif
}

void WebServerManager::handleRTCSync(AsyncWebServerRequest *request) {
#if RTC_ENABLED
    if (!rtcManager.isAvailable()) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"RTC not available\"}");
        return;
    }

    if (rtcManager.setTimeFromNTP()) {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["message"] = "RTC synchronized with NTP";
        doc["formattedDateTime"] = rtcManager.getFormattedDateTime();
        doc["epochTime"] = (unsigned long)rtcManager.getEpochTime();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"NTP sync failed\"}");
    }
#else
    request->send(400, "application/json", "{\"error\":\"RTC not enabled in firmware\"}");
#endif
}

void WebServerManager::handleRTCSetTime(AsyncWebServerRequest *request,
                                         uint8_t *data, size_t len,
                                         size_t index, size_t total) {
    if (index + len != total) return;

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

#if RTC_ENABLED
    if (!rtcManager.isAvailable()) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"RTC not available\"}");
        return;
    }

    // Option 1: Set from epoch time
    if (doc.containsKey("epoch")) {
        time_t epoch = doc["epoch"].as<unsigned long>();
        if (rtcManager.setTimeFromEpoch(epoch)) {
            DynamicJsonDocument respDoc(256);
            respDoc["success"] = true;
            respDoc["message"] = "Time set from epoch";
            respDoc["formattedDateTime"] = rtcManager.getFormattedDateTime();

            String response;
            serializeJson(respDoc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to set time\"}");
        }
        return;
    }

    // Option 2: Set from individual components
    if (doc.containsKey("year") && doc.containsKey("month") && doc.containsKey("day") &&
        doc.containsKey("hours") && doc.containsKey("minutes") && doc.containsKey("seconds")) {

        RTCDateTime dt;
        dt.year = doc["year"];
        dt.month = doc["month"];
        dt.day = doc["day"];
        dt.hours = doc["hours"];
        dt.minutes = doc["minutes"];
        dt.seconds = doc["seconds"];
        dt.dayOfWeek = RTCManager::calculateDayOfWeek(dt.year, dt.month, dt.day);

        if (rtcManager.setDateTime(dt)) {
            DynamicJsonDocument respDoc(256);
            respDoc["success"] = true;
            respDoc["message"] = "Time set successfully";
            respDoc["formattedDateTime"] = rtcManager.getFormattedDateTime();

            String response;
            serializeJson(respDoc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to set time\"}");
        }
        return;
    }

    request->send(400, "application/json", "{\"error\":\"Missing time parameters. Provide 'epoch' or year/month/day/hours/minutes/seconds\"}");
#else
    request->send(400, "application/json", "{\"error\":\"RTC not enabled in firmware\"}");
#endif
}

void WebServerManager::handleRestart(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
    delay(500);
    ESP.restart();
}

void WebServerManager::handleOTA(AsyncWebServerRequest *request, String filename,
                                  size_t index, uint8_t *data, size_t len, bool final) {
    if (index == 0) {
        Serial.printf("[OTA] Starting update: %s\n", filename.c_str());
        otaUploadInProgress = true;
        otaUploadError = "";

        // Validate firmware header
        if (len > 0 && !isValidESP32Firmware(data, len)) {
            otaUploadError = "Invalid firmware file";
            otaUploadInProgress = false;
            return;
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaUploadError = "Update.begin() failed";
            otaUploadInProgress = false;
            Serial.printf("[OTA] Error: %s\n", Update.errorString());
            return;
        }
    }

    if (otaUploadInProgress && len > 0) {
        if (Update.write(data, len) != len) {
            otaUploadError = "Update.write() failed";
            otaUploadInProgress = false;
            Serial.printf("[OTA] Error: %s\n", Update.errorString());
            return;
        }
    }

    if (final) {
        otaUploadInProgress = false;

        if (Update.end(true)) {
            Serial.printf("[OTA] Update success: %u bytes\n", index + len);
        } else {
            otaUploadError = "Update.end() failed";
            Serial.printf("[OTA] Error: %s\n", Update.errorString());
        }
    }
}

bool WebServerManager::isValidESP32Firmware(uint8_t *data, size_t len) {
    if (len < 4) return false;

    // ESP32 firmware starts with magic byte 0xE9
    if (data[0] != 0xE9) {
        Serial.printf("[OTA] Invalid magic byte: 0x%02X (expected 0xE9)\n", data[0]);
        return false;
    }

    return true;
}

void WebServerManager::loop() {
    // Cleanup disconnected WebSocket clients
    ws.cleanupClients();
}

// ============================================================================
// File Manager Implementation
// ============================================================================

bool WebServerManager::isValidPath(const String& path) {
    // Check empty path
    if (path.length() == 0) {
        return false;
    }

    // Check for path traversal
    if (path.indexOf("..") >= 0) {
        Serial.printf("[Files] Path traversal blocked: %s\n", path.c_str());
        return false;
    }

    // Must start with /
    if (!path.startsWith("/")) {
        return false;
    }

    // Check for backslashes
    if (path.indexOf('\\') >= 0) {
        return false;
    }

    // Limit path length
    if (path.length() > 128) {
        return false;
    }

    // Validate characters (alphanumeric, /, -, _, ., space)
    for (size_t i = 0; i < path.length(); i++) {
        char c = path.charAt(i);
        if (!isalnum(c) && c != '/' && c != '-' && c != '_' && c != '.' && c != ' ') {
            return false;
        }
    }

    return true;
}

void WebServerManager::handleFileList(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "application/json", "{\"error\":\"System busy\"}");
        return;
    }

    String path = "/";
    if (request->hasParam("dir")) {
        path = request->getParam("dir")->value();
        if (!isValidPath(path)) {
            request->send(400, "application/json", "{\"error\":\"Invalid directory path\"}");
            return;
        }
    }

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        request->send(404, "application/json", "{\"error\":\"Directory not found\"}");
        return;
    }

    DynamicJsonDocument doc(2048);
    JsonArray files = doc.createNestedArray("files");

    // Add storage info
    doc["total"] = LittleFS.totalBytes();
    doc["used"] = LittleFS.usedBytes();
    doc["free"] = LittleFS.totalBytes() - LittleFS.usedBytes();

    File file = root.openNextFile();
    while (file) {
        JsonObject fileObj = files.createNestedObject();
        fileObj["name"] = String(file.name());
        fileObj["size"] = file.size();
        fileObj["isDir"] = file.isDirectory();
        file = root.openNextFile();
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleFileDownload(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "text/plain", "System busy");
        return;
    }

    if (!request->hasParam("file")) {
        request->send(400, "text/plain", "Missing file parameter");
        return;
    }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "text/plain", "Invalid file path");
        return;
    }

    if (!LittleFS.exists(filepath)) {
        request->send(404, "text/plain", "File not found");
        return;
    }

    request->send(LittleFS, filepath, String(), true);
}

void WebServerManager::handleFileView(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "text/plain", "System busy");
        return;
    }

    if (!request->hasParam("file")) {
        request->send(400, "text/plain", "Missing file parameter");
        return;
    }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "text/plain", "Invalid file path");
        return;
    }

    if (!LittleFS.exists(filepath)) {
        request->send(404, "text/plain", "File not found");
        return;
    }

    request->send(LittleFS, filepath, "text/plain", false);
}

void WebServerManager::handleFileRead(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "application/json", "{\"error\":\"System busy\"}");
        return;
    }

    if (!request->hasParam("file")) {
        request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
        return;
    }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    if (!LittleFS.exists(filepath)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    File file = LittleFS.open(filepath, FILE_READ);
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }

    size_t fileSize = file.size();
    if (fileSize > 51200) {  // 50KB limit
        file.close();
        request->send(413, "application/json", "{\"error\":\"File too large (max 50KB)\"}");
        return;
    }

    // Check heap memory
    size_t requiredHeap = (fileSize * 3) + 2048;
    if (ESP.getFreeHeap() < requiredHeap) {
        file.close();
        request->send(503, "application/json", "{\"error\":\"Insufficient memory\"}");
        return;
    }

    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        file.close();
        request->send(500, "application/json", "{\"error\":\"Memory allocation failed\"}");
        return;
    }

    size_t bytesRead = file.readBytes(buffer, fileSize);
    buffer[bytesRead] = '\0';
    String content = String(buffer);
    free(buffer);
    file.close();

    DynamicJsonDocument doc(fileSize + 256);
    doc["status"] = "ok";
    doc["content"] = content;
    doc["size"] = fileSize;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleFileWrite(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "application/json", "{\"error\":\"System busy\"}");
        return;
    }

    if (!request->hasParam("file", true) || !request->hasParam("content", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
    }

    String filepath = request->getParam("file", true)->value();
    String content = request->getParam("content", true)->value();

    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    File file = LittleFS.open(filepath, FILE_WRITE);
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }

    size_t written = file.print(content);
    file.close();

    if (written > 0) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to write\"}");
    }
}

void WebServerManager::handleFileDelete(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "application/json", "{\"error\":\"System busy\"}");
        return;
    }

    if (!request->hasParam("file", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
        return;
    }

    String filepath = request->getParam("file", true)->value();
    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    File file = LittleFS.open(filepath);
    if (!file) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    bool isDir = file.isDirectory();
    file.close();

    bool success = isDir ? LittleFS.rmdir(filepath) : LittleFS.remove(filepath);
    if (success) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to delete\"}");
    }
}

void WebServerManager::handleFileCreateDir(AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
        request->send(503, "application/json", "{\"error\":\"System busy\"}");
        return;
    }

    if (!request->hasParam("dir", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing dir parameter\"}");
        return;
    }

    String dirpath = request->getParam("dir", true)->value();
    if (!isValidPath(dirpath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid directory path\"}");
        return;
    }

    if (LittleFS.mkdir(dirpath)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to create directory\"}");
    }
}

void WebServerManager::handleFileUpload(AsyncWebServerRequest *request, String filename,
                                         size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;

    if (otaUploadInProgress) return;

    if (index == 0) {
        String path = "/";
        if (request->hasParam("dir", false)) {
            path = request->getParam("dir", false)->value();
            if (!isValidPath(path)) {
                Serial.println("[Files] Path traversal blocked in upload");
                return;
            }
            if (path != "/" && !path.endsWith("/")) path += "/";
        }

        String filepath = path + filename;
        if (!isValidPath(filepath)) {
            Serial.println("[Files] Invalid filepath in upload");
            return;
        }

        Serial.printf("[Files] Upload starting: %s\n", filepath.c_str());
        if (LittleFS.exists(filepath)) LittleFS.remove(filepath);
        uploadFile = LittleFS.open(filepath, FILE_WRITE);
    }

    if (uploadFile && len) {
        uploadFile.write(data, len);
    }

    if (final && uploadFile) {
        Serial.printf("[Files] Upload complete: %u bytes\n", index + len);
        uploadFile.close();
    }
}

// ==================== Network Manager Handlers ====================

void WebServerManager::handleNetworkStatus(AsyncWebServerRequest *request) {
    if (networkManager) {
        String json = networkManager->getStatusJson();
        request->send(200, "application/json", json);
    } else {
        request->send(503, "application/json", "{\"error\":\"Network Manager not available\"}");
    }
}

void WebServerManager::handleNetworkConfig(AsyncWebServerRequest *request) {
    if (!networkManager) {
        request->send(503, "application/json", "{\"error\":\"Network Manager not available\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    NetworkManagerConfig& cfg = networkManager->getConfig();

    doc["wifi_enabled"] = cfg.wifiEnabled;
    doc["ethernet_enabled"] = cfg.ethernetEnabled;
    doc["primary"] = cfg.primary == PrimaryInterface::WIFI ? "wifi" : "ethernet";
    doc["failover_enabled"] = cfg.failoverEnabled;
    doc["failover_timeout"] = cfg.failoverTimeout;
    doc["reconnect_interval"] = cfg.reconnectInterval;

    // Ethernet config
    if (networkManager->getEthernet()) {
        EthernetConfig& ethCfg = networkManager->getEthernet()->getConfig();
        doc["ethernet"]["enabled"] = ethCfg.enabled;
        doc["ethernet"]["dhcp"] = ethCfg.useDHCP;
        doc["ethernet"]["static_ip"] = ethCfg.staticIP.toString();
        doc["ethernet"]["gateway"] = ethCfg.gateway.toString();
        doc["ethernet"]["subnet"] = ethCfg.subnet.toString();
        doc["ethernet"]["dns"] = ethCfg.dns.toString();
        doc["ethernet"]["dhcp_timeout"] = ethCfg.dhcpTimeout;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebServerManager::handleNetworkConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                                                size_t len, size_t index, size_t total) {
    if (!networkManager) {
        request->send(503, "application/json", "{\"error\":\"Network Manager not available\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    NetworkManagerConfig& cfg = networkManager->getConfig();

    if (doc.containsKey("wifi_enabled")) {
        cfg.wifiEnabled = doc["wifi_enabled"].as<bool>();
    }
    if (doc.containsKey("ethernet_enabled")) {
        cfg.ethernetEnabled = doc["ethernet_enabled"].as<bool>();
    }
    if (doc.containsKey("primary")) {
        String primary = doc["primary"].as<String>();
        cfg.primary = (primary == "ethernet") ? PrimaryInterface::ETHERNET : PrimaryInterface::WIFI;
    }
    if (doc.containsKey("failover_enabled")) {
        cfg.failoverEnabled = doc["failover_enabled"].as<bool>();
    }
    if (doc.containsKey("failover_timeout")) {
        cfg.failoverTimeout = doc["failover_timeout"].as<uint32_t>();
    }
    if (doc.containsKey("reconnect_interval")) {
        cfg.reconnectInterval = doc["reconnect_interval"].as<uint32_t>();
    }

    // Ethernet config
    if (doc.containsKey("ethernet") && networkManager->getEthernet()) {
        EthernetConfig& ethCfg = networkManager->getEthernet()->getConfig();
        JsonObject eth = doc["ethernet"];

        if (eth.containsKey("enabled")) {
            ethCfg.enabled = eth["enabled"].as<bool>();
        }
        if (eth.containsKey("dhcp")) {
            ethCfg.useDHCP = eth["dhcp"].as<bool>();
        }
        if (eth.containsKey("static_ip")) {
            ethCfg.staticIP.fromString(eth["static_ip"].as<const char*>());
        }
        if (eth.containsKey("gateway")) {
            ethCfg.gateway.fromString(eth["gateway"].as<const char*>());
        }
        if (eth.containsKey("subnet")) {
            ethCfg.subnet.fromString(eth["subnet"].as<const char*>());
        }
        if (eth.containsKey("dns")) {
            ethCfg.dns.fromString(eth["dns"].as<const char*>());
        }
        if (eth.containsKey("dhcp_timeout")) {
            ethCfg.dhcpTimeout = eth["dhcp_timeout"].as<uint16_t>();
        }
    }

    // Save configuration
    if (networkManager->saveConfig()) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to save config\"}");
    }
}

void WebServerManager::handleNetworkForce(AsyncWebServerRequest *request, uint8_t *data,
                                           size_t len, size_t index, size_t total) {
    if (!networkManager) {
        request->send(503, "application/json", "{\"error\":\"Network Manager not available\"}");
        return;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (doc.containsKey("interface")) {
        String iface = doc["interface"].as<String>();

        if (iface == "auto") {
            networkManager->setAutoMode();
            request->send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"auto\"}");
        } else if (iface == "wifi") {
            if (networkManager->forceInterface(NetworkType::WIFI)) {
                request->send(200, "application/json", "{\"status\":\"ok\",\"interface\":\"wifi\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"WiFi not available\"}");
            }
        } else if (iface == "ethernet") {
            if (networkManager->forceInterface(NetworkType::ETHERNET)) {
                request->send(200, "application/json", "{\"status\":\"ok\",\"interface\":\"ethernet\"}");
            } else {
                request->send(400, "application/json", "{\"error\":\"Ethernet not available\"}");
            }
        } else {
            request->send(400, "application/json", "{\"error\":\"Invalid interface\"}");
        }
    } else {
        request->send(400, "application/json", "{\"error\":\"Missing interface parameter\"}");
    }
}

void WebServerManager::handleNetworkReconnect(AsyncWebServerRequest *request) {
    if (!networkManager) {
        request->send(503, "application/json", "{\"error\":\"Network Manager not available\"}");
        return;
    }

    networkManager->reconnect();
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Reconnecting...\"}");
}
