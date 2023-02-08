#include "./tones.h"
#include "SSD1306Wire.h"
#include "arduinoFFT.h"

#define MICROPHONE_INPUT_PIN 15
#define LED_PIN 18
#define I2C_SDA 21
#define I2C_SCL 22

#define SAMPLES 512
#define SAMPLING_FREQUENCY 40000 // Hz
#define TONES_BATCH_SIZE 5
#define TONES_BATCH_MINIMUM 4
#define TONES_BATCH_TOLERANCE 0
#define SONG_TOLERANCE 1
#define SONG_TONE_WAIT 2200 // ms
#define LED_ON_TIME 200     // ms
#define LED_OFF_TIME 800    // ms

#define SONG_LENGTH 19
int song[SONG_LENGTH] = {C5, E5, G5, C5, E5, G5, E5, D5, E5, F5, D5, E5, D5, E5, F5, D5, E5, D5, C5};
int songProgress = 0;
unsigned int relativeProgress = 0;
unsigned long nextToneDeadline = 0;

unsigned int sampling_period_us;
unsigned long newTime, oldTime;

double vReal[SAMPLES];
double vImg[SAMPLES];

char toneString[4] = {0, 0, 0, 0};
int tonesBatch[TONES_BATCH_SIZE];

int ledStatus = 0;
int nextLedChange = 0;

arduinoFFT FFT = arduinoFFT();
SSD1306Wire display(0x3c, I2C_SDA, I2C_SCL);

void setup() {
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  pinMode(LED_PIN, OUTPUT);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
}

void loop() {
  if (songProgress >= SONG_LENGTH) {
    display.clear();
    display.display();
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, ledStatus);
    for (int actToneIdx = 0; actToneIdx < TONES_BATCH_SIZE; actToneIdx++) {
      for (int i = 0; i < SAMPLES; i++) {
        newTime = micros() - oldTime;
        oldTime = newTime;
        vReal[i] = analogRead(MICROPHONE_INPUT_PIN);
        vImg[i] = 0;
        while (micros() < (newTime + sampling_period_us)) {
        }
      }
      FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
      FFT.Compute(vReal, vImg, SAMPLES, FFT_FORWARD);
      FFT.ComplexToMagnitude(vReal, vImg, SAMPLES);
      double freq = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY) / 2.0;

      tonesBatch[actToneIdx] = freqToTone(freq);
    }
    int tone = selectToneFromBatch();
    if (tone >= 0) {
      toneToString(tone, toneString);
      if (nextToneDeadline < millis()) {
        songProgress = 0;
      }
      if (abs(tone - song[songProgress]) <= SONG_TOLERANCE) {
        songProgress++;
        relativeProgress = (uint)(100.0f * ((float)songProgress) / (SONG_LENGTH));
        nextToneDeadline = millis() + SONG_TONE_WAIT;
      }
    } else {
      toneString[0] = 0;
    }
    updateLED();
    redrawDisplay();
  }
}

int abs(int x) {
  return x < 0 ? -x : x;
}

int selectToneFromBatch() {
  for (int i = 0; i < TONES_BATCH_SIZE; i++) {
    int actCount = 0;
    for (int j = 0; j < TONES_BATCH_SIZE; j++) {
      if (abs(tonesBatch[i] - tonesBatch[j]) <= TONES_BATCH_TOLERANCE)
        actCount++;
    }
    if (actCount >= TONES_BATCH_MINIMUM)
      return tonesBatch[i];
  }
  return -1;
}

void updateLED() {
  if (millis() >= nextLedChange) {
    if (ledStatus == LOW) {
      ledStatus = HIGH;
      nextLedChange = millis() + LED_ON_TIME;
    } else {
      ledStatus = LOW;
      nextLedChange = millis() + LED_OFF_TIME;
    }
  }
}

void redrawDisplay() {
  display.clear();
  display.drawString(50, 20, toneString);
  display.drawProgressBar(0, 50, 125, 10, relativeProgress);
  display.display();
}