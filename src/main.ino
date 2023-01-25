#include "./tones.h"
#include "arduinoFFT.h"

#define MICROPHONE_INPUT_PIN 15
#define SAMPLES 512
#define SAMPLING_FREQUENCY 40000 // Hz

unsigned int sampling_period_us;
unsigned long newTime, oldTime;

double vReal[SAMPLES];
double vImg[SAMPLES];

char toneString[4] = {0, 0, 0, 0};

arduinoFFT FFT = arduinoFFT();

void setup() {
  Serial.begin(115200);
  Serial.println("Board init complete");
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
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

  int tone = freqToTone(freq);

  toneToString(tone, toneString);
  Serial.printf("%s (%lf Hz)\n", toneString, freq);
}