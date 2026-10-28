#include <Arduino.h>
#include <FS.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <SD_MMC.h>
#include <WebServer.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "soc/gpio_struct.h"
#include "camera_web.h"
#include "config.h"

// FINAL U4: selfie OV2640, LCD1, local website, status LEDs and controller.
constexpr int U3_DATA = 32, U3_SHIFT_CLOCK = 1, U3_UPDATE_CLOCK = 3;
constexpr int LCD_DC_PIN = 13;
constexpr uint32_t POLL_MS = 70;
constexpr size_t MAX_REMOTE_JPEG = 1024 * 1024;
constexpr int LCD_WIDTH = 320, LCD_HEIGHT = 240;
// Allow up to 20 refreshes per second. Actual speed is set by camera capture,
// JPEG decoding, display transfer and, for U1, the local Wi-Fi hop.
constexpr uint32_t LCD_FRAME_INTERVAL_MS = 50;
constexpr framesize_t STILL_FRAME_SIZE = FRAMESIZE_UXGA;
constexpr int STILL_JPEG_QUALITY = 8;
constexpr framesize_t DEFAULT_PREVIEW_FRAME_SIZE = FRAMESIZE_QVGA;
constexpr int DEFAULT_PREVIEW_JPEG_QUALITY = 20;

constexpr uint8_t RED = 1u << 0, YELLOW = 1u << 1, GREEN = 1u << 2;
constexpr uint8_t BLUE = 1u << 3, PHOTO_RED = 1u << 4;
constexpr uint8_t LCD_CS_HIGH = 1u << 5, LCD_RST_HIGH = 1u << 6;
// LCD DC is direct on GPIO13. U3 Q5 and Q6 provide explicit CS and RST.

constexpr int PWDN_GPIO_NUM = -1, RESET_GPIO_NUM = -1, XCLK_GPIO_NUM = 21;
constexpr int SIOD_GPIO_NUM = 26, SIOC_GPIO_NUM = 27;
constexpr int Y9_GPIO_NUM = 35, Y8_GPIO_NUM = 34, Y7_GPIO_NUM = 39;
constexpr int Y6_GPIO_NUM = 36, Y5_GPIO_NUM = 19, Y4_GPIO_NUM = 18;
constexpr int Y3_GPIO_NUM = 5, Y2_GPIO_NUM = 4;
constexpr int VSYNC_GPIO_NUM = 25, HREF_GPIO_NUM = 23, PCLK_GPIO_NUM = 22;

WebServer server(80);
JPEGDEC jpeg;
uint8_t lights = 0;
uint8_t lcdControl = LCD_CS_HIGH | LCD_RST_HIGH;
uint32_t previous[3] = {};
bool firstEvents = true, selfieSelected = false;
bool cameraReady = false, cardReady = false, peerReady = false;
uint32_t selfieFileNumber = 1, blueUntil = 0, lastPoll = 0;
uint32_t lastLcdFrame = 0;
bool clearLcdBeforeFrame = true;
bool rotateSelfieForLcd = false;
bool doublePreviewForLcd = false;
uint16_t* selfieDecodeBuffer = nullptr;
int selfieDecodeWidth = 0, selfieDecodeHeight = 0;
uint32_t physicalCaptureRevision = 0;
String lastSelfiePath, lastPhysicalCamera = "outward";
String frameSizeName = "qvga";
int jpegQuality = DEFAULT_PREVIEW_JPEG_QUALITY;
bool verticalFlip = false;

String selectedName();
bool remotePost(const String& path, const String& body);
framesize_t parseFrameSize(const String& name);

void writeLights() {
  // LCD1 and U3 share GPIO32 only. U3 uses GPIO1 as its clock while LCD1
  // uses GPIO33, so updating one device does not clock data into the other.
  pinMode(U3_DATA, OUTPUT);
  digitalWrite(U3_UPDATE_CLOCK, LOW);
  shiftOut(U3_DATA, U3_SHIFT_CLOCK, MSBFIRST, lights | lcdControl);
  digitalWrite(U3_UPDATE_CLOCK, HIGH);
}

void setLcdControl(uint8_t mask, bool high) {
  const uint8_t next = high ? static_cast<uint8_t>(lcdControl | mask)
                            : static_cast<uint8_t>(lcdControl & ~mask);
  if (next != lcdControl) {
    lcdControl = next;
    writeLights();
  }
}

void setLight(uint8_t mask, bool on) {
  const uint8_t next = on ? static_cast<uint8_t>(lights | mask)
                          : static_cast<uint8_t>(lights & ~mask);
  if (next != lights) { lights = next; writeLights(); }
}

void pulseBlue(uint32_t duration = 250) {
  setLight(BLUE, true); blueUntil = millis() + duration;
}

