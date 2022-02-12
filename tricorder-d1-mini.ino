#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

#include "secrets.h"
#include "sounds/tng_tricorder_close.wav.h"
#include "sounds/tng_tricorder_open.wav.h"
#include "sounds/tng_tricorder_scan.wav.h"
#include "sounds/tng_tricorder_scan_low_beep.wav.h"

#define PAR_SELECT_PIN 5 // Set high to write to 74HC595 and read from 74HC165

#define WIDTH 240
#define HEIGHT 320

#define CHUNK_SIZE 64

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

WiFiClient client;

AudioFileSourcePROGMEM* audioFile;
AudioGeneratorWAV* wav;
AudioOutputI2S* audioOut;

byte parallelOutput = 0;
byte parallelInput1 = 0;
byte parallelInput2 = 0;

void readWriteParallel() {
  digitalWrite(PAR_SELECT_PIN, HIGH);

  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
  // The 74HC165 shifts the first bit _after_ the first clock cycle,
  // so the first bit we read is useless. Here we are just discarding
  // the highest-order bit of each byte, but we could get them if we wanted
  // by looking at the lowest-order bit of the following byte.
  parallelInput1 = SPI.transfer(0) >> 1;
  parallelInput2 = SPI.transfer(parallelOutput) >> 1;
  SPI.endTransaction();

  digitalWrite(PAR_SELECT_PIN, LOW);
}

void setSwitch(const char* entityName, bool targetState) {
  char reqBodyBuffer[256];
  int reqBodyLen = sprintf(
                     reqBodyBuffer,
                     "{\"entity_id\":\"%s\"}\r\n\r\n",
                     entityName
                   );

  if (!client.connect(HOME_ASSISTANT_HOST, HOME_ASSISTANT_PORT)) {
    return;
  }

  client.printf(
    "POST /api/services/switch/turn_%s HTTP/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %u\r\n"
    "Authorization: Bearer %s\r\n"
    "\r\n",
    targetState ? "on" : "off",
    reqBodyLen,
    HOME_ASSISTANT_API_KEY
  );
  client.write(reqBodyBuffer, reqBodyLen);
  client.stop();
}

void setup() {
  enableWiFiAtBootTime();

  pinMode(PAR_SELECT_PIN, OUTPUT);
  digitalWrite(PAR_SELECT_PIN, LOW);

  audioFile = new AudioFileSourcePROGMEM();
  audioOut = new AudioOutputI2S();
  wav = new AudioGeneratorWAV();

  Serial.begin(9600);
  Serial.setDebugOutput(true);

//  audioFile->open(tng_tricorder_open_wav, sizeof(tng_tricorder_open_wav));
//  wav->begin(audioFile, audioOut);
//  wav->loop();

  WiFi.persistent(true);
  //WiFi.begin(WIFI_SSID, WIFI_PASS);

  tft.init();
  tft.setRotation(2);

  tft.startWrite();
  tft.writecommand(ST7789_PTLAR);
  tft.writedata(0);
  tft.writedata(0);
  tft.writedata(0);
  tft.writedata(240);
  tft.writecommand(ST7789_PTLON);
  tft.endWrite();

  spr.createSprite(240, 50);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.fillScreen(TFT_BLACK);

  parallelOutput = 0b10101;
  readWriteParallel();

  {
    TFT_eSprite spr2 = TFT_eSprite(&tft);
    spr2.createSprite(120, 25);
    spr2.setTextColor(TFT_WHITE, TFT_BLACK);
    spr2.fillSprite(TFT_BLACK);
    spr2.drawString("I Love Pip!", 0, 0, 4);
    spr2.pushSprite(60, 250);
  }

  while (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
    }
    delay(1);
  }
  
  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(200);
  //}

  //  Serial.print("Heap space: ");
  //  Serial.println(ESP.getFreeHeap());
  //  Serial.println();
  //  Serial.println();
}

byte lastInput = 0;

void loop() {
  parallelOutput = lastInput | 0b10000;
  readWriteParallel();

  String buttonStatus = parallelInput1 == 0 ? "RDY" : String(parallelInput1, BIN);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(buttonStatus, 120, 25, 4);
  spr.pushSprite(0, 150);
  buttonStatus = parallelInput2 == 0 ? "RDY" : String(parallelInput2, BIN);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(buttonStatus, 120, 25, 4);
  spr.pushSprite(0, 200);

  byte pressed = parallelInput2 & ~lastInput;
  if (pressed != 0) {
    Serial.println("PRESSED");
    Serial.println(String(pressed, BIN));
  }
  if (pressed == 0b1000) {
    //setSwitch("switch.corner_lamp", true);
  } else if (pressed == 0b100) {
    //setSwitch("switch.corner_lamp", false);
    if (!wav->isRunning()) {
      audioFile->open(tng_tricorder_scan_wav, sizeof(tng_tricorder_scan_wav));
      wav->begin(audioFile, audioOut);
    }
  } else if (pressed == 0b10) {
    if (!wav->isRunning()) {
      audioFile->open(tng_tricorder_scan_wav, sizeof(tng_tricorder_scan_wav));
      wav->begin(audioFile, audioOut);
    }
  } else if (parallelInput2 == 0b0) {
    if (wav->isRunning()) {
      wav->stop();
    }
    audioFile->open(tng_tricorder_close_wav, sizeof(tng_tricorder_close_wav));
    wav->begin(audioFile, audioOut);
    while (wav->isRunning()) {
      if (!wav->loop()) {
        wav->stop();
      }
      delay(1);
    }

    readWriteParallel();
    tft.startWrite();
    tft.writecommand(TFT_SLPIN);
    tft.endWrite();
    //ESP.deepSleep(0);
  }

  if (parallelInput1 == 0b1) {
    //    if (wav->isRunning()) {
    //      wav->stop();
    //    }
    if (!wav->isRunning()) {
      audioFile->open(tng_tricorder_scan_low_beep_wav, sizeof(tng_tricorder_scan_low_beep_wav));
      wav->begin(audioFile, audioOut);
    }
  }

  lastInput = parallelInput2;

  if (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
    }
  }

  delay(1);
}
