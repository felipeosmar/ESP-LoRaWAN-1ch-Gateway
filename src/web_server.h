#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"

class WebServerManager {
public:
    WebServerManager();

    void begin();
    void loop();
    void broadcastLog(const String& message);

private:
    AsyncWebServer server;
    AsyncWebSocket ws;

    bool otaUploadInProgress;
    String otaUploadError;

    // Helper functions
    void setupRoutes();
    void serveCompressedFile(AsyncWebServerRequest *request, const char* filepath,
                             const char* contentType, bool isGzipped = true);

    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);

    // API handlers
    void handleStatus(AsyncWebServerRequest *request);
    void handleLoRaConfig(AsyncWebServerRequest *request);
    void handleLoRaConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                               size_t len, size_t index, size_t total);
    void handleServerConfig(AsyncWebServerRequest *request);
    void handleServerConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                                 size_t len, size_t index, size_t total);
    void handleWiFiConfig(AsyncWebServerRequest *request);
    void handleWiFiConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                               size_t len, size_t index, size_t total);
    void handleWiFiScan(AsyncWebServerRequest *request);
    void handleNTPConfig(AsyncWebServerRequest *request);
    void handleNTPConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                              size_t len, size_t index, size_t total);
    void handleNTPSync(AsyncWebServerRequest *request);
    void handleLCDConfig(AsyncWebServerRequest *request);
    void handleLCDConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                              size_t len, size_t index, size_t total);
    void handleBuzzerConfig(AsyncWebServerRequest *request);
    void handleBuzzerConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                                 size_t len, size_t index, size_t total);
    void handleBuzzerTest(AsyncWebServerRequest *request, uint8_t *data,
                           size_t len, size_t index, size_t total);
    void handleGPSConfig(AsyncWebServerRequest *request);
    void handleGPSConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                              size_t len, size_t index, size_t total);
    void handleRTCConfig(AsyncWebServerRequest *request);
    void handleRTCConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                              size_t len, size_t index, size_t total);
    void handleRTCStatus(AsyncWebServerRequest *request);
    void handleRTCSync(AsyncWebServerRequest *request);
    void handleRTCSetTime(AsyncWebServerRequest *request, uint8_t *data,
                           size_t len, size_t index, size_t total);
    void handleStats(AsyncWebServerRequest *request);
    void handleResetStats(AsyncWebServerRequest *request);
    void handleRestart(AsyncWebServerRequest *request);

    // Network Manager handlers
    void handleNetworkStatus(AsyncWebServerRequest *request);
    void handleNetworkHealth(AsyncWebServerRequest *request);
    void handleNetworkConfig(AsyncWebServerRequest *request);
    void handleNetworkConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                                  size_t len, size_t index, size_t total);
    void handleNetworkForce(AsyncWebServerRequest *request, uint8_t *data,
                             size_t len, size_t index, size_t total);
    void handleNetworkReconnect(AsyncWebServerRequest *request);

    // File Manager handlers
    void handleFileList(AsyncWebServerRequest *request);
    void handleFileDownload(AsyncWebServerRequest *request);
    void handleFileView(AsyncWebServerRequest *request);
    void handleFileRead(AsyncWebServerRequest *request);
    void handleFileWrite(AsyncWebServerRequest *request);
    void handleFileDelete(AsyncWebServerRequest *request);
    void handleFileCreateDir(AsyncWebServerRequest *request);
    void handleFileUpload(AsyncWebServerRequest *request, String filename,
                          size_t index, uint8_t *data, size_t len, bool final);

    // OTA handler
    void handleOTA(AsyncWebServerRequest *request, String filename, size_t index,
                   uint8_t *data, size_t len, bool final);
    bool isValidESP32Firmware(uint8_t *data, size_t len);

    // Security helpers
    bool isValidPath(const String& path);
};

// Global instance
extern WebServerManager webServer;

#endif // WEB_SERVER_H