void refreshBaseLights() {
  const uint8_t preserved = lights & (BLUE | PHOTO_RED);
  lights = preserved;
  if (cameraReady) lights |= GREEN;
  if (!cardReady) lights |= YELLOW;
  writeLights();
}

inline __attribute__((always_inline)) void lcdBusPause() {
  // About 50 ns at 240 MHz. Two pauses per bit provide real data setup and
  // clock-high hold time without returning to slow microsecond delays.
  asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
}

inline __attribute__((always_inline)) void lcdSendByte(uint8_t value) {
  // GPIO32 and GPIO33 are in the ESP32's upper GPIO register. Direct register
  // writes preserve the proven software-driven wiring while avoiding the very
  // large digitalWrite() cost for every one of a frame's 1,228,800 data bits.
  constexpr uint32_t DATA_MASK = 1u << (32 - 32);
  constexpr uint32_t CLOCK_MASK = 1u << (33 - 32);
  for (int bit = 7; bit >= 0; --bit) {
    GPIO.out1_w1tc.val = CLOCK_MASK;
    if (value & (1u << bit)) GPIO.out1_w1ts.val = DATA_MASK;
    else GPIO.out1_w1tc.val = DATA_MASK;
    lcdBusPause();
    GPIO.out1_w1ts.val = CLOCK_MASK;
    lcdBusPause();
  }
  GPIO.out1_w1tc.val = CLOCK_MASK;
}

void lcdWriteRaw(bool dataMode, const uint8_t* data, size_t length) {
  digitalWrite(LCD_DC_PIN, dataMode ? HIGH : LOW);
  for (size_t index = 0; index < length; ++index) lcdSendByte(data[index]);
}

void lcdCommand(uint8_t command) {
  setLcdControl(LCD_CS_HIGH, false);
  lcdWriteRaw(false, &command, 1);
  setLcdControl(LCD_CS_HIGH, true);
}

void lcdCommandData(uint8_t command, const uint8_t* data, size_t length) {
  setLcdControl(LCD_CS_HIGH, false);
  lcdWriteRaw(false, &command, 1);
  if (length) lcdWriteRaw(true, data, length);
  setLcdControl(LCD_CS_HIGH, true);
}

void lcdSetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  const uint16_t x2 = x + width - 1;
  const uint16_t y2 = y + height - 1;
  const uint8_t columns[] = {static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x),
                             static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2)};
  const uint8_t rows[] = {static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y),
                          static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2)};
  lcdCommandData(0x2A, columns, sizeof(columns));
  lcdCommandData(0x2B, rows, sizeof(rows));
  lcdCommand(0x2C);
}

void lcdPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
               uint16_t* pixels) {
  if (!width || !height || x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
  if (x + width > LCD_WIDTH) width = LCD_WIDTH - x;
  if (y + height > LCD_HEIGHT) height = LCD_HEIGHT - y;
  const uint16_t x2 = x + width - 1;
  const uint16_t y2 = y + height - 1;
  const uint8_t columns[] = {static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x),
                             static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2)};
  const uint8_t rows[] = {static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y),
                          static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2)};
  // Keep CS active for the address and pixel payload. This avoids eight U3
  // updates for every JPEG block and makes a large difference to frame rate.
  setLcdControl(LCD_CS_HIGH, false);
  const uint8_t columnCommand = 0x2A, rowCommand = 0x2B, memoryWrite = 0x2C;
  lcdWriteRaw(false, &columnCommand, 1);
  lcdWriteRaw(true, columns, sizeof(columns));
  lcdWriteRaw(false, &rowCommand, 1);
  lcdWriteRaw(true, rows, sizeof(rows));
  lcdWriteRaw(false, &memoryWrite, 1);
  digitalWrite(LCD_DC_PIN, HIGH);
  const uint8_t* data = reinterpret_cast<uint8_t*>(pixels);
  const size_t length = width * height * 2;
  for (size_t index = 0; index < length; ++index) lcdSendByte(data[index]);
  setLcdControl(LCD_CS_HIGH, true);
}

void lcdFillColor(uint16_t color) {
  static uint16_t line[LCD_WIDTH];
  // ST7789 receives the high RGB565 byte first; ESP32 stores uint16_t values
  // little-endian, so swap the solid color before sending the raw byte buffer.
  const uint16_t wireColor = static_cast<uint16_t>((color << 8) | (color >> 8));
  for (uint16_t x = 0; x < LCD_WIDTH; ++x) line[x] = wireColor;
  for (uint16_t y = 0; y < LCD_HEIGHT; ++y) {
    lcdPixels(0, y, LCD_WIDTH, 1, line);
  }
}

void lcdFillBlack() { lcdFillColor(0x0000); }

