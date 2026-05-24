#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WS_Flow.h"

const char* ssid = "WIFISSID";
const char* password = "WIFIPASSWORD";

const char* pixelsURL = "PIXELS API";

uint32_t pixels[8][8];

// =========================
// DOOBIT LOGO
// =========================
const uint32_t DOOBIT_LOGO[8][8] = {
  {0xFF2020,0xFF2020,0x000000,0x3250FF,0x3250FF,0x3250FF,0x66FF1A,0x66FF1A},
  {0xFF2020,0x000000,0xFF2020,0x3250FF,0x000000,0x3250FF,0x000000,0x66FF1A},
  {0xFF2020,0xFF2020,0x000000,0x3250FF,0x3250FF,0x3250FF,0x66FF1A,0x66FF1A},
  {0xD92CFF,0xD92CFF,0xC7F01A,0xC7F01A,0xC7F01A,0x37D1E0,0x37D1E0,0x37D1E0},
  {0xD92CFF,0x000000,0xD92CFF,0xC7F01A,0x000000,0x000000,0x37D1E0,0x000000},
  {0xD92CFF,0xD92CFF,0x000000,0xC7F01A,0x000000,0x000000,0x37D1E0,0x000000},
  {0xD92CFF,0x000000,0xD92CFF,0xC7F01A,0x000000,0x000000,0x37D1E0,0x000000},
  {0xD92CFF,0xD92CFF,0xC7F01A,0xC7F01A,0xC7F01A,0x000000,0x37D1E0,0x000000}
};

// =========================
// STARTUP LOGO
// =========================
void showDoobitLogo() {
  // Copy logo into pixels array
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      pixels[y][x] = DOOBIT_LOGO[y][x];
    }
  }
  // Use renderPixels to display it
  renderPixels();
}

// =========================
// WIFI CONNECT
// =========================
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  const int MAX_ATTEMPTS = 80; // 40 seconds (80 * 500ms)

  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed! Retrying...");
    // Red screen = WiFi failed
    Matrix.fillScreen(Matrix.Color(20, 0, 0));
    Matrix.show();
    delay(2000);
  }
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  delay(500); // Let serial stabilize
  
  Serial.println("\n\nStarting up...");
  
  Matrix_Init();
  Matrix.setBrightness(15);
  
  showDoobitLogo();
  
  // Keep showing logo until WiFi connects
  while (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  // WiFi connected! Fetch initial content from server
  if (fetchPixels()) {
    renderPixels();
  }
}

// =========================
// MAIN LOOP
// =========================
void loop() {
  static unsigned long lastFetch = 0;
  static unsigned long lastWiFiCheck = 0;

  // Check WiFi connection every 10 seconds
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected! Reconnecting...");
      connectWiFi();
      return;
    }
  }

  // Fetch pixels every 3 seconds
  if (millis() - lastFetch > 3000) {
    lastFetch = millis();
    
    if (fetchPixels()) {
      renderPixels();
    } else {
      Serial.println("Failed to fetch pixels");
    }
  }
}

// =========================
// FETCH JSON
// =========================
bool fetchPixels() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping fetch");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  if (!https.begin(client, pixelsURL)) {
    Serial.println("Failed to begin HTTPS request");
    return false;
  }

  https.setTimeout(5000); // 5 second timeout
  int code = https.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("HTTP Error: %d\n", code);
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  Serial.printf("Got payload: %s\n", payload.c_str());
  return parsePixelJSON(payload);
}

// =========================
// PARSE JSON
// =========================
bool parsePixelJSON(String payload) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON Error: ");
    Serial.println(error.c_str());
    return false;
  }

  if (doc["brightness"].is<int>()) {
    Matrix.setBrightness(doc["brightness"]);
  }

  bool sleeping = doc["is_sleeping"] | false;

  if (sleeping) {
    Matrix.fillScreen(0);
    Matrix.show();
    return false;
  }

  JsonArray pixelData = doc["pixels"];

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (pixelData[y][x].is<const char*>()) {
        const char* hexStr = pixelData[y][x];
        pixels[y][x] = strtol(hexStr, NULL, 16);
      } else {
        pixels[y][x] = pixelData[y][x].as<uint32_t>();
      }
    }
  }

  return true;
}

// =========================
// RENDER MATRIX (unified for logo + dynamic updates)
// =========================
void renderPixels() {
  Matrix.fillScreen(0);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      uint32_t c = pixels[y][x];

      uint8_t r = (c >> 16) & 0xFF;
      uint8_t g = (c >> 8) & 0xFF;
      uint8_t b = c & 0xFF;

      int rx = 7 - y;
      int ry = x;

      Matrix.drawPixel(rx, ry, Matrix.Color(g, r, b));
    }
  }

  Matrix.show();
}