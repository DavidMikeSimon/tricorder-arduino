#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define CLOCK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13

#define PISO_SH_LD_PIN 5

#define SCR_CS_PIN 4
#define SCR_DC_PIN 0
#define SCR_RST_PIN 2

Adafruit_ST7789 tft = Adafruit_ST7789(SCR_CS_PIN, SCR_DC_PIN, SCR_RST_PIN);

void setup() {
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(PISO_SH_LD_PIN, OUTPUT);

  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(PISO_SH_LD_PIN, LOW);

  Serial.begin(9600);

  tft.init(240, 320);
  
  delay(100);
  Serial.println();
  Serial.println();
}

void loop() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE2));
  digitalWrite(PISO_SH_LD_PIN, HIGH);
  byte input = SPI.transfer(0);
  digitalWrite(PISO_SH_LD_PIN, LOW);
  SPI.endTransaction();
  
  Serial.println(input, BIN);

  String buttonStatus = String(input, BIN);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(20, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print(buttonStatus);
  
  delay(500);
}