// Copy opening.cbs to the root of the microSD card installed in U4.
// The file contains RGB565 frames already arranged in LCD wire-byte order.
constexpr char BOOT_SPLASH_FILE[] = "/opening.cbs";
constexpr int BOOT_SPLASH_MAX_WIDTH = 224;

bool splashReadExact(File& file, uint8_t* dest, size_t count) {
  while (count) {
    const int readCount = file.read(dest, count);
    if (readCount <= 0) return false;
    dest += readCount;
    count -= static_cast<size_t>(readCount);
  }
  return true;
}

uint16_t splashUInt16(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0] |
         (static_cast<uint16_t>(bytes[1]) << 8));
}

bool playBootSplash() {
  if (!cardReady) return false;
  File file = SD_MMC.open(BOOT_SPLASH_FILE, FILE_READ);
  if (!file) return false;

  uint8_t header[12];
  if (!splashReadExact(file, header, sizeof(header)) ||
      memcmp(header, "CBS1", 4) != 0) {
    file.close();
    return false;
  }

  const uint16_t width = splashUInt16(header + 4);
  const uint16_t height = splashUInt16(header + 6);
  const uint16_t frameCount = splashUInt16(header + 8);
  const uint16_t flags = splashUInt16(header + 10);
  const uint64_t expectedSize = static_cast<uint64_t>(12) +
      static_cast<uint64_t>(frameCount) *
      (2ull + 2ull * width * height);
  if (!width || width > BOOT_SPLASH_MAX_WIDTH || !height ||
      height > LCD_HEIGHT || !frameCount || flags != 0 ||
      expectedSize > file.size()) {
    file.close();
    return false;
  }

  static uint16_t line[BOOT_SPLASH_MAX_WIDTH];
  const uint16_t x = (LCD_WIDTH - width) / 2;
  const uint16_t y = (LCD_HEIGHT - height) / 2;
  lcdFillBlack();
  bool complete = true;
  for (uint16_t frame = 0; frame < frameCount && complete; ++frame) {
    uint8_t delayBytes[2];
    if (!splashReadExact(file, delayBytes, sizeof(delayBytes))) {
      complete = false;
      break;
    }
    const uint16_t holdMs = splashUInt16(delayBytes);
    const uint32_t frameStarted = millis();
    for (uint16_t row = 0; row < height; ++row) {
      if (!splashReadExact(file, reinterpret_cast<uint8_t*>(line), width * 2)) {
        complete = false;
        break;
      }
      lcdPixels(x, y + row, width, 1, line);
    }
    const uint32_t renderTime = millis() - frameStarted;
    if (complete && renderTime < holdMs) delay(holdMs - renderTime);
    yield();
  }
  file.close();
  clearLcdBeforeFrame = true;
  lastLcdFrame = 0;
  return complete;
}

