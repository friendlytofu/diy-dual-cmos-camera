#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WebServer.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "config.h"

// FINAL U1: outward OV2640, microSD, shutter button, and camera selector.
constexpr int BUTTON_LINE_A = 32;  // SW1 shutter
constexpr int BUTTON_LINE_B = 33;  // SW2 direct
constexpr uint32_t DEBOUNCE_MS = 40;
constexpr uint32_t SELECTOR_HOLD_MS = 2000;
constexpr framesize_t STILL_FRAME_SIZE = FRAMESIZE_UXGA;
constexpr int STILL_JPEG_QUALITY = 8;
constexpr framesize_t DEFAULT_PREVIEW_FRAME_SIZE = FRAMESIZE_QVGA;
constexpr int DEFAULT_PREVIEW_JPEG_QUALITY = 20;

constexpr int PWDN_GPIO_NUM = -1, RESET_GPIO_NUM = -1, XCLK_GPIO_NUM = 21;
constexpr int SIOD_GPIO_NUM = 26, SIOC_GPIO_NUM = 27;
constexpr int Y9_GPIO_NUM = 35, Y8_GPIO_NUM = 34, Y7_GPIO_NUM = 39;
constexpr int Y6_GPIO_NUM = 36, Y5_GPIO_NUM = 19, Y4_GPIO_NUM = 18;
constexpr int Y3_GPIO_NUM = 5, Y2_GPIO_NUM = 4;
constexpr int VSYNC_GPIO_NUM = 25, HREF_GPIO_NUM = 23, PCLK_GPIO_NUM = 22;

enum class Key : uint8_t { Released, Direct };
struct ButtonLine {
  int pin;
  Key candidate = Key::Released, stable = Key::Released;
  uint32_t changedAt = 0, pressedAt = 0;
  bool longSent = false;
};

ButtonLine lineA{BUTTON_LINE_A}, lineB{BUTTON_LINE_B};
WebServer server(80);
uint32_t shutterCount = 0, selectorCount = 0, selectorHoldCount = 0;
uint32_t fileNumber = 1;
bool cameraReady = false, cardReady = false;
String lastCapturePath;
String frameSizeName = "qvga";
int jpegQuality = DEFAULT_PREVIEW_JPEG_QUALITY;
bool verticalFlip = false;

framesize_t parseFrameSize(const String& name);
void writeBytes(File& file);

Key readKey(int pin) {
  return digitalRead(pin) == LOW ? Key::Direct : Key::Released;
}

void updateLine(ButtonLine& line, bool lineASelected) {
  const uint32_t now = millis();
  const Key raw = readKey(line.pin);
  if (raw != line.candidate) { line.candidate = raw; line.changedAt = now; }
  if (line.candidate != line.stable && now - line.changedAt >= DEBOUNCE_MS) {
    line.stable = line.candidate;
    line.longSent = false;
    if (line.stable == Key::Released) return;
    line.pressedAt = now;
    if (lineASelected && line.stable == Key::Direct) ++shutterCount;
    if (!lineASelected && line.stable == Key::Direct) ++selectorCount;
  }
  if (!lineASelected && line.stable == Key::Direct && !line.longSent &&
      now - line.pressedAt >= SELECTOR_HOLD_MS) {
    line.longSent = true;
    ++selectorHoldCount;
  }
}

bool startCamera() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM; c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM; c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM; c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM; c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM; c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM; c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000; c.pixel_format = PIXFORMAT_JPEG;
  // Allocate buffers for maximum-resolution stills, then run the sensor at a
  // 160 x 120 preview size. Saved files stay clear without slowing every frame.
  c.frame_size = STILL_FRAME_SIZE; c.jpeg_quality = STILL_JPEG_QUALITY; c.fb_count = 2;
  c.grab_mode = CAMERA_GRAB_LATEST; c.fb_location = CAMERA_FB_IN_PSRAM;
  if (!psramFound() || esp_camera_init(&c) != ESP_OK) return false;
  sensor_t* sensor = esp_camera_sensor_get();
  if (!sensor) return false;
  // The outward view is intentionally mirrored on LCD, website and captures.
  sensor->set_hmirror(sensor, 1);
  return sensor->set_framesize(sensor, DEFAULT_PREVIEW_FRAME_SIZE) == 0 &&
         sensor->set_quality(sensor, jpegQuality) == 0;
}

bool startCard() {
  for (int attempt = 0; attempt < 3; ++attempt) {
    if (SD_MMC.begin("/sdcard", true) && SD_MMC.cardType() != CARD_NONE) return true;
    SD_MMC.end(); delay(400);
  }
  return false;
}

