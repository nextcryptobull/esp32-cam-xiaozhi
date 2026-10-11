#pragma once

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <functional>

using McpToolHandler = std::function<JsonDocument(const JsonObject& args)>;

struct McpToolDef {
    String name;
    String description;
    JsonObject inputSchema;
    McpToolHandler handler;
};

class XiaozhiMcpClient {
public:
    void begin(const String& wsUrl);
    void loop();
    void registerTool(const String& name, const String& description, JsonObject inputSchema, McpToolHandler handler);
    bool isConnected() const { return _connected; }

private:
    String _host;
    String _path;
    uint16_t _port = 443;
    bool _useTls = true;
    bool _connected = false;
    bool _initialized = false;
    unsigned long _lastReconnectAttempt = 0;
    unsigned long _lastKeepalive = 0;

    WiFiClientSecure _wifiClient;

    static const int MAX_TOOLS = 16;
    McpToolDef _tools[MAX_TOOLS];
    int _toolCount = 0;

    void parseUrl(const String& url);
    bool connect();
    void disconnect();
    void processIncoming();
    void handleMcpMessage(const JsonObject& obj);
    void sendJson(const JsonDocument& doc);
    void sendInitializeResponse(int id);
    void sendToolsListResponse(int id);
    void sendToolsCallResponse(int id, const JsonDocument& result, bool isError = false);
    void sendErrorResponse(int id, int code, const String& message);
    bool sendWsText(const String& payload);
    String readWsFrame();
    bool performWsHandshake();
};
