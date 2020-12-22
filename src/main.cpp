#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266AutoIOT.h>   // https://github.com/RobretMcReed/esp8266AutoIOT.git
#include "html.h"
#include "light.h"

ESP8266AutoIOT app((char*)"esp8266", (char*)"newcouch");

unsigned long rebootAt = 0;

void handleResetWiFi() {
  app.resetCredentials();
}

void handleResetSoft() {
  app.softReset();
}

void handleResetHard() {
  app.softReset();
  rebootAt = millis();
  Serial.println("Rebooting in 3 seconds...");
}

DynamicJsonDocument jsonBody(500);
uint8_t r = 0;
uint8_t g = 255;
uint8_t b = 0;
uint8_t _r = 0;
uint8_t _g = 255;
uint8_t _b = 0;
uint8_t a = 50;
uint8_t MAX = 255;
uint8_t MAX_A = 150;
uint8_t MIN = 0;
uint8_t speed = 3;

enum {
  status_free,
  status_busy,
  status_dnd,
  status_unknown,
  status_party,
  status_custom,
};

char customStatus[80];
const char* STATUSES[] = { "Free", "Busy", "DND", "Unknown", "Party!" };

uint8_t currentStatus = status_unknown;

uint8_t neo_mode = off_mode;

const char* NEO_MODE_NAMES[] = { "solid", "breath", "marquee", "theater", "rainbow", "rainbow_marquee", "rainbow_theater" };

String makeSimpleJson(String key, String value) {
  DynamicJsonDocument jDoc(16);
  jDoc[key.c_str()] = value.c_str();

  char jChar[128];

  serializeJson(jDoc, jChar);

  return jChar;
}

String makeSimpleJson(String key, int value) {
  DynamicJsonDocument jDoc(16);
  jDoc[key.c_str()] = value;

  char jChar[128];

  serializeJson(jDoc, jChar);

  return jChar;
}

String makeErrorJson(String errorMessage) {
  String errorJson = makeSimpleJson("error", errorMessage.c_str());

  Serial.print("[ERROR]: ");
  Serial.println(errorJson);

  return errorJson;
}

String getStatus() {
  String status = currentStatus == status_custom ? customStatus : STATUSES[currentStatus];
  
  return status;
}

String getStatusJson() {
  return makeSimpleJson("status", getStatus());
}

String getJsonConfig() {
  const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(4);
  DynamicJsonDocument neoDoc(capacity);
  
  String status = getStatus();
  JsonArray color = neoDoc.createNestedArray("color");
  color.add(r);
  color.add(g);
  color.add(b);
  color.add(a);
  neoDoc["mode_num"] = neo_mode;
  neoDoc["mode"] = (neo_mode < MODE_END) ? NEO_MODE_NAMES[neo_mode] : "off";
  neoDoc["brightness"] = a;
  neoDoc["speed"] = speed;
  neoDoc["status"] = status.c_str();
  char output[256];

  serializeJson(neoDoc, output);

  return output;
}

int getMode(String requestedMode) {
  for (int i = 0; i < MODE_END; i++) {
    if (!strcmp(requestedMode.c_str(), NEO_MODE_NAMES[i])) {
      return i;
    }
  }

  // this really doesn't need to be a special case
  if (!strcmp(requestedMode.c_str(), "off")) {
    return off_mode;
  }

  // return last mode registered if turned back "on"
  if (!strcmp(requestedMode.c_str(), "on")) {
    return getLastNeoMode();
  }

  return -1;
}

bool setMode(String requestedMode) {
  int mode_num = getMode(requestedMode);

  if (mode_num == -1) {
    return false;
  }

  neo_mode = mode_num;
  return true;
}

bool setMode(int requestedMode) {
  if (((requestedMode < MODE_END) && (requestedMode >= 0)) || requestedMode == off_mode) {
    neo_mode = requestedMode;
    return true;
  }

  return false;
}

void ensureStatus() {
  a = max(a, (uint8_t)50);
  if (neo_mode > theater_mode) {
    neo_mode = solid_mode;
  }
}

