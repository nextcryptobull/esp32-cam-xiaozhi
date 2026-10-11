#include "xiaozhi_mcp_client.h"
#include <esp_log.h>
#include <HTTPClient.h>

static const char* TAG = "XIAOZHI";

// Local HTTP MCP server URL (set dynamically in begin())
String LOCAL_MCP_URL = "http://127.0.0.1/";

void XiaozhiMcpClient::begin(const String& wsUrl) {
    parseUrl(wsUrl);
    ESP_LOGI(TAG, "Xiaozhi MCP client configured:");
    ESP_LOGI(TAG, "  Host: %s", _host.c_str());
    ESP_LOGI(TAG, "  Port: %u", _port);
    ESP_LOGI(TAG, "  Path: %s", _path.c_str());
    ESP_LOGI(TAG, "  TLS:  %s", _useTls ? "yes" : "no");
    _wifiClient.setInsecure();
    _wifiClient.setTimeout(30);
    LOCAL_MCP_URL = "http://" + WiFi.localIP().toString() + "/";
    ESP_LOGI(TAG, "  Local forward: %s", LOCAL_MCP_URL.c_str());
    ESP_LOGI(TAG, "Free heap after setup: %u bytes", ESP.getFreeHeap());
}

void XiaozhiMcpClient::loop() {
    if (!_connected) {
        if (millis() - _lastReconnectAttempt > 10000) {
            _lastReconnectAttempt = millis();
            ESP_LOGI(TAG, "Free heap: %u bytes (before reconnect)", ESP.getFreeHeap());
            connect();
            delay(100);
        }
        return;
    }
    processIncoming();
    if (millis() - _lastKeepalive > 30000) {
        _lastKeepalive = millis();
        uint8_t pingFrame[2] = {0x89, 0x00};
        _wifiClient.write(pingFrame, 2);
    }
    if (!_wifiClient.connected()) {
        ESP_LOGW(TAG, "WebSocket disconnected");
        _connected = false;
        _initialized = false;
    }
}

void XiaozhiMcpClient::registerTool(const String& name, const String& description,
                                     JsonObject inputSchema, McpToolHandler handler) {
    ESP_LOGI(TAG, "Tool '%s' registered (forwarder mode)", name.c_str());
}

void XiaozhiMcpClient::parseUrl(const String& url) {
    _useTls = url.startsWith("wss://");
    int schemeEnd = url.indexOf("://") + 3;
    int pathStart = url.indexOf('/', schemeEnd);
    if (pathStart < 0) {
        _host = url.substring(schemeEnd);
        _path = "/";
    } else {
        _host = url.substring(schemeEnd, pathStart);
        _path = url.substring(pathStart);
    }
    int colon = _host.indexOf(':');
    if (colon > 0) {
        _port = _host.substring(colon + 1).toInt();
        _host = _host.substring(0, colon);
    } else {
        _port = _useTls ? 443 : 80;
    }
    if (_useTls && _port == 80) {
        _port = 443;
    }
    ESP_LOGI(TAG, "Parsed URL: host=%s port=%u path=%s tls=%s",
             _host.c_str(), _port, _path.c_str(), _useTls ? "yes" : "no");
}

bool XiaozhiMcpClient::connect() {
    ESP_LOGI(TAG, "Connecting to %s:%u (TLS=%s) ...", _host.c_str(), _port, _useTls ? "yes" : "no");
    ESP_LOGI(TAG, "Free heap before TLS: %u bytes", ESP.getFreeHeap());
    if (!_wifiClient.connect(_host.c_str(), _port, 30000)) {
        ESP_LOGE(TAG, "TCP/TLS connect failed (host=%s port=%u)", _host.c_str(), _port);
        return false;
    }
    ESP_LOGI(TAG, "TCP connected, performing WebSocket handshake...");
    if (!performWsHandshake()) {
        ESP_LOGE(TAG, "WebSocket handshake failed");
        _wifiClient.stop();
        return false;
    }
    _connected = true;
    _initialized = false;
    _lastKeepalive = millis();
    ESP_LOGI(TAG, "WebSocket connected! Waiting for MCP initialize...");
    return true;
}

