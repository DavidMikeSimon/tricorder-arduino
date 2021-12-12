#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2SNoDAC.h>

#include "secrets.h"
#include "sounds/tng_tricorder_scan.wav.h"

#define PISO_SH_LD_PIN 5 // Set high to read from 74HC165
#define POSI_BSRCLR_PIN 2 // Set high to write to 74HC595 shift registers
#define POSI_RCLK_PIN 15 // Positive edge copies 74HC595 shift registers to output

#define WIDTH 240
#define HEIGHT 320

#define CHUNK_SIZE 64

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

WiFiClient client;

AudioFileSourcePROGMEM* audioFile;
AudioGeneratorWAV* wav;
AudioOutputI2SNoDAC* audioOut;

void setup() {
  enableWiFiAtBootTime();

  Serial.begin(9600);
  //Serial.setDebugOutput(true);

  WiFi.persistent(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

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
  spr.setTextColor(TFT_WHITE, TFT_BLUE);
  
  tft.fillScreen(TFT_BLUE);

  {
    TFT_eSprite spr2 = TFT_eSprite(&tft);
    spr2.createSprite(120, 25);
    spr2.setTextColor(TFT_WHITE, TFT_BLUE);
    spr2.fillSprite(TFT_BLUE);
    spr2.drawString("I Love Pip!", 0, 0, 4);
    spr2.pushSprite(60, 250);
  }

  audioFile = new AudioFileSourcePROGMEM();
  audioOut = new AudioOutputI2SNoDAC();
  wav = new AudioGeneratorWAV();

  pinMode(PISO_SH_LD_PIN, OUTPUT);
  digitalWrite(PISO_SH_LD_PIN, LOW);

  pinMode(POSI_BSRCLR_PIN, OUTPUT);
  digitalWrite(POSI_BSRCLR_PIN, LOW);

  pinMode(POSI_RCLK_PIN, OUTPUT);
  digitalWrite(POSI_RCLK_PIN, LOW);

  write595(0b10000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    write595(0b10101);
  }

  Serial.print("Heap space: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();
  Serial.println();
}


void write595(byte value) {
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
  digitalWrite(POSI_BSRCLR_PIN, HIGH);
  SPI.transfer(value);
  SPI.endTransaction();

  digitalWrite(POSI_RCLK_PIN, HIGH);
  delay(1);
  digitalWrite(POSI_RCLK_PIN, LOW);
  delay(1);
  digitalWrite(POSI_BSRCLR_PIN, LOW);
}

byte lastInput = 0;

void loop() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(PISO_SH_LD_PIN, HIGH);
  byte input1 = SPI.transfer(0);
  byte input2 = SPI.transfer(0);
  digitalWrite(PISO_SH_LD_PIN, LOW);
  SPI.endTransaction();

  write595(input2 | 0b10000);

  String buttonStatus = input1 == 0 ? "RDY" : String(input1, BIN);
  spr.fillSprite(TFT_BLUE);
  spr.drawString(buttonStatus, 120, 25, 4);
  spr.pushSprite(0, 150);
  buttonStatus = input2 == 0 ? "RDY" : String(input2, BIN);
  spr.fillSprite(TFT_BLUE);
  spr.drawString(buttonStatus, 120, 25, 4);
  spr.pushSprite(0, 200);

  byte pressed = input2 & ~lastInput;
  if (pressed == 0b1000) {
    setSwitch("switch.corner_lamp", true);
  } else if (pressed == 0b100) {
    setSwitch("switch.corner_lamp", false);
  } else if (pressed == 0b10) {
    write595(0b100000); // Enable reset signal from reed switch
    tft.startWrite();
    tft.writecommand(TFT_SLPIN);
    tft.endWrite();
    ESP.deepSleep(0);
//  } else if (pressed == 0b1) {
//    if (wav->isRunning()) {
//      wav->stop();
//    }
//    audioFile->open(tng_tricorder_scan_wav, sizeof(tng_tricorder_scan_wav));
//    wav->begin(audioFile, audioOut);
  }
  lastInput = input2;

  if (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      Serial.println("WAV done");
    }
  }

  delay(1);
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
