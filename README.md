
The ESP32-CAM connects directly to xiaozhi.me via WebSocket. No bridge needed.

## Features

- ✅ Native WebSocket client (custom C++ implementation)
- ✅ TLS to Cloudflare (WiFiClientSecure)
- ✅ MCP 2024-11-05 protocol (initialize, tools/list, tools/call, ping, notifications)
- ✅ Multi-network WiFi (WiFiMulti — up to 3 networks)
- ✅ HTTP forwarding to local MCP server
- ✅ Optimized photo size (QVGA 320×240 ~6KB JPEG)
- ✅ Stable connection with ping handling
- ✅ OTA firmware updates

## Hardware

- AI-Thinker ESP32-CAM (OV2640 camera)
- Xiaozhi ESP32-S3 voice assistant
- WiFi router (2.4 GHz)

## Setup

1. Clone this repo
2. Copy `.env.template` to `.env`, fill in WiFi credentials
3. Edit `include/settings.h`, replace `PASTE_YOUR_TOKEN_HERE` with your xiaozhi.me MCP endpoint URL
4. Install PlatformIO in VS Code
5. Plug in ESP32-CAM-MB, hold IO0 → press RST → release IO0
6. Click Upload in PlatformIO
7. Open serial monitor (115200 baud)
8. Say "take a picture" to your Xiaozhi ESP32-S3

## Files

- `src/main.cpp` — main firmware (multi-WiFi, MCP HTTP server, Xiaozhi client integration)
- `src/xiaozhi_mcp_client.cpp` — WebSocket client + MCP protocol
- `include/xiaozhi_mcp_client.h` — client class declaration
- `include/settings.h` — configuration (capture resolution, token, etc.)
- `.env.template` — WiFi credentials template

## Based on

Forked from https://github.com/rzeldent/esp32-cam-ai (MIT license)

## License

MIT