bool saveFrame(camera_fb_t* frame, String& path) {
  if (!cardReady || !frame) return false;
  do { path = "/OUT_" + String(fileNumber++) + ".JPG"; }
  while (SD_MMC.exists(path));
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) return false;
  const bool ok = file.write(frame->buf, frame->len) == frame->len;
  file.close();
  if (ok) lastCapturePath = path;
  return ok;
}

void restorePreviewSettings() {
  sensor_t* sensor = esp_camera_sensor_get();
  if (!sensor) return;
  sensor->set_framesize(sensor, parseFrameSize(frameSizeName));
  sensor->set_quality(sensor, jpegQuality);
}

camera_fb_t* captureMaximumStill() {
  sensor_t* sensor = esp_camera_sensor_get();
  if (!sensor) return nullptr;
  if (sensor->set_quality(sensor, STILL_JPEG_QUALITY) != 0 ||
      sensor->set_framesize(sensor, STILL_FRAME_SIZE) != 0) {
    restorePreviewSettings();
    return nullptr;
  }
  // Discard both queued preview buffers so the returned image is UXGA.
  for (int index = 0; index < 2; ++index) {
    camera_fb_t* queued = esp_camera_fb_get();
    if (!queued) { restorePreviewSettings(); return nullptr; }
    esp_camera_fb_return(queued);
  }
  return esp_camera_fb_get();
}








