#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "WS_Flow.h"

const char* pixelsURL = "https://traillink.pythonanywhere.com/SecretWSControlPanel/getPixels?v=2";

uint32_t pixels[8][8];

// WiFiManager instance
WiFiManager wifiManager;

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
// SHOW LOGO
// =========================
void showDoobitLogo() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      pixels[y][x] = DOOBIT_LOGO[y][x];
    }
  }

  renderPixels();
}

// =========================
// RUNNING LIGHTS ANIMATION
// =========================
void animateDoobitLogoRunningLights(unsigned long elapsedTime) {
  uint32_t logoColors[8][8];

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      logoColors[y][x] = DOOBIT_LOGO[y][x];
    }
  }

  int pixelCount = 0;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (DOOBIT_LOGO[y][x] != 0x000000) {
        pixelCount++;
      }
    }
  }

  int offPixelIndex = (elapsedTime / 92) % pixelCount;
  int currentPixel = 0;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {

      if (logoColors[y][x] != 0x000000) {

        if (currentPixel == offPixelIndex) {
          pixels[y][x] = 0x000000;
        } else {
          pixels[y][x] = logoColors[y][x];
        }

        currentPixel++;

      } else {
        pixels[y][x] = 0x000000;
      }
    }
  }
  renderPixels();
}

// =========================
// WIFI GLYPH
// =========================
const uint32_t WIFI_GLYPH[8][8] = {
  {0x0000FF,0x0000FF,0x0000FF,0x0000FF,0x000000,0x000000,0x000000,0x000000},
  {0x000000,0x000000,0x000000,0x000000,0x0000FF,0x000000,0x000000,0x000000},
  {0x000000,0x000000,0x000000,0x000000,0x000000,0x0000FF,0x000000,0x000000},
  {0x0000FF,0x0000FF,0x000000,0x000000,0x000000,0x000000,0x0000FF,0x000000},
  {0x000000,0x000000,0x0000FF,0x000000,0x000000,0x000000,0x000000,0x0000FF},
  {0x000000,0x000000,0x000000,0x0000FF,0x000000,0x000000,0x000000,0x0000FF},
  {0x000000,0x000000,0x000000,0x000000,0x0000FF,0x000000,0x000000,0x0000FF},
  {0x0000FF,0x000000,0x000000,0x000000,0x0000FF,0x000000,0x000000,0x0000FF}
};

// =========================
// BLUE WIFI GLYPH PULSE
// =========================
void showPortalPulse() {

  Serial.println("\n--- Portal Setup Activated ---");
  for (int i = 0; i < 3; i++) {
    // Show WiFi glyph
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        pixels[y][x] = WIFI_GLYPH[y][x];
      }
    }
    renderPixels();
    delay(300);

    // Clear screen
    Matrix.fillScreen(0);
    Matrix.show();
    delay(200);
  }
}

// =========================
// WIFI MANAGER PORTAL
// =========================
void setupWiFiManager() {
  Serial.println("\n--- Saved WiFi Failed ---");
  Serial.println("Starting WiFiManager Portal...");

  // Blue Captive portal activation pulse
  showPortalPulse();
  
  // OPTIONAL:
  // Uncomment ONLY when manually clearing credentials
  // wifiManager.resetSettings();

  // Portal timeout
  wifiManager.setTimeout(180);

  // Blocks here until:
  // 1. Successful WiFi config
  // 2. Timeout expires
  bool connected = wifiManager.autoConnect("Doobit-Setup", "doobit123");

  // Check if actually connected
  if (connected && WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi Configured via Portal!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return; // SUCCESS - exit function
  }

  // TIMEOUT OCCURRED - Retry
  Serial.println("\n✗ Portal Timeout - No Configuration Received");
  Serial.println("Retrying WiFi Setup...\n");

  setupWiFiManager(); // Call itself again to show pulse and retry
}

// =========================
// CONNECT WIFI
// =========================
void connectWiFi() {

  Serial.println("\nAttempting Saved WiFi Connection...");

  WiFi.mode(WIFI_STA);

  // Begin connection using saved credentials
  WiFi.begin();

  const int MAX_ATTEMPTS = 800; // 800 * 50ms = 40 seconds
  int attempts = 0;

  unsigned long animationStartTime = millis();

  // =========================
  // TRY SAVED WIFI
  // =========================
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {

    unsigned long elapsedTime = millis() - animationStartTime;

    animateDoobitLogoRunningLights(elapsedTime);

    delay(50);

    Serial.print(".");

    attempts++;
  }

  // =========================
  // SAVED WIFI SUCCESS
  // =========================
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\n✓ Connected Using Saved WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    return;
  }

  // =========================
  // SAVED WIFI FAILED
  // =========================
  Serial.println("\nSaved WiFi Connection Failed!");

  // Launch captive portal
  setupWiFiManager();

  // IMPORTANT:
  // autoConnect() only returns AFTER successful connection

  Serial.println("\n✓ Connected Using New WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// =========================
// FETCH PIXELS
// =========================
bool fetchPixels() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Not Connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  if (!https.begin(client, pixelsURL)) {
    Serial.println("HTTPS Begin Failed");
    return false;
  }

  https.setTimeout(5000);

  int code = https.GET();

  if (code != HTTP_CODE_OK) {

    Serial.printf("HTTP Error: %d\n", code);

    https.end();

    return false;
  }

  String payload = https.getString();

  https.end();

  Serial.printf("Payload Received: %s\n", payload.c_str());

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
// RENDER MATRIX
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

// =========================
// SETUP
// =========================
void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println("\n\n--- DOOBIT MATRIX BOOT ---");

  Matrix_Init();

  Matrix.setBrightness(15);

  // =========================
  // BOOT LOGO
  // =========================
  showDoobitLogo();

  delay(2000);

  // =========================
  // WIFI CONNECTION FLOW
  // =========================
  connectWiFi();

  // =========================
  // FETCH INITIAL CONTENT
  // =========================
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

  // =========================
  // WIFI HEALTH CHECK
  // =========================
  if (millis() - lastWiFiCheck > 10000) {

    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {

      Serial.println("WiFi Disconnected!");

      connectWiFi();

      return;
    }
  }

  // =========================
  // FETCH PIXELS
  // =========================
  if (millis() - lastFetch > 3000) {

    lastFetch = millis();

    if (fetchPixels()) {

      renderPixels();

    } else {

      Serial.println("Pixel Fetch Failed");
    }
  }
}
