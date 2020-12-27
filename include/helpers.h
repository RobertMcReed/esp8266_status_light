#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef HELPERS_h
#define HELPERS_h

String makeErrorJson(String errorMessage);
String makeSimpleJson(String key, int value);
String makeSimpleJson(String key, String value);

#endif