void startLcd() {
  // Give the powered panel its own validated active-low reset pulse. U3 keeps
  // chip select high except while an LCD command or pixel block is sent.
  pinMode(LCD_DC_PIN, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(U3_DATA, OUTPUT);
  digitalWrite(LCD_DC_PIN, HIGH);
  digitalWrite(33, LOW);
  setLcdControl(LCD_CS_HIGH, true);
  setLcdControl(LCD_RST_HIGH, true);
  delay(100);
  setLcdControl(LCD_RST_HIGH, false);
  delay(200);
  setLcdControl(LCD_RST_HIGH, true);
  delay(200);
  lcdCommand(0x01);
  delay(150);
  const uint8_t portrait = 0x00;
  lcdCommandData(0x36, &portrait, 1);
  const uint8_t colorMode = 0x05;
  lcdCommandData(0x3A, &colorMode, 1);
  // With RGB channel order established, the six-colour hardware check showed
  // a pure complementary palette. This panel therefore requires INVON while
  // receiving unchanged RGB565 pixel data.
  lcdCommand(0x21);
  const uint8_t columns[] = {0x00, 0x00, 0x01, 0x3F};
  const uint8_t rows[] = {0x00, 0x00, 0x00, 0xEF};
  lcdCommandData(0x2A, columns, sizeof(columns));
  lcdCommandData(0x2B, rows, sizeof(rows));
  const uint8_t porch[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
  lcdCommandData(0xB2, porch, sizeof(porch));
  const uint8_t gate = 0x35;
  lcdCommandData(0xB7, &gate, 1);
  const uint8_t vcom = 0x1F, lcm = 0x2C, enable = 0x01;
  lcdCommandData(0xBB, &vcom, 1);
  lcdCommandData(0xC0, &lcm, 1);
  lcdCommandData(0xC2, &enable, 1);
  const uint8_t vrh = 0x12, vdv = 0x20, frameRate = 0x0F;
  lcdCommandData(0xC3, &vrh, 1);
  lcdCommandData(0xC4, &vdv, 1);
  lcdCommandData(0xC6, &frameRate, 1);
  const uint8_t power[] = {0xA4, 0xA1};
  lcdCommandData(0xD0, power, sizeof(power));
  const uint8_t positiveGamma[] = {0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39,
                                   0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2D};
  const uint8_t negativeGamma[] = {0xD0, 0x08, 0x10, 0x08, 0x06, 0x06, 0x39,
                                   0x44, 0x51, 0x0B, 0x16, 0x14, 0x2F, 0x31};
  lcdCommandData(0xE0, positiveGamma, sizeof(positiveGamma));
  lcdCommandData(0xE1, negativeGamma, sizeof(negativeGamma));
  lcdCommand(0x11);
  delay(120);
  lcdCommand(0x29);
  delay(20);
  // Keep the proven MX + MV landscape direction, but use RGB channel order.
  const uint8_t landscape = 0x60;
  lcdCommandData(0x36, &landscape, 1);
  lcdFillBlack();
}

int drawJpegBlock(JPEGDRAW* draw) {
  if (rotateSelfieForLcd && selfieDecodeBuffer) {
    for (int row = 0; row < draw->iHeight; ++row) {
      memcpy(selfieDecodeBuffer + (draw->y + row) * selfieDecodeWidth + draw->x,
             draw->pPixels + row * draw->iWidth,
             draw->iWidth * sizeof(uint16_t));
    }
    return 1;
  }
  if (doublePreviewForLcd) {
    static uint16_t line[LCD_WIDTH];
    for (int row = 0; row < draw->iHeight; ++row) {
      for (int sourceX = 0; sourceX < draw->iWidth; ++sourceX) {
        const uint16_t pixel = draw->pPixels[row * draw->iWidth + sourceX];
        line[sourceX * 2] = pixel;
        line[sourceX * 2 + 1] = pixel;
      }
      const int outputY = (draw->y + row) * 2;
      lcdPixels(draw->x * 2, outputY, draw->iWidth * 2, 1, line);
      lcdPixels(draw->x * 2, outputY + 1, draw->iWidth * 2, 1, line);
    }
    return 1;
  }
  lcdPixels(draw->x, draw->y, draw->iWidth, draw->iHeight, draw->pPixels);
  return 1;
}

bool drawJpeg(uint8_t* data, size_t length) {
  if (!data || !length || !jpeg.openRAM(data, length, drawJpegBlock)) return false;
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  const int width = jpeg.getWidth();
  const int height = jpeg.getHeight();
  if (rotateSelfieForLcd) {
    if (width != 320 || height != 240) {
      jpeg.close();
      return false;
    }
    if (!selfieDecodeBuffer) {
      selfieDecodeBuffer = static_cast<uint16_t*>(
          ps_malloc(width * height * sizeof(uint16_t)));
    }
    if (!selfieDecodeBuffer) {
      jpeg.close();
      return false;
    }
    if (clearLcdBeforeFrame) {
      lcdFillBlack();
      clearLcdBeforeFrame = false;
    }
    selfieDecodeWidth = width;
    selfieDecodeHeight = height;
    if (!jpeg.decode(0, 0, 0)) {
      jpeg.close();
      return false;
    }
    jpeg.close();

    // Rotate counterclockwise into a portrait image, fill the LCD height,
    // and keep equal black borders at the left and right.
    constexpr int outputWidth = 180;
    constexpr int outputX = (LCD_WIDTH - outputWidth) / 2;
    static uint16_t line[outputWidth];
    for (int outputY = 0; outputY < LCD_HEIGHT; ++outputY) {
      const int rotatedY = outputY * selfieDecodeWidth / LCD_HEIGHT;
      for (int column = 0; column < outputWidth; ++column) {
        const int rotatedX = column * selfieDecodeHeight / outputWidth;
        const int sourceX = selfieDecodeWidth - 1 - rotatedY;
        const int sourceY = rotatedX;
        line[column] = selfieDecodeBuffer[sourceY * selfieDecodeWidth + sourceX];
      }
      lcdPixels(outputX, outputY, outputWidth, 1, line);
    }
    return true;
  }
  int scale = 0;  // zero is JPEGDEC's full-size decode option
  int divisor = 1;
  while ((width / divisor > LCD_WIDTH || height / divisor > LCD_HEIGHT) && divisor < 8) {
    divisor *= 2;
  }
  if (divisor == 2) scale = JPEG_SCALE_HALF;
  else if (divisor == 4) scale = JPEG_SCALE_QUARTER;
  else if (divisor == 8) scale = JPEG_SCALE_EIGHTH;
  const int outputWidth = max(1, width / divisor);
  const int outputHeight = max(1, height / divisor);
  if (outputWidth > LCD_WIDTH || outputHeight > LCD_HEIGHT) {
    jpeg.close();
    return false;
  }
  if (clearLcdBeforeFrame) {
    lcdFillBlack();
    clearLcdBeforeFrame = false;
  }
  doublePreviewForLcd = width == 160 && height == 120;
  const int x = (LCD_WIDTH - outputWidth) / 2;
  const int y = (LCD_HEIGHT - outputHeight) / 2;
  const bool ok = jpeg.decode(doublePreviewForLcd ? 0 : x,
                              doublePreviewForLcd ? 0 : y, scale);
  doublePreviewForLcd = false;
  jpeg.close();
  return ok;
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
  // Allocate maximum-size buffers once, then use QQVGA for the live view.
  // Captures can switch to UXGA without reallocating camera memory.
  c.frame_size = STILL_FRAME_SIZE; c.jpeg_quality = STILL_JPEG_QUALITY; c.fb_count = 2;
  c.grab_mode = CAMERA_GRAB_LATEST; c.fb_location = CAMERA_FB_IN_PSRAM;
  if (!psramFound() || esp_camera_init(&c) != ESP_OK) return false;
  sensor_t* sensor = esp_camera_sensor_get();
  if (!sensor) return false;
  sensor->set_hmirror(sensor, 0);
  sensor->set_vflip(sensor, 0);
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

bool saveSelfie(camera_fb_t* frame, String& path) {
  if (!cardReady || !frame) return false;
  do { path = "/SELF_" + String(selfieFileNumber++) + ".JPG"; }
  while (SD_MMC.exists(path));
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) return false;
  const bool ok = file.write(frame->buf, frame->len) == frame->len;
  file.close();
  if (ok) lastSelfiePath = path;
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
  for (int index = 0; index < 2; ++index) {
    camera_fb_t* queued = esp_camera_fb_get();
    if (!queued) { restorePreviewSettings(); return nullptr; }
    esp_camera_fb_return(queued);
  }
  return esp_camera_fb_get();
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

bool streamFileFast(File& file) {
  static uint8_t buffer[8192];
  WiFiClient client = server.client();
  client.setNoDelay(true);
  while (file.available() && client.connected()) {
    const size_t received = file.read(buffer, sizeof(buffer));
    if (!received) return false;
    size_t offset = 0;
    uint32_t lastProgress = millis();
    while (offset < received && client.connected()) {
      const size_t sent = client.write(buffer + offset, received - offset);
      if (!sent) {
        if (millis() - lastProgress > 4000) return false;
        delay(1);
        continue;
      }
      offset += sent;
      lastProgress = millis();
    }
  }
  return !file.available();
}

void setDownloadDisplay(bool downloading) {
  lcdCommand(downloading ? 0x28 : 0x29);  // display off / display on
  if (!downloading) {
    clearLcdBeforeFrame = true;
    lastLcdFrame = 0;
  }
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

String selectedName() { return selfieSelected ? "selfie" : "outward"; }

bool readRemote(const String& path, uint8_t*& buffer, size_t& length) {
  buffer = nullptr; length = 0;
  HTTPClient http;
  // Keep a missing or delayed outward frame from blocking physical-button
  // polling for several seconds.
  http.setConnectTimeout(300); http.setTimeout(1200);
  if (!http.begin("http://192.168.4.2" + path) || http.GET() != HTTP_CODE_OK) {
    http.end(); return false;
  }
  const int receivedLength = http.getSize();
  if (receivedLength <= 0 || static_cast<size_t>(receivedLength) > MAX_REMOTE_JPEG) {
    http.end(); return false;
  }
  buffer = static_cast<uint8_t*>(ps_malloc(receivedLength));
  if (!buffer) { http.end(); return false; }
  const size_t received = http.getStreamPtr()->readBytes(buffer, receivedLength);
  http.end();
  if (received != static_cast<size_t>(receivedLength)) { free(buffer); buffer = nullptr; return false; }
  length = received;
  return true;
}

void updateLcdFrame() {
  if (millis() - lastLcdFrame < LCD_FRAME_INTERVAL_MS) return;
  lastLcdFrame = millis();

  if (selfieSelected) {
    camera_fb_t* frame = cameraReady ? esp_camera_fb_get() : nullptr;
    if (!frame) return;
    rotateSelfieForLcd = true;
    drawJpeg(frame->buf, frame->len);
    rotateSelfieForLcd = false;
    esp_camera_fb_return(frame);
    return;
  }

  uint8_t* buffer = nullptr;
  size_t length = 0;
  if (!readRemote("/frame.jpg", buffer, length)) return;
  drawJpeg(buffer, length);
  free(buffer);
}

bool remotePost(const String& path, const String& body = "") {
  HTTPClient http;
  http.setConnectTimeout(450); http.setTimeout(5000);
  if (!http.begin("http://192.168.4.2" + path)) return false;
  if (body.length()) http.addHeader("Content-Type", "application/json");
  const int result = http.POST(body);
  http.end();
  return result >= 200 && result < 300;
}

bool readEvents(uint32_t out[3]) {
  HTTPClient http;
  http.setConnectTimeout(150); http.setTimeout(250);
  if (!http.begin("http://192.168.4.2/events") || http.GET() != HTTP_CODE_OK) {
    http.end(); return false;
  }
  String body = http.getString(); http.end();
  for (int i = 0; i < 3; ++i) {
    const int comma = body.indexOf(',');
    const String field = comma < 0 ? body : body.substring(0, comma);
    out[i] = static_cast<uint32_t>(field.toInt());
    if (i < 2 && comma < 0) return false;
    body = comma < 0 ? "" : body.substring(comma + 1);
  }
  return true;
}

bool remoteCardReady() {
  HTTPClient http;
  http.setConnectTimeout(250); http.setTimeout(400);
  if (!http.begin("http://192.168.4.2/health") || http.GET() != HTTP_CODE_OK) {
    http.end(); return false;
  }
  const bool ready = http.getString().indexOf("\"sd\":true") >= 0;
  http.end();
  return ready;
}

void selectCamera(bool selfie) {
  if (selfieSelected == selfie) return;
  selfieSelected = selfie;
  clearLcdBeforeFrame = true;
  refreshBaseLights(); pulseBlue();
}

bool capturePhysical() {
  setLight(PHOTO_RED, true);
  bool ok = false;
  if (selfieSelected) {
    camera_fb_t* frame = cameraReady ? captureMaximumStill() : nullptr;
    String path;
    ok = frame && saveSelfie(frame, path);
    if (frame) esp_camera_fb_return(frame);
    restorePreviewSettings();
  } else {
    ok = remotePost("/capture");
  }
  setLight(PHOTO_RED, false);
  if (ok) {
    lastPhysicalCamera = selfieSelected ? "selfie" : "outward";
    ++physicalCaptureRevision;
    pulseBlue(400);
  } else { setLight(RED, true); setLight(YELLOW, true); }
  return ok;
}

void handlePhysicalEvents() {
  if (millis() - lastPoll < POLL_MS) return;
  lastPoll = millis();
  uint32_t now[3];
  if (!readEvents(now)) { peerReady = false; return; }
  peerReady = true;
  if (firstEvents) { memcpy(previous, now, sizeof(previous)); firstEvents = false; refreshBaseLights(); return; }
  if (now[0] != previous[0]) capturePhysical();
  if (now[1] != previous[1]) selectCamera(!selfieSelected);
  if (now[2] != previous[2]) playBootSplash();
  memcpy(previous, now, sizeof(previous));
}

void sendJpeg(bool capture) {
  const String requested = server.hasArg("camera") ? server.arg("camera") : selectedName();
  if (requested != "selfie" && requested != "outward") {
    server.send(400, "text/plain", "Unknown camera"); return;
  }
  if (capture) setLight(PHOTO_RED, true);
  if (requested == "selfie") {
    camera_fb_t* frame = cameraReady ? (capture ? captureMaximumStill() : esp_camera_fb_get()) : nullptr;
    if (!frame) {
      if (capture) restorePreviewSettings();
      if (capture) setLight(PHOTO_RED, false);
      server.send(503, "text/plain", "Selfie camera unavailable"); return;
    }
    String path;
    const bool saved = capture && saveSelfie(frame, path);
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-CamBuild-Saved", saved ? "1" : "0");
    server.setContentLength(frame->len);
    server.send(200, "image/jpeg", "");
    writeBytes(frame->buf, frame->len);
    esp_camera_fb_return(frame);
    if (capture) restorePreviewSettings();
  } else {
    uint8_t* buffer = nullptr; size_t length = 0;
    if (!readRemote(capture ? "/capture" : "/frame.jpg", buffer, length)) {
      if (capture) setLight(PHOTO_RED, false);
      server.send(503, "text/plain", "Outward camera unavailable"); return;
    }
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(length);
    server.send(200, "image/jpeg", "");
    writeBytes(buffer, length);
    free(buffer);
  }
  if (capture) {
    setLight(PHOTO_RED, false);
    pulseBlue(350);
  }
}

void sendLatestPhysicalCapture() {
  const String requested = server.hasArg("camera") ? server.arg("camera") : lastPhysicalCamera;
  if (requested == "selfie") {
    if (!cardReady || !lastSelfiePath.length() || !SD_MMC.exists(lastSelfiePath)) {
      server.send(404, "text/plain", "No selfie capture yet"); return;
    }
    File file = SD_MMC.open(lastSelfiePath, FILE_READ);
    if (!file) { server.send(500, "text/plain", "Cannot read selfie capture"); return; }
    server.sendHeader("Cache-Control", "no-store");
    server.streamFile(file, "image/jpeg");
    file.close();
    return;
  }
  uint8_t* buffer = nullptr; size_t length = 0;
  if (!readRemote("/latest", buffer, length)) {
    server.send(404, "text/plain", "No outward capture yet"); return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(length);
  server.send(200, "image/jpeg", "");
  writeBytes(buffer, length);
  free(buffer);
}

bool validStoredFileName(String name, const char* prefix) {
  if (name.startsWith("/")) name.remove(0, 1);
  const String expectedPrefix(prefix);
  if (!name.startsWith(expectedPrefix) || !name.endsWith(".JPG") ||
      name.length() <= expectedPrefix.length() + 4) return false;
  for (int i = expectedPrefix.length(); i < name.length() - 4; ++i) {
    if (!isDigit(name[i])) return false;
  }
  return true;
}

void listSelfieFiles() {
  if (!cardReady) { server.send(503, "application/json", "{\"error\":\"card unavailable\"}"); return; }
  File root = SD_MMC.open("/");
  if (!root) { server.send(500, "application/json", "{\"error\":\"cannot open card\"}"); return; }
  String body = "{\"camera\":\"selfie\",\"files\":[";
  bool first = true;
  uint16_t count = 0;
  File file = root.openNextFile();
  while (file && count < 400) {
    String name = file.name();
    if (name.startsWith("/")) name.remove(0, 1);
    if (!file.isDirectory() && validStoredFileName(name, "SELF_")) {
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

void proxyOutwardFile(const String& name, bool inlineView) {
  HTTPClient http;
  http.setConnectTimeout(800); http.setTimeout(15000);
  if (!http.begin("http://192.168.4.2/file?name=" + name)) {
    server.send(503, "text/plain", "Outward file unavailable"); return;
  }
  const int result = http.GET();
  if (result != HTTP_CODE_OK) {
    http.end(); server.send(result == HTTP_CODE_NOT_FOUND ? 404 : 503,
                            "text/plain", "Outward file unavailable"); return;
  }
  const int length = http.getSize();
  server.sendHeader("Cache-Control", "no-store");
  if (!inlineView) server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  if (length >= 0) server.setContentLength(length);
  else server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "image/jpeg", "");
  setDownloadDisplay(true);
  WiFiClient* source = http.getStreamPtr();
  WiFiClient destination = server.client();
  source->setNoDelay(true);
  destination.setNoDelay(true);
  static uint8_t buffer[8192];
  int remaining = length;
  uint32_t lastData = millis();
  while (destination.connected() && http.connected() && (remaining != 0)) {
    const size_t available = source->available();
    if (!available) {
      if (millis() - lastData > 4000) break;
      delay(1); continue;
    }
    const size_t wanted = min(available, sizeof(buffer));
    const size_t received = source->readBytes(buffer, wanted);
    if (!received) break;
    size_t offset = 0;
    while (offset < received && destination.connected()) {
      const size_t sent = destination.write(buffer + offset, received - offset);
      if (!sent) {
        if (millis() - lastData > 4000) break;
        delay(1);
        continue;
      }
      offset += sent;
      lastData = millis();
    }
    if (offset != received) break;
    lastData = millis();
    if (remaining > 0) remaining -= received;
  }
  http.end();
  setDownloadDisplay(false);
}

void listStoredFiles() {
  const String requested = server.arg("camera");
  if (requested == "selfie") { listSelfieFiles(); return; }
  if (requested != "outward") { server.send(400, "text/plain", "Unknown camera"); return; }
  HTTPClient http;
  http.setConnectTimeout(600); http.setTimeout(2500);
  if (!http.begin("http://192.168.4.2/files") || http.GET() != HTTP_CODE_OK) {
    http.end(); server.send(503, "application/json", "{\"error\":\"outward card unavailable\"}"); return;
  }
  const String body = http.getString();
  http.end();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

void sendStoredFile() {
  const String requested = server.arg("camera");
  String name = server.arg("name");
  if (requested == "outward") {
    if (!validStoredFileName(name, "OUT_")) { server.send(400, "text/plain", "Invalid outward filename"); return; }
    if (name.startsWith("/")) name.remove(0, 1);
    proxyOutwardFile(name, server.hasArg("inline"));
    return;
  }
  if (requested != "selfie" || !validStoredFileName(name, "SELF_")) {
    server.send(400, "text/plain", "Invalid selfie filename"); return;
  }
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
  setDownloadDisplay(true);
  streamFileFast(file);
  file.close();
  setDownloadDisplay(false);
}




void handleControl() {
  const String body = server.arg("plain");
  const String camera = jsonString(body, "camera");
  if (camera == "outward") {
    server.send(remotePost("/control", body) ? 204 : 503, "text/plain", "");
    return;
  }
  if (camera != "selfie") { server.send(400, "text/plain", "Unknown camera"); return; }
  sensor_t* sensor = esp_camera_sensor_get();
  const String setting = jsonString(body, "setting");
  int result = -1;
  if (sensor && setting == "vflip") {
    verticalFlip = jsonInt(body, "value", 0) != 0;
    result = sensor->set_vflip(sensor, verticalFlip);
  } else if (sensor && setting == "quality") {
    jpegQuality = constrain(jsonInt(body, "value", 12), 10, 35);
    result = sensor->set_quality(sensor, jpegQuality);
  } else if (sensor && setting == "framesize") {
    frameSizeName = jsonString(body, "value");
    if (frameSizeName != "qqvga" && frameSizeName != "qvga" && frameSizeName != "vga" &&
        frameSizeName != "svga" && frameSizeName != "uxga") frameSizeName = "vga";
    result = sensor->set_framesize(sensor, parseFrameSize(frameSizeName));
  }
  server.send(result == 0 ? 204 : 400, "text/plain", result == 0 ? "" : "Invalid control");
}

void startWebServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8",
                  reinterpret_cast<PGM_P>(CAMERA_WEB_GZIP), CAMERA_WEB_GZIP_SIZE);
  });
  server.on("/api/status", HTTP_GET, []() {
    const bool outwardStorage = peerReady && remoteCardReady();
    const String body = String("{\"activeCamera\":\"") + selectedName() +
      "\",\"connected\":{\"outward\":" + (peerReady ? "true" : "false") +
      ",\"selfie\":" + (cameraReady ? "true" : "false") +
      "},\"settings\":{\"vflip\":" + (verticalFlip ? "true" : "false") +
      ",\"quality\":" + jpegQuality + ",\"framesize\":\"" + frameSizeName +
      "\"},\"storage\":{\"outward\":\"" +
      (outwardStorage ? "ready" : "unavailable") + "\",\"selfie\":\"" +
      (cardReady ? "ready" : "unavailable") + "\"},\"physicalCaptureRevision\":" +
      physicalCaptureRevision + ",\"lastPhysicalCamera\":\"" +
      lastPhysicalCamera + "\"}";
    server.send(200, "application/json", body);
  });
  server.on("/api/camera", HTTP_POST, []() {
    const String camera = jsonString(server.arg("plain"), "camera");
    if (camera != "outward" && camera != "selfie") {
      server.send(400, "text/plain", "Unknown camera"); return;
    }
    selectCamera(camera == "selfie");
    server.send(204, "text/plain", "");
  });
  server.on("/stream", HTTP_GET, []() { sendJpeg(false); });
  server.on("/api/capture", HTTP_GET, []() { sendJpeg(true); });
  server.on("/api/latest", HTTP_GET, sendLatestPhysicalCapture);
  server.on("/api/files", HTTP_GET, listStoredFiles);
  server.on("/api/file", HTTP_GET, sendStoredFile);
  server.on("/api/control", HTTP_POST, handleControl);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void setup() {
  pinMode(U3_DATA, OUTPUT); pinMode(U3_SHIFT_CLOCK, OUTPUT); pinMode(U3_UPDATE_CLOCK, OUTPUT);
  digitalWrite(U3_SHIFT_CLOCK, LOW); digitalWrite(U3_UPDATE_CLOCK, HIGH);
  const uint8_t chase[] = {RED, YELLOW, GREEN, BLUE};
  for (uint8_t light : chase) {
    lights = light;
    writeLights();
    delay(180);
  }
  lights = 0;
  writeLights();
  startLcd();
  cardReady = startCard();
  playBootSplash();  // Missing SD or animation file never blocks normal startup.
  cameraReady = startCamera();
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  WiFi.softAP(CAMBUILD_WIFI_SSID, CAMBUILD_WIFI_PASSWORD);
  startWebServer();
  refreshBaseLights();
}

void loop() {
  server.handleClient();
  handlePhysicalEvents();
  updateLcdFrame();
  // A display transfer can exceed the normal poll interval, so check the
  // persistent U1 event counters again immediately after every frame.
  handlePhysicalEvents();
  if (blueUntil && static_cast<int32_t>(millis() - blueUntil) >= 0) {
    blueUntil = 0; setLight(BLUE, false); refreshBaseLights();
  }
  delay(2);
}
