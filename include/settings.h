#pragma once

constexpr auto MCP_PROTOCOL_VERSION = "2025-06-18";
constexpr auto MCP_NAME = "ESP32-CAM-AI MCP Server";
constexpr auto MCP_VERSION = "1.0.1";

constexpr auto MCP_CAPTURE_PIXELFORMAT = PIXFORMAT_JPEG;
constexpr auto MCP_CAPTURE_FRAMESIZE = FRAMESIZE_QVGA;
constexpr auto MCP_CAPTURE_JPEG_QUALITY = 30;
constexpr auto MCP_CAPTURE_WB_MODE = 0;
// Reduce memory usage to allow TLS (WebSocket) to work alongside the camera
constexpr auto MCP_CAPTURE_FRAMESIZE_DEFAULT = FRAMESIZE_QVGA;  // 320x240 instead of VGA 640x480
constexpr auto MCP_CAPTURE_JPEG_QUALITY_DEFAULT = 25;            // Lower quality = smaller buffer

constexpr auto WATCHDOG_TIMEOUT = 30000UL;
constexpr auto MAX_MCP_REQUEST_SIZE = 16384;

constexpr auto MCP_API_USER = "";
constexpr auto MCP_API_PASSWORD = "";
constexpr auto MCP_API_REALM = "ESP32-CAM-AI";
constexpr auto MCP_OTA_PASSWORD = "ESP32-CAM-AI";

// === Secondary WiFi (optional fallback) ===
#ifndef WIFI_SSID2
#define WIFI_SSID2 ""
#endif
#ifndef WIFI_PASSWORD2
#define WIFI_PASSWORD2 ""
#endif
#ifndef WIFI_SSID3
#define WIFI_SSID3 ""
#endif
#ifndef WIFI_PASSWORD3
#define WIFI_PASSWORD3 ""
#endif

// === Xiaozhi MCP WebSocket endpoint ===
// Get your token from https://xiaozhi.me → Console → MCP endpoint
// Hardcoded here because URLs with "://" break PlatformIO build flag parsing
#ifdef XIAOZHI_MCP_ENDPOINT
#undef XIAOZHI_MCP_ENDPOINT
#endif
#define XIAOZHI_MCP_ENDPOINT "wss://api.xiaozhi.me/mcp/?token=PASTE_YOUR_TOKEN_HERE"