void XiaozhiMcpClient::disconnect() {
    if (_connected) {
        uint8_t closeFrame[2] = {0x88, 0x00};
        _wifiClient.write(closeFrame, 2);
    }
    _wifiClient.stop();
    _connected = false;
    _initialized = false;
    // Force TLS context cleanup
    delay(1000);
    ESP_LOGI(TAG, "Free heap after disconnect: %u bytes", ESP.getFreeHeap());
}

bool XiaozhiMcpClient::performWsHandshake() {
    uint8_t keyBytes[16];
    for (int i = 0; i < 16; i++) {
        keyBytes[i] = random(0, 256);
    }
    static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String key = "";
    for (int i = 0; i < 16; i += 3) {
        uint32_t n = ((uint32_t)keyBytes[i]) << 16;
        if (i + 1 < 16) n |= ((uint32_t)keyBytes[i+1]) << 8;
        if (i + 2 < 16) n |= ((uint32_t)keyBytes[i+2]);
        key += b64chars[(n >> 18) & 0x3F];
        key += b64chars[(n >> 12) & 0x3F];
        key += (i + 1 < 16) ? b64chars[(n >> 6) & 0x3F] : '=';
        key += (i + 2 < 16) ? b64chars[n & 0x3F] : '=';
    }

    String request = "GET " + _path + " HTTP/1.1\r\n" +
                     "Host: " + _host + ":" + _port + "\r\n" +
                     "Upgrade: websocket\r\n" +
                     "Connection: Upgrade\r\n" +
                     "Sec-WebSocket-Key: " + key + "\r\n" +
                     "Sec-WebSocket-Version: 13\r\n" +
                     "Origin: https://xiaozhi.me\r\n" +
                     "User-Agent: ESP32-CAM-AI/1.0\r\n" +
                     "\r\n";

    _wifiClient.print(request);

    String response = "";
    unsigned long timeout = millis() + 5000;
    while (millis() < timeout) {
        while (_wifiClient.available()) {
            char c = _wifiClient.read();
            response += c;
            if (response.endsWith("\r\n\r\n")) {
                goto handshake_done;
            }
        }
        delay(10);
    }
    ESP_LOGE(TAG, "Handshake timeout");
    return false;

handshake_done:
    if (response.indexOf("101") < 0) {
        ESP_LOGE(TAG, "WebSocket upgrade rejected");
        return false;
    }
    ESP_LOGI(TAG, "WebSocket handshake OK");
    return true;
}

String XiaozhiMcpClient::readWsFrame() {
    if (_wifiClient.available() < 2) {
        unsigned long timeout = millis() + 1000;
        while (_wifiClient.available() < 2 && millis() < timeout) {
            delay(1);
        }
        if (_wifiClient.available() < 2) return "";
    }

    uint8_t b1 = _wifiClient.read();
    uint8_t b2 = _wifiClient.read();

    uint8_t opcode = b1 & 0x0F;
    bool masked = (b2 & 0x80) != 0;
    uint64_t payloadLen = b2 & 0x7F;

    if (payloadLen == 126) {
        uint8_t ext[2];
        if (_wifiClient.readBytes(ext, 2) != 2) return "";
        payloadLen = (ext[0] << 8) | ext[1];
    } else if (payloadLen == 127) {
        uint8_t ext[8];
        if (_wifiClient.readBytes(ext, 8) != 8) return "";
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | ext[i];
        }
    }

    uint8_t mask[4];
    if (masked) {
        if (_wifiClient.readBytes(mask, 4) != 4) return "";
    }

    if (payloadLen > 32768) {
        ESP_LOGW(TAG, "WS frame too large (%llu bytes), truncating", payloadLen);
        payloadLen = 32768;
    }

    String payload;
    payload.reserve(payloadLen + 1);
    for (uint64_t i = 0; i < payloadLen; i++) {
        while (_wifiClient.available() == 0) {
            delay(1);
            if (millis() > _lastKeepalive + 60000) return "";
        }
        uint8_t c = _wifiClient.read();
        if (masked) c = c ^ mask[i % 4];
        payload += (char)c;
    }

    if (opcode == 0x8) {
        ESP_LOGI(TAG, "Received WebSocket close frame");
        _connected = false;
        return "";
    } else if (opcode == 0x9) {
        uint8_t pong[2] = {0x8A, 0x00};
        _wifiClient.write(pong, 2);
        return "";
    } else if (opcode == 0xA) {
        return "";
    }

    return payload;
}

