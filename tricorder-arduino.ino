#include <Arduino.h>
#include <ArduinoLowPower.h>
#include <SPI.h>
#include <Adafruit_ZeroI2S.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

//#include "secrets.h"
//#include "sounds/tng_tricorder_close.wav.h"
//#include "sounds/tng_tricorder_open.wav.h"
#include "sounds/tng_tricorder_scan.wav.h"
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

Adafruit_ST7789 tft = Adafruit_ST7789(PIN_TFT_CS, PIN_TFT_DC, -1);

// Use default pins in board variant
Adafruit_ZeroI2S i2s = Adafruit_ZeroI2S();


#define SAMPLERATE_HZ 44100  // The sample rate of the audio.  Higher sample rates have better fidelity,
                             // but these tones are so simple it won't make a difference.  44.1khz is
                             // standard CD quality sound.

#define AMPLITUDE     ((1<<29)-1)   // Set the amplitude of generated waveforms.  This controls how loud
                             // the signals are, and can be any value from 0 to 2**31 - 1.  Start with
                             // a low value to prevent damaging speakers!

#define WAV_SIZE      256    // The size of each generated waveform.  The larger the size the higher
                             // quality the signal.  A size of 256 is more than enough for these simple
                             // waveforms.


// Define the frequency of music notes (from http://www.phy.mtu.edu/~suits/notefreqs.html):
#define C4_HZ      261.63
#define D4_HZ      293.66
#define E4_HZ      329.63
#define F4_HZ      349.23
#define G4_HZ      392.00
#define A4_HZ      440.00
#define B4_HZ      493.88

// Define a C-major scale to play all the notes up and down.
float scale[] = { C4_HZ, D4_HZ, E4_HZ, F4_HZ, G4_HZ, A4_HZ, B4_HZ, A4_HZ, G4_HZ, F4_HZ, E4_HZ, D4_HZ, C4_HZ };

// Store basic waveform in memory.
int32_t sawtooth[WAV_SIZE] = {0};

void generateSawtooth(int32_t amplitude, int32_t* buffer, uint16_t length) {
  // Generate a sawtooth signal that goes from -amplitude/2 to amplitude/2
  // and store it in the provided buffer of size length.
  float delta = float(amplitude)/float(length);
  for (int i=0; i<length; ++i) {
    buffer[i] = -(amplitude/2)+delta*i;
  }
}

void playWave(int32_t* buffer, uint16_t length, float frequency, float seconds) {
  // Play back the provided waveform buffer for the specified
  // amount of seconds.
  // First calculate how many samples need to play back to run
  // for the desired amount of seconds.
  uint32_t iterations = seconds*SAMPLERATE_HZ;
  // Then calculate the 'speed' at which we move through the wave
  // buffer based on the frequency of the tone being played.
  float delta = (frequency*length)/float(SAMPLERATE_HZ);
  // Now loop through all the samples and play them, calculating the
  // position within the wave buffer for each moment in time.
  for (uint32_t i=0; i<iterations; ++i) {
    uint16_t pos = uint32_t(i*delta) % length;
    int32_t sample = buffer[pos];
    // Duplicate the sample so it's sent to both the left and right channel.
    // It appears the order is right channel, left channel if you want to write
    // stereo sound.
    i2s.write(sample, sample);
  }
}

void enterSleep() {
  Serial.println("Sleep");
  analogWrite(PIN_TFT_BL, 0);
  delay(120);
  tft.sendCommand(ST7789_SLPIN);
  USBDevice.detach();
  LowPower.attachInterruptWakeup(PIN_MAGNET, wakeupInterruptCallback, CHANGE);
  LowPower.sleep();

  detachInterrupt(PIN_MAGNET);
  analogWrite(PIN_TFT_BL, 255);
  delay(5);
  tft.sendCommand(ST7789_SLPOUT);
  USBDevice.attach();
}

void wakeupInterruptCallback() {
  // Do nothing
}

//void setSwitch(const char* entityName, bool targetState) {
//  char reqBodyBuffer[256];
//  int reqBodyLen = sprintf(
//                     reqBodyBuffer,
//                     "{\"entity_id\":\"%s\"}\r\n\r\n",
//                     entityName
//                   );
//
//  if (!client.connect(HOME_ASSISTANT_HOST, HOME_ASSISTANT_PORT)) {
//    return;
//  }
//
//  client.printf(
//    "POST /api/services/switch/turn_%s HTTP/1.0\r\n"
//    "Content-Type: application/json\r\n"
//    "Content-Length: %u\r\n"
//    "Authorization: Bearer %s\r\n"
//    "\r\n",
//    targetState ? "on" : "off",
//    reqBodyLen,
//    HOME_ASSISTANT_API_KEY
//  );
//  client.write(reqBodyBuffer, reqBodyLen);
//  client.stop();
//}

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

  for (int c = 0; c < 2; ++c) {
    for (int i = 0; i <= 64; ++i) {
      analogWrite(PIN_LED_ALPHA, i);
      analogWrite(PIN_LED_BETA, (i+16)%64);
      analogWrite(PIN_LED_DELTA, (i+32)%64);
      analogWrite(PIN_LED_GAMMA, (i+48)%64);
      delay(10);
    }
  }  
  Serial.println("OK!");

  analogWrite(PIN_LED_ALPHA, 0);
  analogWrite(PIN_LED_BETA, 0);
  analogWrite(PIN_LED_DELTA, 0);
  analogWrite(PIN_LED_GAMMA, 0);

  if (!i2s.begin(I2S_32_BIT, 44100)) {
    Serial.println("Unable to initialize I2S");
  }
  i2s.enableTx();

  // Generate waveforms.
  generateSawtooth(AMPLITUDE, sawtooth, WAV_SIZE);

  const uint8_t partialArea[] = {0, 0, 0, 195};
  tft.sendCommand(ST7789_PTLAR, partialArea, 4);
  tft.sendCommand(ST7789_PTLON);

//  Serial.println("Sawtooth wave");
//  for (int i=0; i<sizeof(scale)/sizeof(float); ++i) {
//    // Play the note for a quarter of a second.
//    playWave(sawtooth, WAV_SIZE, scale[i], 0.25);
//    // Pause for a tenth of a second between notes.
//    delay(100);
//  }
}

void loop() {
  int value = digitalRead(PIN_SWITCH);

  if (value == 1) {
    tft.fillScreen(ST77XX_GREEN);
  } else {
    tft.fillScreen(ST77XX_RED);
  }

  Serial.print("S:");
  Serial.print(value);

  value = digitalRead(PIN_JOY_UP);
  Serial.print(" U:");
  Serial.print(value);

  value = digitalRead(PIN_JOY_BTN);
  Serial.print(" J:");
  Serial.print(value);

  value = digitalRead(PIN_JOY_DOWN);
  Serial.print(" D:");
  Serial.print(value);

  value = analogRead(PIN_BTN_GEO);
  Serial.print(" G:");
  Serial.print(value);

  value = analogRead(PIN_BTN_MET);
  Serial.print(" M:");
  Serial.print(value);

  value = analogRead(PIN_BTN_BIO);
  Serial.print(" B:");
  Serial.print(value);

  int reedSwitchValue = digitalRead(PIN_MAGNET);
  Serial.print(" R:");
  Serial.print(reedSwitchValue);

  Serial.println();

  if (reedSwitchValue == 0) {
    enterSleep();
  }

  delay(100);
}
