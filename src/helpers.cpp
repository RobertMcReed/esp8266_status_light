#include <Arduino.h>
#include <ArduinoJson.h>

#include "helpers.h"

String makeErrorJson(String errorMessage) {
  String errorJson = makeSimpleJson("error", errorMessage.c_str());

  Serial.print("[ERROR]: ");
  Serial.println(errorJson);

  return errorJson;
}

String makeSimpleJson(String key, int value) {
  DynamicJsonDocument jDoc(16);
  jDoc[key.c_str()] = value;

  char jChar[128];

  serializeJson(jDoc, jChar);

  return jChar;
}

String makeSimpleJson(String key, String value) {
  DynamicJsonDocument jDoc(16);
  jDoc[key.c_str()] = value.c_str();

  char jChar[128];

  serializeJson(jDoc, jChar);

  return jChar;
}
