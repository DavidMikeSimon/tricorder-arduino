#include <Arduino.h>
#include <ArduinoLowPower.h>
#include <SPI.h>
#include <Adafruit_ZeroI2S.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFiNINA.h>
#include <math.h>

#include "secrets.h"
#include "sounds/tng_tricorder_close.wav.h"
#include "sounds/tng_tricorder_open.wav.h"
//#include "sounds/tng_tricorder_scan.wav.h"
//#include "sounds/tng_tricorder_scan_low_beep.wav.h"

#define ST7789_PTLAR  0x30
#define ST7789_PTLON  0x12
#define ST7789_SLPIN  0x10
#define ST7789_SLPOUT 0x11

#define PIN_BTN_GEO   PIN_A0
#define PIN_BTN_MET   PIN_A1
#define PIN_BTN_BIO   PIN_A2
#define PIN_JOY_UP    PIN_A3
#define PIN_LED_GAMMA PIN_A4
#define PIN_JOY_DOWN  PIN_A5
#define PIN_I2S_DIN   PIN_A6
#define PIN_LED_ALPHA 0
#define PIN_SWITCH    1
#define PIN_I2S_CLK   2
#define PIN_I2S_LRC   3
#define PIN_LED_BETA  4
#define PIN_LED_DELTA 5
#define PIN_JOY_BTN   6
#define PIN_MAGNET    7
#define PIN_TFT_MOSI  8
#define PIN_TFT_SCK   9
#define PIN_TFT_BL    10
#define PIN_TFT_DC    11
#define PIN_TFT_CS    12

#define LCARS_ORANGE    0xFCA6
#define LCARS_BLUE      0x333F
#define LCARS_YELLOW    0xFFF2
#define LCARS_RED       0xD328

Adafruit_ST7789 tft = Adafruit_ST7789(PIN_TFT_CS, PIN_TFT_DC, -1);

// Use default pins in board variant
Adafruit_ZeroI2S i2s = Adafruit_ZeroI2S();

WiFiClient client;

void playAudio16(const uint8_t* buffer, uint32_t length) {
  delay(3);
  for (uint32_t i=0; i<length; i += 2) {
    // Can't seem to run I2S in 16-bit mode, so let's run it in 32-bit mode
    // and transform our 16-bit sample to 32-bit.
    int32_t sample = (buffer[i+1] << 24) | (buffer[i] << 16);
    i2s.write(sample, sample);
  }
}

void checkSleep() {
  int reedSwitchValue = digitalRead(PIN_MAGNET);
  if (reedSwitchValue != 0) {
    return;
  }

  Serial.println("Sleep");
  analogWrite(PIN_TFT_BL, 0);
  playAudio16(tng_tricorder_close_wav, TNG_TRICORDER_CLOSE_WAV_LEN);
  delay(120); // From ST7789 docs, need 120ms between last SLPOUT and SLPIN
  tft.sendCommand(ST7789_SLPIN);
  USBDevice.detach();

  reedSwitchValue = digitalRead(PIN_MAGNET);
  if (reedSwitchValue == 0) {
    LowPower.attachInterruptWakeup(PIN_MAGNET, wakeupInterruptCallback, CHANGE);
    LowPower.sleep();
    detachInterrupt(PIN_MAGNET);
  }

  USBDevice.attach();
  analogWrite(PIN_TFT_BL, 255);
  delay(5); // From ST7789 docs, need 5ms between last SLPIN and SLPOUT
  tft.sendCommand(ST7789_SLPOUT);
  playAudio16(tng_tricorder_open_wav, TNG_TRICORDER_OPEN_WAV_LEN);
}

void wakeupInterruptCallback() {
  // Do nothing
}

void setHassSwitch(const char* entityName, bool targetState) {
  Serial.println("setHassSwitch");
  char reqBodyBuffer[256];
  int reqBodyLen = sprintf(
    reqBodyBuffer,
    "{\"entity_id\":\"%s\"}\r\n\r\n",
    entityName
  );

  char reqHeaderBuffer[512];
  int reqHeaderLen = sprintf(
    reqHeaderBuffer,
    "POST /api/services/switch/turn_%s HTTP/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %u\r\n"
    "Authorization: Bearer %s\r\n"
    "Connection: close\r\n"
    "\r\n",
    targetState ? "on" : "off",
    reqBodyLen,
    HOME_ASSISTANT_API_KEY
  );


  if (!client.connect(HOME_ASSISTANT_HOST, HOME_ASSISTANT_PORT)) {
    Serial.println("Connection failed");
    return;
  }

  client.write(reqHeaderBuffer, reqHeaderLen);
  client.write(reqBodyBuffer, reqBodyLen);
  client.stop();
}

void drawScreen() {
  // Drawable points from 0, 124 to 240, 320

  tft.fillScreen(ST77XX_BLACK);

  lcarsBox(
    0, 124,
    40, 184,
    20, 0,
    1, 0,
    0, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    40, 124,
    140, 135,
    20, 0,
    0, 0,
    -1, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    144, 124,
    240, 135,
    0, 5,
    0, 1,
    0, 1,
    LCARS_RED
  );

  lcarsBox(
    0, 188,
    40, 204,
    0, 0, 0, 0, 0, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    0, 208,
    40, 260,
    20, 0,
    0, 0,
    1, 0,
    LCARS_YELLOW
  );

  lcarsBox(
    40, 254,
    140, 260,
    10, 0,
    -1, 0,
    0, 0,
    LCARS_YELLOW
  );

  lcarsBox(
    144, 254,
    164, 260,
    0, 0, 0, 0, 0, 0,
    LCARS_BLUE
  );

  lcarsBox(
    168, 254,
    240, 260,
    0, 0, 0, 0, 0, 0,
    LCARS_YELLOW
  );

  lcarsBox(
    0, 264,
    40, 284,
    20, 0,
    1, 0,
    0, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    40, 264,
    164, 272,
    10, 0,
    0, 0,
    -1, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    168, 264,
    240, 272,
    0, 0, 0, 0, 0, 0,
    LCARS_ORANGE
  );

  lcarsBox(
    0, 288,
    40, 320,
    0, 0, 0, 0, 0, 0,
    LCARS_ORANGE
  );
}