String setFree() {
  r = 0;
  g = 255;
  b = 0;
  currentStatus = status_free;

  ensureStatus();

  return getJsonConfig();
}

String setBusy() {
  r = 255;
  g = 0;
  b = 255;
  currentStatus = status_busy;

  ensureStatus();

  return getJsonConfig();
}

String setDND() {
  r = 255;
  g = 0;
  b = 0;
  currentStatus = status_dnd;

  ensureStatus();

  return getJsonConfig();
}

String setUnknown() {
  currentStatus = status_unknown;

  return getJsonConfig();
}

String setParty() {
  currentStatus = status_party;
  a = 150;
  neo_mode = rainbow_marquee_mode;
  speed = max((uint8_t)3, speed);

  return getJsonConfig();
}

void verifyStatusIsCorrect(bool colorsChanged) {
  // set status unknown if device is off or colors changed while free/busy/dnd
  // or we just switched out of party mode
  if ((neo_mode == off_mode) || (colorsChanged && (currentStatus < status_unknown)) || ((neo_mode < rainbow_mode) && (currentStatus == status_party))) {
    currentStatus = status_unknown;
    // set party if any rainbow and not custom status
  } else if ((neo_mode > theater_mode) && (currentStatus != status_custom)) {
    // set status to party if in rainbow mode, as long as no custom status is set
    currentStatus = status_party;
  }
}

String handleGetHostname() {
  String hostname = app.getHostname();
  return makeSimpleJson("hostname", hostname);
}

String handleConfigRequest(String body) {
  Serial.print("[REQUEST]: ");
  Serial.println(body);

  char json[body.length() + 1];
  body.toCharArray(json, body.length()+1);
  DeserializationError jsonError = deserializeJson(jsonBody, json);

  if (jsonError) {
    String errorMessage = makeErrorJson(jsonError.c_str());
    return errorMessage;
  }

  if (!(jsonBody.containsKey("mode") || jsonBody.containsKey("mode_num") || jsonBody.containsKey("color") || jsonBody.containsKey("brightness") || jsonBody.containsKey("speed") || jsonBody.containsKey("status"))) {
    String errorMessage = makeErrorJson("mode, mode_num, brightness, color, speed, or status is required.");
    return errorMessage;
  }

  bool success = true;
  if (jsonBody.containsKey("mode")) {
    String requestedMode = jsonBody["mode"];
    success = setMode(requestedMode);
  } else if (jsonBody.containsKey("mode_num")) {
    int requestedMode = jsonBody["mode_num"];
    success = setMode(requestedMode);
  } else if (neo_mode == off_mode) {
    uint8_t last_mode = getLastNeoMode();

    // if we're off but sent a brightness update, go back to last mode
    if (jsonBody.containsKey("brightness")) {
      neo_mode = last_mode;
    } else if (jsonBody.containsKey("color")) {
      // if we're off but sent a color, go back to last mode unless last mode was party, then go to solid
      neo_mode = last_mode < rainbow_mode ? last_mode : solid_mode;
    }
  }

  if (!success) {
    String errorMessage = makeErrorJson("Invalid mode or mode_num");

    return errorMessage;
  }

  uint8_t temp_a = a;
  bool colorChanged = false;
  if (jsonBody.containsKey("color")) {
    r = jsonBody["color"][0];
    g = jsonBody["color"][1];
    b = jsonBody["color"][2];
    temp_a = jsonBody["color"][3];
    colorChanged = true;

    r = max(r, MIN);
    r = min(r, MAX);
    
    g = max(g, MIN);
    g = min(g, MAX);
    
    b = max(b, MIN);
    b = min(b, MAX);

    if (neo_mode > theater_mode) {
      // if we aren't in a color mode revert to solid
      neo_mode = solid_mode;
    }
  }

  if (jsonBody.containsKey("brightness")) {
    temp_a = jsonBody["brightness"];
  }

  if (temp_a >= MIN) {
    a = max(temp_a, MIN);
    a = min(temp_a, MAX_A);
  }

  if (jsonBody.containsKey("speed")) {
    speed = jsonBody["speed"];
    speed = min(speed, (uint8_t)5); // MAX SPEED = 5
    speed = max(speed, (uint8_t)1);
  }

  bool skipVerifyStatus = false;
  if (jsonBody.containsKey("status")) {
    String status = jsonBody["status"];

    // { "Free", "Busy", "Do Not Disturb", "Unknown", "Party!" };
    if (!strcmp(status.c_str(), STATUSES[0])) {
      setFree();
    } else if (!strcmp(status.c_str(), STATUSES[1])) {
      setBusy();
    } else if (!strcmp(status.c_str(), STATUSES[2])) {
      setDND();
    } else if (!strcmp(status.c_str(), STATUSES[3])) {
      skipVerifyStatus = true;
      setUnknown();
    } else if (!strcmp(status.c_str(), STATUSES[4])) {
      setParty();
    } else {
      skipVerifyStatus = true;
      currentStatus = status_custom;
      strcpy(customStatus, status.c_str());
    }

    Serial.print("Setting status to: ");
    Serial.println(getStatus());
  }

  // don't overwrite custom statuses or unknown
  if (!skipVerifyStatus) {
    verifyStatusIsCorrect(colorChanged); // check status against current mode
  }

  String neoSettings = getJsonConfig();
  Serial.print("[RESPONSE]: ");
  Serial.println(neoSettings);

  return neoSettings;
}

