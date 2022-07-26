// These are necessary to convince Adafruit_TFTSPI to use DMA
#define ARDUINO_SAMD_ZERO
#define USE_SPI_DMA

#include <Arduino.h>
#include <ArduinoLowPower.h>
#include <SPI.h>
#include <Adafruit_ZeroDMA.h>
#include <Adafruit_ZeroI2S.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFiNINA.h>
#include <math.h>

#define ST7789_PTLAR  0x30
#define ST7789_PTLON  0x12
#define ST7789_SLPIN  0x10
#define ST7789_SLPOUT 0x11

#define PIN_BTN_GEO   PIN_A0
#define PIN_BTN_BIO   PIN_A1
#define PIN_BTN_MET   PIN_A2
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

Adafruit_ST7789 tft = Adafruit_ST7789(PIN_TFT_CS, PIN_TFT_DC, -1);
Adafruit_ZeroI2S i2s = Adafruit_ZeroI2S();
WiFiClient client;

#include "menu.h"
#include "secrets.h"
#include "sounds/tng_tricorder_close.wav.h"
#include "sounds/tng_tricorder_open.wav.h"
//#include "sounds/tng_tricorder_scan.wav.h"
//#include "sounds/tng_tricorder_scan_low_beep.wav.h"

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

void setup() {
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  pinMode(PIN_MAGNET, INPUT_PULLUP);
  pinMode(PIN_BTN_GEO, INPUT_PULLUP);
  pinMode(PIN_BTN_MET, INPUT_PULLUP);
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
  const uint8_t partialArea[] = {0, 0, 0, 195};
  tft.sendCommand(ST7789_PTLAR, partialArea, 4);
  tft.sendCommand(ST7789_PTLON);
  tft.setTextWrap(false);

  tft.fillScreen(ST77XX_BLACK);
  setupMainMenu();
  drawDirtyWidgets();

  analogWrite(PIN_TFT_BL, 255);

//  Serial.println("Connecting to WiFi");
//  int status = WL_IDLE_STATUS;
//  while (status != WL_CONNECTED) {
//    status = WiFi.begin(WIFI_SSID, WIFI_PASS);
//
//    if (status == WL_NO_MODULE) {
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
//    Serial.print("WiFi status: ");
//    Serial.println(status);
//  }
//  WiFi.lowPowerMode();

  analogWrite(PIN_LED_ALPHA, 0);
  analogWrite(PIN_LED_BETA, 0);
  analogWrite(PIN_LED_DELTA, 0);
  analogWrite(PIN_LED_GAMMA, 0);

  if (!i2s.begin(I2S_32_BIT, 22050)) {
    Serial.println("Unable to initialize I2S");
  }
  i2s.enableTx();
}


void loop() {
  changeRandomColor();
  checkInput();

  if (millis() > 3000) {
    checkSleep();
  }

  delay(20);
}
