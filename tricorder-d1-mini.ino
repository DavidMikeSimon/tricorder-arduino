#include <SPI.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>

#include "secrets.h"

#define PISO_SH_LD_PIN 5 // Set high to read from 74HC165
#define POSI_BSRCLR_PIN 2 // Set high to write to 74HC595 shift registers
#define POSI_RCLK_PIN 16 // Positive edge copies 74HC595 shift registers to output

#define WIDTH 240
#define HEIGHT 320

TFT_eSPI tft = TFT_eSPI();           // TFT object
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite object
TFT_eSprite spr2 = TFT_eSprite(&tft); // Sprite object

void setup() {
  pinMode(PISO_SH_LD_PIN, OUTPUT);
  digitalWrite(PISO_SH_LD_PIN, LOW);

  pinMode(POSI_BSRCLR_PIN, OUTPUT);
  digitalWrite(POSI_BSRCLR_PIN, LOW);

  pinMode(POSI_RCLK_PIN, OUTPUT);
  digitalWrite(POSI_RCLK_PIN, LOW);

  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  tft.init();
  tft.setRotation(2);

  spr.createSprite(240, 50);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLUE);

  spr2.createSprite(240, 50);
  spr2.setTextDatum(MC_DATUM);
  spr2.setTextColor(TFT_WHITE, TFT_BLUE);
  spr2.fillSprite(TFT_BLUE);
  spr2.drawString("I Love Pip!", 120, 25, 4);

  tft.fillScreen(TFT_BLUE);
  spr2.pushSprite(0, 250);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }

  Serial.println();
  Serial.println();
}

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

  String buttonStatus = String(input, BIN);
  spr.fillSprite(TFT_BLUE);
  spr.drawString(buttonStatus, 120, 25, 4);

  spr.pushSprite(0, 150);

  delay(50);
}