bool XiaozhiMcpClient::sendWsText(const String& payload) {
    size_t len = payload.length();
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) mask[i] = random(0, 256);

    // Build the entire WebSocket frame in a single buffer
    // This avoids byte-by-byte writes which are extremely slow over TLS
    size_t headerLen;
    uint8_t header[10];
    
    if (len <= 125) {
        header[0] = 0x81;
        header[1] = 0x80 | len;
        headerLen = 2;
    } else if (len <= 65535) {
        header[0] = 0x81;
        header[1] = 0x80 | 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        headerLen = 4;
    } else {
        ESP_LOGE(TAG, "Payload too large for WS frame: %u bytes", len);
        return false;
    }

    // Build the masked payload in a single buffer
    size_t totalLen = headerLen + 4 + len;
    uint8_t* frame = (uint8_t*)malloc(totalLen);
    if (!frame) {
        ESP_LOGE(TAG, "Failed to allocate %u bytes for WS frame", totalLen);
        return false;
    }
    
    // Copy header
    memcpy(frame, header, headerLen);
    // Copy mask
    memcpy(frame + headerLen, mask, 4);
    // Copy masked payload
    for (size_t i = 0; i < len; i++) {
        frame[headerLen + 4 + i] = payload[i] ^ mask[i % 4];
    }

        // Write in chunks ??? TLS buffer is 16 KB, so write 8 KB at a time
    size_t totalWritten = 0;
    size_t chunkSize = 8192;  // 8 KB chunks
    while (totalWritten < totalLen) {
        size_t toWrite = (totalLen - totalWritten < chunkSize) ? (totalLen - totalWritten) : chunkSize;
        size_t written = _wifiClient.write(frame + totalWritten, toWrite);
        if (written == 0) {
            ESP_LOGE(TAG, "WS write failed at offset %u/%u", totalWritten, totalLen);
            free(frame);
            return false;
        }
        totalWritten += written;
        // Small delay between chunks to let TLS process
        if (totalWritten < totalLen) {
            delay(5);
        }
    }
    free(frame);
    
    if (totalWritten != totalLen) {
        ESP_LOGE(TAG, "WS write incomplete: %u/%u bytes", totalWritten, totalLen);
        return false;
    }
    
    return true;
}

