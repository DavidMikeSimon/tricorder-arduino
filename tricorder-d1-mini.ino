#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>

#include "secrets.h"

#define PISO_SH_LD_PIN 5 // Set high to read from 74HC165
#define POSI_BSRCLR_PIN 2 // Set high to write to 74HC595 shift registers
#define POSI_RCLK_PIN 15 // Positive edge copies 74HC595 shift registers to output

#define WIDTH 240
#define HEIGHT 320

#define CHUNK_SIZE 64

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

WiFiClient client;

void setup() {
  enableWiFiAtBootTime();

  pinMode(PISO_SH_LD_PIN, OUTPUT);
  digitalWrite(PISO_SH_LD_PIN, LOW);

  pinMode(POSI_BSRCLR_PIN, OUTPUT);
  digitalWrite(POSI_BSRCLR_PIN, LOW);

  pinMode(POSI_RCLK_PIN, OUTPUT);
  digitalWrite(POSI_RCLK_PIN, LOW);

  Serial.begin(9600);
  //Serial.setDebugOutput(true);

  WiFi.persistent(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  tft.init();
  tft.setRotation(2);

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

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }

  Serial.print("Heap space: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println();
  Serial.println();
}

byte lastInput = 0;

void loop() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(PISO_SH_LD_PIN, HIGH);
  byte input = SPI.transfer(0);
  digitalWrite(PISO_SH_LD_PIN, LOW);
  SPI.endTransaction();

  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
  digitalWrite(POSI_BSRCLR_PIN, HIGH);
  SPI.transfer(input);
  SPI.endTransaction();

  digitalWrite(POSI_RCLK_PIN, HIGH);
  delay(1);
  digitalWrite(POSI_RCLK_PIN, LOW);
  delay(1);
  digitalWrite(POSI_BSRCLR_PIN, LOW);

  String buttonStatus = input == 0 ? "RDY" : String(input, BIN);
  spr.fillSprite(TFT_BLUE);
  spr.drawString(buttonStatus, 120, 25, 4);

  spr.pushSprite(0, 150);

  byte pressed = input & ~lastInput;
  if (pressed == 0b1000) {
    setSwitch("switch.corner_lamp", true);
  } else if (pressed == 0b100) {
    setSwitch("switch.corner_lamp", false);
  } else if (pressed == 0b10) {
    Serial.print("Before sleep");
    ESP.deepSleep(3e6);
  }
  lastInput = input;

  delay(50);
}

void setSwitch(const char* entityName, bool targetState) {
  char reqBodyBuffer[256];
  int reqBodyLen = sprintf(
    reqBodyBuffer,
    "{\"entity_id\":\"%s\"}\r\n\r\n",
    entityName
  );

  if (!client.connect(HOME_ASSISTANT_HOST, HOME_ASSISTANT_PORT)) {
    Serial.println("Error connecting");
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