String handleSetMode(int newMode) {
  neo_mode = newMode;
  verifyStatusIsCorrect(false);
  return getJsonConfig();
}

String handleSetOff() {
  return handleSetMode(off_mode);
}

String handleSetOn() {
  return handleSetMode(getLastNeoMode());
}

String handleSwitch() {
  if (neo_mode == off_mode)
  {
    return handleSetOn();
  }
  else
  {
    return handleSetOff();
  }
}

String handleSetSolid() {
  return handleSetMode(solid_mode);
}

String handleSetBreath() {
  return handleSetMode(breath_mode);
}

String handleSetMarquee() {
  return handleSetMode(marquee_mode);
}

String handleSetTheater() {
  return handleSetMode(theater_mode);
}

String handleSetRainbow() {
  return handleSetMode(rainbow_mode);
}

String handleSetRainbowMarquee() {
  return handleSetMode(rainbow_marquee_mode);
}

String handleSetRainbowTheater() {
  return handleSetMode(rainbow_theater_mode);
}

String handleSetNextMode() {
  uint8_t nextMode = neo_mode + 1;

  if (nextMode >= MODE_END) {
    nextMode = 0;
  }
  return handleSetMode(nextMode);
}

String handleSetPrevMode() {
  int prevMode = neo_mode - 1; // specifically an int

  if ((prevMode >= MODE_END) || (prevMode < 0)) {
    prevMode = MODE_END - 1;
  }
  return handleSetMode(prevMode);
}

String handleSetSpeedSlow() {
  speed = 1;
  return getJsonConfig();
}

String handleSetSpeedNormal() {
  speed = 3;
  return getJsonConfig();
}

String handleSetSpeedFast() {
  speed = 5;
  return getJsonConfig();
}

String ensureBrightness(uint8_t newA) {
  a = newA;

  if (neo_mode == off_mode) {
    neo_mode = getLastNeoMode();
  }
  return getJsonConfig();
}

String handleSetBrightnessLow() {
  return ensureBrightness(25);
}

String handleSetBrightnessMedium() {
  return ensureBrightness(50);
}

String handleSetBrightnessHigh() {
  return ensureBrightness(MAX_A);
}

enum {
  disconnected = 0,
  inConfig = 1,
  connected = 2,
};

int wiFiStatus = disconnected;

void handleDisconnected() {
  noWiFiSolidOrange();
  wiFiStatus = disconnected;
  _r = 240;
  _g = 100;
  _b = 0;
}

