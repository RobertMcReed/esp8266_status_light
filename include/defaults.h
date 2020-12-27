#include <Arduino.h>
#include <ArduinoJson.h>

#include "light.h"

#ifndef DEFAULTS_h
#define DEFAULTS_h

DynamicJsonDocument jsonBody(500);

enum {
  status_free,
  status_busy,
  status_dnd,
  status_unknown,
  status_party,
  status_custom,
};

enum {
  disconnected = 0,
  inConfig = 1,
  connected = 2,
};

const char* STATUSES[] = {
  "Free",
  "Busy",
  "DND",
  "Unknown",
  "Party!"
};

const char* NEO_MODE_NAMES[] = {
  "solid",
  "breath",
  "marquee",
  "theater",
  "rainbow",
  "rainbow_marquee",
  "rainbow_theater"
};

bool USE_WIFI = true;
int wiFiStatus = disconnected;
char customStatus[80];
uint8_t r = 0;
uint8_t g = 255;
uint8_t b = 0;
uint8_t a = 50;

uint8_t _r = 0;
uint8_t _g = 255;
uint8_t _b = 0;

uint8_t MIN = 0;
uint8_t MAX = 255;
uint8_t MAX_A = 150;
uint8_t LOW_A = 25;
uint8_t MED_A = 75;
uint8_t HIGH_A = MAX_A;
uint8_t MAX_SPEED = 5;
uint8_t MIN_SPEED = 1;

uint8_t speed = 3;

byte _lastRand = 0;
bool _resetFlagged = false;

uint8_t neo_mode = off_mode;
uint8_t currentStatus = status_unknown;

#endif
