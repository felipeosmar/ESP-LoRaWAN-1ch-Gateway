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
    void handleStats(AsyncWebServerRequest *request);
    void handleResetStats(AsyncWebServerRequest *request);
    void handleRestart(AsyncWebServerRequest *request);

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