void handleInConfig() {
  inConfigSolidBlue();
  wiFiStatus = inConfig;
  _r = 0;
  _g = 100;
  _b = 255;
}

void handleConnected() {
  wiFiStatus = connected;
  clearStrip();
}

void setup() {
  // WiFi information is printed, so it's a good idea to start the Serial monitor
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  delay(1000);
  neoSetup();

  // To change the WiFi your device connects to, reset WiFi Credentials
  // app.resetCredentials();
  // app.softReset();
  // app.hardReset();
  app.enableCors();

  app.disableLED();
  app.root(HTML);

  // return the current hostname
  app.get("/hostname", handleGetHostname);

  // power
  app.get("/power/on", handleSetOn); // turn lights on (revert to previously known state)
  app.get("/power/off", handleSetOff); // turn lights off
  app.get("/power/toggle", handleSwitch); // turn lights off or revert to previous state

  // status
  app.get("/status", getStatusJson); // get current status
  app.get("/status/free", setFree); // mark self as free (green)
  app.get("/status/busy", setBusy); // mark self as busy (yellow)
  app.get("/status/dnd", setDND); // mark self as dnd (red)
  app.get("/status/unknown", setUnknown); // mark self as unknown (status only)

  // config
  app.post("/config", handleConfigRequest); // set any setting manually
  app.get("/config/state", getJsonConfig); // get full config

  // config shorthand - mode
  app.get("/config/mode/next", handleSetNextMode); // change to next mode
  app.get("/config/mode/prev", handleSetPrevMode); // change to next mode
  app.get("/config/mode/solid", handleSetSolid); // change to solid mode
  app.get("/config/mode/breath", handleSetBreath); // change to breath mode
  app.get("/config/mode/marquee", handleSetMarquee); // change to marquee mode
  app.get("/config/mode/theater", handleSetTheater); // change to theater mode
  app.get("/config/mode/rainbow", handleSetRainbow); // change to rainbow mode
  app.get("/config/mode/rainbow/marquee", handleSetRainbowMarquee); // change to rainbow marquee mode
  app.get("/config/mode/marquee/rainbow", handleSetRainbowMarquee); // change to rainbow marquee mode
  app.get("/config/mode/rainbow/theater", handleSetRainbowTheater); // change to theater rainbow mode
  app.get("/config/mode/theater/rainbow", handleSetRainbowTheater); // change to theater rainbow mode

  // config shorthand - speed
  app.get("/config/speed/slow", handleSetSpeedSlow);
  app.get("/config/speed/medium", handleSetSpeedNormal);
  app.get("/config/speed/med", handleSetSpeedNormal);
  app.get("/config/speed/fast", handleSetSpeedFast);

  // config shorthand - brightness
  app.get("/config/brightness/low", handleSetBrightnessLow);
  app.get("/config/brightness/medium", handleSetBrightnessMedium);
  app.get("/config/brightness/med", handleSetBrightnessMedium);
  app.get("/config/brightness/high", handleSetBrightnessHigh);

  // resets
  app.get("/reset/wifi", handleResetWiFi); // reset WiFi credentials
  app.get("/reset/soft", handleResetSoft); // reset WiFi credentials and clear storage
  app.get("/reset/hard", handleResetHard); // reset WiFi, clear storage, and reboot device (glitchy)

  
  inConfigSolidBlue(); // this is actually indicating that we've not yet connected to WiFi
  app.setOnDisconnect(handleDisconnected);
  app.setOnEnterConfig(handleInConfig);
  app.setOnConnect(handleConnected);

  app.begin();
  clearStrip();
}

void loop() {
    // You must call app.loop(); to keep the server going...
    app.loop();
    if (wiFiStatus == connected)
    {
      neoLoop(r, g, b, a, neo_mode, speed);
    }
    else
    {
      neoLoop(_r, _g, _b, 100, breath_mode, 3);
    }

    if (rebootAt && (millis() - rebootAt > 3000)) {
      ESP.restart();
    }
}