void sendLatestCapture() {
  if (!cardReady || !lastCapturePath.length() || !SD_MMC.exists(lastCapturePath)) {
    server.send(404, "text/plain", "No outward capture yet"); return;
  }
  File file = SD_MMC.open(lastCapturePath, FILE_READ);
  if (!file) { server.send(500, "text/plain", "Cannot read outward capture"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(file, "image/jpeg");
  file.close();
}

bool validOutwardFileName(String name) {
  if (name.startsWith("/")) name.remove(0, 1);
  if (!name.startsWith("OUT_") || !name.endsWith(".JPG") || name.length() < 9) return false;
  for (int i = 4; i < name.length() - 4; ++i) {
    if (!isDigit(name[i])) return false;
  }
  return true;
}

void listOutwardFiles() {
  if (!cardReady) { server.send(503, "application/json", "{\"error\":\"card unavailable\"}"); return; }
  File root = SD_MMC.open("/");
  if (!root) { server.send(500, "application/json", "{\"error\":\"cannot open card\"}"); return; }
  String body = "{\"camera\":\"outward\",\"files\":[";
  bool first = true;
  uint16_t count = 0;
  File file = root.openNextFile();
  while (file && count < 400) {
    String name = file.name();
    if (name.startsWith("/")) name.remove(0, 1);
    if (!file.isDirectory() && validOutwardFileName(name)) {
      if (!first) body += ',';
      body += "{\"name\":\"" + name + "\",\"size\":" + String((uint32_t)file.size()) + "}";
      first = false;
      ++count;
    }
    file.close();
    file = root.openNextFile();
  }
  const bool truncated = static_cast<bool>(file);
  if (file) file.close();
  root.close();
  body += String("],\"truncated\":") + (truncated ? "true" : "false") + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

void sendOutwardFile() {
  String name = server.arg("name");
  if (!validOutwardFileName(name)) { server.send(400, "text/plain", "Invalid outward filename"); return; }
  if (name.startsWith("/")) name.remove(0, 1);
  const String path = "/" + name;
  if (!cardReady || !SD_MMC.exists(path)) { server.send(404, "text/plain", "File not found"); return; }
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) { server.send(500, "text/plain", "Cannot read file"); return; }
  server.sendHeader("Cache-Control", "no-store");
  if (!server.hasArg("inline")) {
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  }
  server.setContentLength(file.size());
  server.send(200, "image/jpeg", "");
  writeBytes(file);
  file.close();
}

void writeBytes(File& file) {
  static uint8_t buffer[8192];
  WiFiClient client = server.client();
  client.setNoDelay(true);
  while (file.available() && client.connected()) {
    const size_t received = file.read(buffer, sizeof(buffer));
    if (!received) break;
    size_t offset = 0;
    uint32_t lastProgress = millis();
    while (offset < received && client.connected()) {
      const size_t sent = client.write(buffer + offset, received - offset);
      if (!sent) {
        if (millis() - lastProgress > 4000) return;
        delay(1);
        continue;
      }
      offset += sent;
      lastProgress = millis();
    }
  }
}

void writeBytes(const uint8_t* data, size_t length) {
  WiFiClient client = server.client();
  client.setNoDelay(true);
  uint32_t lastProgress = millis();
  while (length && client.connected()) {
    const size_t count = client.write(data, min(length, static_cast<size_t>(8192)));
    if (!count) {
      if (millis() - lastProgress > 4000) break;
      delay(1);
      continue;
    }
    lastProgress = millis();
    data += count; length -= count;
  }
}

void sendFrame(bool save) {
  camera_fb_t* frame = cameraReady ? (save ? captureMaximumStill() : esp_camera_fb_get()) : nullptr;
  if (!frame) {
    if (save) restorePreviewSettings();
    server.send(503, "text/plain", "outward camera unavailable"); return;
  }
  String path;
  const bool saved = save && saveFrame(frame, path);
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-CamBuild-Saved", saved ? "1" : "0");
  if (saved) server.sendHeader("X-CamBuild-Path", path);
  server.setContentLength(frame->len);
  server.send(200, "image/jpeg", "");
  writeBytes(frame->buf, frame->len);
  esp_camera_fb_return(frame);
  if (save) restorePreviewSettings();
}

String jsonString(const String& body, const char* key) {
  const String marker = String('"') + key + '"';
  int p = body.indexOf(marker); if (p < 0) return "";
  p = body.indexOf(':', p + marker.length());
  p = body.indexOf('"', p + 1); if (p < 0) return "";
  const int end = body.indexOf('"', p + 1);
  return end < 0 ? "" : body.substring(p + 1, end);
}

int jsonInt(const String& body, const char* key, int fallback) {
  const String marker = String('"') + key + '"';
  int p = body.indexOf(marker); if (p < 0) return fallback;
  p = body.indexOf(':', p + marker.length());
  return p < 0 ? fallback : body.substring(p + 1).toInt();
}

framesize_t parseFrameSize(const String& name) {
  if (name == "qqvga") return FRAMESIZE_QQVGA;
  if (name == "qvga") return FRAMESIZE_QVGA;
  if (name == "svga") return FRAMESIZE_SVGA;
  if (name == "uxga") return FRAMESIZE_UXGA;
  return FRAMESIZE_VGA;
}

void handleControl() {
  const String body = server.arg("plain");
  const String setting = jsonString(body, "setting");
  sensor_t* sensor = esp_camera_sensor_get();
  if (!sensor) { server.send(503, "text/plain", "outward camera unavailable"); return; }
  int result = -1;
  if (setting == "vflip") {
    verticalFlip = jsonInt(body, "value", 0) != 0;
    result = sensor->set_vflip(sensor, verticalFlip);
  } else if (setting == "quality") {
    jpegQuality = constrain(jsonInt(body, "value", 12), 10, 35);
    result = sensor->set_quality(sensor, jpegQuality);
  } else if (setting == "framesize") {
    frameSizeName = jsonString(body, "value");
    if (frameSizeName != "qqvga" && frameSizeName != "qvga" && frameSizeName != "vga" &&
        frameSizeName != "svga" && frameSizeName != "uxga") frameSizeName = "vga";
    result = sensor->set_framesize(sensor, parseFrameSize(frameSizeName));
  }
  server.send(result == 0 ? 204 : 400, "text/plain", result == 0 ? "" : "invalid control");
}

void setup() {
  // R6 and R7 are the physical 10-kiloohm pull-ups. INPUT deliberately does
  // not add a second internal pull-up in parallel with either resistor.
  pinMode(BUTTON_LINE_A, INPUT);
  pinMode(BUTTON_LINE_B, INPUT);
  cameraReady = startCamera();
  cardReady = startCard();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.config(IPAddress(192, 168, 4, 2), IPAddress(192, 168, 4, 1),
              IPAddress(255, 255, 255, 0));
  WiFi.begin(CAMBUILD_WIFI_SSID, CAMBUILD_WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(100);
  server.on("/events", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/plain", String(shutterCount) + "," + selectorCount +
                "," + selectorHoldCount);
  });
  server.on("/frame.jpg", HTTP_GET, []() { sendFrame(false); });
  server.on("/capture", HTTP_GET, []() { sendFrame(true); });
  server.on("/capture", HTTP_POST, []() { sendFrame(true); });
  server.on("/latest", HTTP_GET, sendLatestCapture);
  server.on("/files", HTTP_GET, listOutwardFiles);
  server.on("/file", HTTP_GET, sendOutwardFile);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/health", HTTP_GET, []() {
    server.send(200, "application/json", String("{\"camera\":") +
                (cameraReady ? "true" : "false") + ",\"sd\":" +
                (cardReady ? "true" : "false") + "}");
  });
  server.begin();
}

void loop() {
  updateLine(lineA, true); updateLine(lineB, false);
  server.handleClient();
  delay(2);
}
