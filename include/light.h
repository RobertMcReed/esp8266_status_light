#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#ifndef NEOPIXEL_LIGHT_h
#define NEOPIXEL_LIGHT_h

enum NEO_MODES {
  solid_mode,
  breath_mode,
  marquee_mode,
  theater_mode,
  rainbow_mode,
  rainbow_marquee_mode,
  rainbow_theater_mode,
  MODE_END,
  off_mode = 10,
};

void neoSetup();
int getLastNeoMode();
void neoLoop(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t neo_mode, uint8_t neo_speed);

void breathe();
void marquee();
void theater();
void rainbow();
void rainbowMarquee();
void rainbowTheater();

void solidRed();
void solidBlue();
void solidOrange();

void clearStrip();

#endif