// Based on Adafruit GFX fillCircleHelper
void lcarsBoxSideHelper(int16_t x0, int16_t y0, int16_t r,
                      int16_t h, bool left,
                      int8_t topMode, int8_t bottomMode,
                      uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;
  int16_t delta =  h - 2 * r - 1;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < (y + 1)) {
      int16_t lineX = left ? x0 - x : x0 + x;
      int16_t lineY0 = topMode == 0 ? y0 - r : (topMode < 0 ? y0 - r*2 + y : y0 - y);
      int16_t lineY1 = bottomMode == 0 ? y0 - r + h : (bottomMode < 0 ? y0 + h - y : y0 + y + delta);
      tft.writeFastVLine(lineX, lineY0, lineY1-lineY0, color);
    }
    if (y != py) {
      int16_t lineX = left ? x0 - py : x0 + py;
      int16_t lineY0 = topMode == 0 ? y0 - r : (topMode < 0 ? y0 - r*2 + px : y0 - px);
      int16_t lineY1 = bottomMode == 0? y0 - r + h : (bottomMode < 0 ? y0 + h - px : y0 + px + delta);
      tft.writeFastVLine(lineX, lineY0, lineY1-lineY0, color);
      py = y;
    }
    px = x;
  }
}

void lcarsBox(
  uint16_t x0, uint16_t y0,
  uint16_t x1, uint16_t y1,
  int16_t r0, int16_t r1,
  int8_t topLeftMode, int8_t topRightMode,
  int8_t bottomLeftMode, int8_t bottomRightMode,
  uint16_t color
) {
  tft.startWrite();
  int w = x1-x0;
  int h = y1-y0;
  lcarsBoxSideHelper(x0 + r0, y0 + r0, r0, h, true, topLeftMode, bottomLeftMode, color);
  tft.writeFillRect(x0 + r0, y0, w - (r0 + r1), h, color);
  lcarsBoxSideHelper(x0 + w - r1 - 1, y0 + r1, r1, h, false, topRightMode, bottomRightMode, color);
  tft.endWrite();
}

void setup() {
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_MAGNET, INPUT_PULLUP);
  pinMode(PIN_BTN_GEO, INPUT);
  pinMode(PIN_BTN_MET, INPUT);
  pinMode(PIN_BTN_BIO, INPUT_PULLUP);
  pinMode(PIN_JOY_UP, INPUT_PULLUP);
  pinMode(PIN_JOY_DOWN, INPUT_PULLUP);
  pinMode(PIN_JOY_BTN, INPUT_PULLUP);

  pinMode(PIN_LED_ALPHA, OUTPUT);
  pinMode(PIN_LED_BETA, OUTPUT);
  pinMode(PIN_LED_DELTA, OUTPUT);
  pinMode(PIN_LED_GAMMA, OUTPUT);
  pinMode(PIN_TFT_BL, OUTPUT);
    
  Serial.begin(9600);
  //while (!Serial) delay(10);

  tft.init(240, 320);
  tft.fillScreen(ST77XX_BLACK);

  analogWrite(PIN_TFT_BL, 255);

//  int status = WL_IDLE_STATUS;
//  while (status != WL_CONNECTED) {
//    status = WiFi.begin(WIFI_SSID, WIFI_PASS);
//
//    if (WiFi.status() == WL_NO_MODULE) {
//      Serial.println("Communication with WiFi module failed!");
//      break;
//    }
//
//    for (int i = 0; i <= 64; ++i) {
//      analogWrite(PIN_LED_ALPHA, i);
//      analogWrite(PIN_LED_BETA, (i+16)%64);
//      analogWrite(PIN_LED_DELTA, (i+32)%64);
//      analogWrite(PIN_LED_GAMMA, (i+48)%64);
//      delay(8);
//    }
//  }
//  WiFi.lowPowerMode();

  Serial.println("OK!");

  analogWrite(PIN_LED_ALPHA, 0);
  analogWrite(PIN_LED_BETA, 0);
  analogWrite(PIN_LED_DELTA, 0);
  analogWrite(PIN_LED_GAMMA, 0);

  if (!i2s.begin(I2S_32_BIT, 22050)) {
    Serial.println("Unable to initialize I2S");
  }
  i2s.enableTx();

  const uint8_t partialArea[] = {0, 0, 0, 195};
  tft.sendCommand(ST7789_PTLAR, partialArea, 4);
  tft.sendCommand(ST7789_PTLON);

  drawScreen();
}

void loop() {
  int geoValue = analogRead(PIN_BTN_GEO);
  int metValue = analogRead(PIN_BTN_MET);
  int bioValue = analogRead(PIN_BTN_BIO);

//  if (metValue < 50) {
//    setHassSwitch("switch.corner_lamp", false);
//  } else if (bioValue < 50) {
//    setHassSwitch("switch.corner_lamp", true);
//  }

  if (millis() > 3000) {
    checkSleep();
  }

  delay(100);
}
