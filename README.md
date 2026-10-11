
The ESP32-CAM connects directly to xiaozhi.me via WebSocket. No bridge needed.

## Features

✅ Native WebSocket client (custom C++ implementation)
✅ TLS to Cloudflare (WiFiClientSecure)
✅ MCP 2024-11-05 protocol (initialize, tools/list, tools/call, ping, notifications)
✅ Multi-network WiFi (WiFiMulti — up to 3 networks)
✅ HTTP forwarding to local MCP server
✅ Optimized photo size (HVGA 480x320 ~12KB JPEG)
✅ Stable connection with ping handling
✅ OTA firmware updates✅ Native WebSocket client (custom C++ implementation)
✅ TLS to Cloudflare (WiFiClientSecure)
✅ MCP 2024-11-05 protocol (initialize, tools/list, tools/call, ping, notifications)
✅ Multi-network WiFi (WiFiMulti — up to 3 networks)
✅ HTTP forwarding to local MCP server
✅ Google Gemini 1.5 Flash integration for free image analysis
✅ Flash LED control for better lighting
✅ HVGA resolution (480x320) for readable text
✅ Stable connection with ping handling
✅ OTA firmware updates
✅ Retry mechanism for API errors (429, 503, timeout)
✅ Specific error messages ("service busy, wait 2 minutes")
How it works
User says "take a picture" to Xiaozhi ESP32-S3
xiaozhi.me cloud calls capture tool via WebSocket
ESP32-CAM captures photo (HVGA, 480x320)
ESP32-CAM sends photo to Google Gemini API (free tier)
Gemini reads handwriting and returns text analysis
ESP32-CAM returns text to xiaozhi.me
GPT-4o receives text and speaks description
Setup
Clone this repo
Copy .env.template to .env, fill in WiFi credentials
Edit include/settings.h:
Replace YOUR_XIAOZHI_TOKEN_HERE with your xiaozhi.me MCP endpoint URL
Replace YOUR_GEMINI_API_KEY_HERE with your Google Gemini API key (from https://aistudio.google.com/app/apikey)
Install PlatformIO in VS Code
Plug in ESP32-CAM-MB, hold IO0 → press RST → release IO0
Click Upload in PlatformIO
Open serial monitor (115200 baud)
Say "take a picture" to your Xiaozhi ESP32-S3
Google Gemini API
This project uses Google Gemini 1.5 Flash for free image analysis:

Free tier: 15 requests per minute
Get your API key at: https://aistudio.google.com/app/apikey
The ESP32-CAM sends the photo to Gemini, which returns a text description
This bypasses xiaozhi.me's vision limitation (backend doesn't display images to LLM)
Error Handling
When Gemini API fails, the system returns specific messages:

429 (rate limit): "The image analysis service is busy. Please wait 2 minutes and try again."
503 (overloaded): "The AI service is overloaded. Please wait 30 seconds and try again."
Timeout: "The image analysis timed out. Please try again in a moment."
404 (unavailable): "The AI service is temporarily unavailable. Please try again later."
✅ Chess game assistance (Board recognition + strategy explanation)

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