void XiaozhiMcpClient::processIncoming() {
    String frame = readWsFrame();
    if (frame.length() == 0) return;

    ESP_LOGI(TAG, "RX from xiaozhi.me (%u bytes)", frame.length());
    // Print first 200 chars of the frame for debugging
    ESP_LOGI(TAG, "RAW: %.200s", frame.c_str());

    // Parse the incoming frame
    JsonDocument inDoc;
    DeserializationError err = deserializeJson(inDoc, frame);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error on incoming: %s", err.c_str());
        return;
    }

    JsonObject inObj = inDoc.as<JsonObject>();
    JsonObject payload;
    bool wrapped = false;

    // Detect xiaozhi's envelope: {type:"mcp", payload:{...}}
    if (inObj["type"] == "mcp" && inObj["payload"].is<JsonObject>()) {
        payload = inObj["payload"].as<JsonObject>();
        wrapped = true;
        ESP_LOGI(TAG, "Incoming message is wrapped (type=mcp)");
    } else {
        payload = inObj;
    }

    String method = payload["method"] | "";
    bool hasId = payload["id"].is<JsonVariant>();

    // Notifications: no "id" OR method starts with "notifications/"
    if (!hasId || method.startsWith("notifications/")) {
        ESP_LOGI(TAG, "Notification received (%s) - not forwarding", method.c_str());
        return;
    }
    // Handle initialize directly (the HTTP server's response is missing protocolVersion)
    if (method == "initialize") {
        ESP_LOGI(TAG, "Handling initialize directly");
        int id = payload["id"] | 0;
        JsonDocument resp;
        JsonObject root = resp.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        root["id"] = id;
        JsonObject result = root.createNestedObject("result");
        result["protocolVersion"] = "2024-11-05";
        JsonObject caps = result.createNestedObject("capabilities");
        caps.createNestedObject("tools");
        JsonObject serverInfo = result.createNestedObject("serverInfo");
        serverInfo["name"] = "ESP32-CAM-AI";
        serverInfo["version"] = "1.0.1";
        
        String outFrame;
        serializeJson(resp, outFrame);
        sendWsText(outFrame);
        ESP_LOGI(TAG, "Sent initialize response (%u bytes)", outFrame.length());
        return;
    }
    // Handle ping directly (keeps connection alive)
    if (method == "ping") {
        ESP_LOGI(TAG, "Handling ping directly");
        int id = payload["id"] | 0;
        JsonDocument resp;
        JsonObject root = resp.to<JsonObject>();
        root["jsonrpc"] = "2.0";
        root["id"] = id;
        JsonObject result = root.createNestedObject("result");
        result.createNestedObject("_meta");  // empty meta object
        
        String outFrame;
        serializeJson(resp, outFrame);
        sendWsText(outFrame);
        ESP_LOGI(TAG, "Sent ping response (%u bytes)", outFrame.length());
        return;
    }

    // Build the JSON-RPC payload to forward
    String rpcPayload;
    if (wrapped) {
        serializeJson(payload, rpcPayload);
    } else {
        rpcPayload = frame;
    }

    ESP_LOGI(TAG, "Forwarding to local HTTP: %s (%u bytes)", method.c_str(), rpcPayload.length());

    // Forward to local HTTP MCP server
    HTTPClient http;
    http.setConnectTimeout(5000);
    http.setTimeout(10000);
    http.setReuse(false);
    if (!http.begin(LOCAL_MCP_URL)) {
        ESP_LOGE(TAG, "Failed to begin HTTP request to local server");
        return;
    }
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(rpcPayload);
    if (httpCode <= 0) {
        ESP_LOGE(TAG, "HTTP POST to local server failed: %s", http.errorToString(httpCode).c_str());
        http.end();
        delay(100);
        return;
    }

    String responseBody = http.getString();
    http.end();
    delay(50);

    ESP_LOGI(TAG, "Local server responded: %d (%u bytes)", httpCode, responseBody.length());

    // Wrap the response if needed
    String outFrame;
    if (wrapped && responseBody.length() > 0) {
        JsonDocument respDoc;
        DeserializationError respErr = deserializeJson(respDoc, responseBody);
        if (respErr == DeserializationError::Ok) {
            JsonDocument outDoc;
            JsonObject outRoot = outDoc.to<JsonObject>();
            outRoot["type"] = "mcp";
            JsonObject outPayload = outRoot.createNestedObject("payload");
            outPayload.set(respDoc.as<JsonObject>());
            serializeJson(outDoc, outFrame);
            ESP_LOGI(TAG, "Wrapped response in {type:mcp, payload:...} (%u bytes)", outFrame.length());
        } else {
            ESP_LOGW(TAG, "HTTP response wasn't valid JSON, sending as-is");
            outFrame = responseBody;
        }
    } else {
        outFrame = responseBody;
    }

    // Send the response back over WebSocket
    if (outFrame.length() > 0) {
        sendWsText(outFrame);
        ESP_LOGI(TAG, "Sent response to xiaozhi.me (%u bytes)", outFrame.length());
    }
}

// Stub implementations
void XiaozhiMcpClient::handleMcpMessage(const JsonObject& obj) {}
void XiaozhiMcpClient::sendJson(const JsonDocument& doc) {}
void XiaozhiMcpClient::sendInitializeResponse(int id) {}
void XiaozhiMcpClient::sendToolsListResponse(int id) {}
void XiaozhiMcpClient::sendToolsCallResponse(int id, const JsonDocument& result, bool isError) {}
void XiaozhiMcpClient::sendErrorResponse(int id, int code, const String& message) {}