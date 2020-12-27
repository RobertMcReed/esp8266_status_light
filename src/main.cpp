#include <Arduino.h>
#include <ArduinoJson.h>
#include <EasierButton.h>   // https://github.com/RobretMcReed/EasierButton.git
#include <ESP8266AutoIOT.h>   // https://github.com/RobretMcReed/ESP8266AutoIOT.git

#include "html.h"
#include "light.h"
#include "helpers.h"
#include "defaults.h"

EasierButton btn(D0, false);
ESP8266AutoIOT app((char*)"esp8266", (char*)"newcouch");

// utility
void handleReboot() {
  _resetFlagged = true;
  solidOrange();
  Serial.println(F("[INFO] Reboot pending in 5 seconds!"));
}

void handleResetAllSettings() {
  app.resetAllSettings(true);
}

int getModeNumFromModeName(String requestedMode) {
  if (!strcmp(requestedMode.c_str(), "off")) {
    return off_mode;
  }

  // return last mode registered if turned back "on"
  if (!strcmp(requestedMode.c_str(), "on")) {
    return getLastNeoMode();
  }

  for (int i = 0; i < MODE_END; i++) {
    if (!strcmp(requestedMode.c_str(), NEO_MODE_NAMES[i])) {
      return i;
    }
  }

  return -1;
}

void ensureStatusMatchesMode(bool colorsChanged) {
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

bool setMode(int requestedMode) {
  if (((requestedMode < MODE_END) && (requestedMode >= 0)) || requestedMode == off_mode) {
    neo_mode = requestedMode;
    return true;
  }

  return false;
}

bool setModeByName(String requestedMode) {
  int mode_num = getModeNumFromModeName(requestedMode);

  bool success = setMode(mode_num);

  return success;
}

void setModeSafe(int newMode) {
  setMode(newMode);
  ensureStatusMatchesMode(false);
}

void enforceColorMode() {
  a = max(a, (uint8_t)50);
  if (neo_mode > theater_mode) {
    neo_mode = solid_mode;
  }
}

// getters
String getStatusAsString() {
  String status = currentStatus == status_custom ? customStatus : STATUSES[currentStatus];
  
  return status;
}

int getNextMode() {
  uint8_t nextMode = neo_mode + 1;

  if (nextMode >= MODE_END) {
    nextMode = 0;
  }

  return nextMode;
}

// getters - route handlers
String getStatusAsJson() {
  return makeSimpleJson("status", getStatusAsString());
}

String getConfigAsJson() {
  const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(4);
  DynamicJsonDocument neoDoc(capacity);
  
  String status = getStatusAsString();
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

// setters
void setNextLightStyle() {
  Serial.println("Next light style");
  int nextMode = getNextMode();
  if (neo_mode <= theater_mode && nextMode > theater_mode) {
      nextMode = solid_mode;
  } else if (neo_mode > theater_mode && neo_mode < MODE_END && nextMode < rainbow_mode) {
    nextMode = rainbow_mode;
  }

  neo_mode = nextMode;
  a = max(a, (uint8_t)15);
}

void setFree() {
  r = 0;
  g = 255;
  b = 0;
  currentStatus = status_free;

  enforceColorMode();
}

void setBusy() {
  r = 255;
  g = 0;
  b = 255;
  currentStatus = status_busy;

  enforceColorMode();
}

void setDND() {
  r = 255;
  g = 0;
  b = 0;
  currentStatus = status_dnd;

  enforceColorMode();
}

void setUnknown() {
  currentStatus = status_unknown;
}

void setParty() {
  Serial.println("Set party");
  currentStatus = status_party;
  a = max(a, MED_A);
  neo_mode = rainbow_marquee_mode;
  speed = max((uint8_t)3, speed);
}

void setNextStatus() {
  Serial.println("Next status");
  if (currentStatus == status_free) {
    setBusy();
  } else if (currentStatus == status_busy) {
    setDND();
  } else {
    setFree();
  }
}

void setNextMode() {
  setModeSafe(getNextMode());
}

void setOffMode() {
  Serial.println("Setting off");
  setModeSafe(off_mode);
}

void setRandomColor() {
  Serial.println("Random color");
  enforceColorMode();

  byte num = rand() % 255;

  // ensure the color is different enough
  while (
    abs(num - _lastRand) < 40 
      || (num < 20 && _lastRand > 235)
      || (_lastRand < 20 && num > 235)
  ) {
    num = rand() % 255;
  }

  _lastRand = num;
  r = wheel_r(num & 255);
  g = wheel_g(num & 255);
  b = wheel_b(num & 255);
}

void setSpeedLow() {
  speed = 1;
}

void setSpeedMed() {
  speed = 3;
}

void setSpeedHigh() {
  speed = 5;
}

void setNextSpeed() {
  Serial.println("Next speed");
  if (++speed > 5) {
    speed = 1;
  }
}

void setBrightness(uint8_t newA) {
  a = min(newA, MAX_A);
  a = max(newA, MIN);

  if (neo_mode == off_mode) {
    neo_mode = getLastNeoMode();
  }
}

void setBrightnessLow() {
  setBrightness(LOW_A);
}

void setBrightnessMed() {
  setBrightness(MED_A);
}

void setBrightnessHigh() {
  setBrightness(MAX_A);
}

void setNextBrightness() {
  Serial.println("Next brightness");
  if (a < LOW_A) {
    setBrightnessLow();
  } else if (a < MED_A) {
    setBrightnessMed();
  } else if (a < MAX_A) {
    setBrightnessHigh();
  } else {
    setBrightnessLow();
  }
}

String setModeSafeAndGetJson(int newMode) {
  setModeSafe(newMode);
  return getConfigAsJson();
}

// setters - route handlers
String handleGetHostnameRequest() {
  String hostname = app.getHostname();
  return makeSimpleJson("hostname", hostname);
}

String handleSetConfigRequest(String body) {
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
    success = setModeByName(requestedMode);
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
    colorChanged = true;
    r = jsonBody["color"][0];
    g = jsonBody["color"][1];
    b = jsonBody["color"][2];
    temp_a = jsonBody["color"][3];

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
    speed = min(speed, MAX_SPEED);
    speed = max(speed, MIN_SPEED);
  }

  bool skipEnsureStatus = false;
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
      skipEnsureStatus = true;
      setUnknown();
    } else if (!strcmp(status.c_str(), STATUSES[4])) {
      setParty();
    } else {
      skipEnsureStatus = true;
      currentStatus = status_custom;
      strcpy(customStatus, status.c_str());
    }

    Serial.print("Setting status to: ");
    Serial.println(getStatusAsString());
  }

  // don't overwrite custom statuses or unknown
  if (!skipEnsureStatus) {
    ensureStatusMatchesMode(colorChanged);
  }

  String neoSettings = getConfigAsJson();
  Serial.print("[RESPONSE]: ");
  Serial.println(neoSettings);

  return neoSettings;
}

// setters - route handlers - status setters
String handleSetFreeRequest() {
  setFree();
  return getConfigAsJson();
}

String handleSetBusyRequest() {
  setBusy();
  return getConfigAsJson();
}

String handleSetDNDRequest() {
  setDND();
  return getConfigAsJson();
}

String handleSetUnknownRequest() {
  setUnknown();
  return getConfigAsJson();
}

String handleSetPartyRequest() {
  setParty();
  return getConfigAsJson();
}

// setters - route handlers - mode setters
String handleSetOffRequest() {
  return setModeSafeAndGetJson(off_mode);
}

String handleSetOnRequest() {
  return setModeSafeAndGetJson(getLastNeoMode());
}

String handleToggleRequest() {
  if (neo_mode == off_mode)
  {
    return handleSetOnRequest();
  }
  else
  {
    return handleSetOffRequest();
  }
}

String handleSetSolidRequest() {
  return setModeSafeAndGetJson(solid_mode);
}

String handleSetBreathRequest() {
  return setModeSafeAndGetJson(breath_mode);
}

String handleSetMarqueeRequest() {
  return setModeSafeAndGetJson(marquee_mode);
}

String handleSetTheaterRequest() {
  return setModeSafeAndGetJson(theater_mode);
}

String handleSetRainbowRequest() {
  return setModeSafeAndGetJson(rainbow_mode);
}

String handleSetRainbowMarqueeRequest() {
  return setModeSafeAndGetJson(rainbow_marquee_mode);
}

String handleSetRainbowTheaterRequest() {
  return setModeSafeAndGetJson(rainbow_theater_mode);
}

String handleSetNextMode() {
  return setModeSafeAndGetJson(getNextMode());
}

String handleSetPrevMode() {
  int prevMode = neo_mode - 1; // specifically an int

  if ((prevMode >= MODE_END) || (prevMode < 0)) {
    prevMode = MODE_END - 1;
  }

  return setModeSafeAndGetJson(prevMode);
}

String handleSetSpeedLow() {
  setSpeedLow();
  return getConfigAsJson();
}

String handleSetSpeedMed() {
  setSpeedMed();
  return getConfigAsJson();
}

String handleSetSpeedHigh() {
  setSpeedHigh();
  return getConfigAsJson();
}

String handleSetBrightnessLow() {
  setBrightnessLow();

  return getConfigAsJson();
}

String handleSetBrightnessMed() {
  setBrightnessMed();
  return getConfigAsJson();
}

String handleSetBrightnessHigh() {
  setBrightnessHigh();
  return getConfigAsJson();
}

// WiFi Event Handlers

void handleDisconnected() {
  solidOrange();
  wiFiStatus = disconnected;
  _r = 240;
  _g = 100;
  _b = 0;
}

void handleInConfig() {
  solidBlue();
  wiFiStatus = inConfig;
  _r = 0;
  _g = 100;
  _b = 255;
}

void handleConnected() {
  wiFiStatus = connected;
  clearStrip();
}

// setup helpers

void setupButton() {
  btn.setOnSingleClick(setNextStatus);
  btn.setOnDoubleClick(setNextLightStyle);
  btn.setOnTripleClick(setNextBrightness);

  btn.setOnReleasedAfter(500, setNextSpeed);
  btn.setOnHold(1250, setRandomColor);
  btn.setOnHold(2500, setParty);

  btn.setOnHold(3500, setOffMode);
  btn.setOnHold(10000, handleReboot);
}

void setupApp() {
  // setup our app
  solidBlue(); // blue until we connect to WiFi
  app.enableCors();
  app.disableLED();
  app.root(HTML);

  // return the current hostname
  app.get("/hostname", handleGetHostnameRequest);

  // power
  app.get("/power/on", handleSetOnRequest); // turn lights on (revert to previously known state)
  app.get("/power/off", handleSetOffRequest); // turn lights off
  app.get("/power/toggle", handleToggleRequest); // turn lights off or revert to previous state

  // status
  app.get("/status", getStatusAsJson); // get current status
  app.get("/status/free", handleSetFreeRequest); // mark self as free (green)
  app.get("/status/busy", handleSetBusyRequest); // mark self as busy (yellow)
  app.get("/status/dnd", handleSetDNDRequest); // mark self as dnd (red)
  app.get("/status/party", handleSetPartyRequest); // mark self as Party!
  app.get("/status/unknown", handleSetUnknownRequest); // mark self as unknown (status only)

  // config
  app.post("/config", handleSetConfigRequest); // set any setting manually
  app.get("/config/state", getConfigAsJson); // get full config

  // config shorthand - mode
  app.get("/config/mode/next", handleSetNextMode); // change to next mode
  app.get("/config/mode/prev", handleSetPrevMode); // change to next mode
  app.get("/config/mode/solid", handleSetSolidRequest); // change to solid mode
  app.get("/config/mode/breath", handleSetBreathRequest); // change to breath mode
  app.get("/config/mode/marquee", handleSetMarqueeRequest); // change to marquee mode
  app.get("/config/mode/theater", handleSetTheaterRequest); // change to theater mode
  app.get("/config/mode/rainbow", handleSetRainbowRequest); // change to rainbow mode
  app.get("/config/mode/rainbow/marquee", handleSetRainbowMarqueeRequest); // change to rainbow marquee mode
  app.get("/config/mode/marquee/rainbow", handleSetRainbowMarqueeRequest); // change to rainbow marquee mode
  app.get("/config/mode/rainbow/theater", handleSetRainbowTheaterRequest); // change to theater rainbow mode
  app.get("/config/mode/theater/rainbow", handleSetRainbowTheaterRequest); // change to theater rainbow mode

  // config shorthand - speed
  app.get("/config/speed/low", handleSetSpeedLow);
  app.get("/config/speed/medium", handleSetSpeedMed);
  app.get("/config/speed/med", handleSetSpeedMed);
  app.get("/config/speed/high", handleSetSpeedHigh);

  // config shorthand - brightness
  app.get("/config/brightness/low", handleSetBrightnessLow);
  app.get("/config/brightness/medium", handleSetBrightnessMed);
  app.get("/config/brightness/med", handleSetBrightnessMed);
  app.get("/config/brightness/high", handleSetBrightnessHigh);
  
  // setup event listeners
  app.setOnDisconnect(handleDisconnected);
  app.setOnEnterConfig(handleInConfig);
  app.setOnConnect(handleConnected);

  // enter the config portal and block until connected to WiFi
  app.begin();
  clearStrip();
}

// HERE WE GO!

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  delay(1000);

  neoSetup(); // initialize light strip

  bool held = btn.begin(1000);

  if (held) {
    USE_WIFI = false;
    Serial.println(F("[INFO] Button held at boot. Skipping WiFi!"));
    solidOrange(); // acknowledge no WiFi mode

    if (btn.heldFor(4000)) {
      Serial.println(F("[WARNING] __HARD_RESET__ in 5 seconds..."));
      solidRed(); // acknowledge the pending reboot
      Serial.println(F("[INFO] Click in the next 5 seconds to cancel"));
      // escape hatch - click to cancel reboot
      if (!btn.waitForClick(5000)) {
        USE_WIFI = true; // this looks wrong
        // but is actually needed so we call app.loop() below
        // which is required for resetting the board
        handleResetAllSettings(); // trigger the board to reboot
        return;
      } else {
        Serial.println(F("[INFO] Hard Reset aborted."));
      }
    } else {
      // keep orange light on for 2 seconds to show we're not in WiFi mode
      delay(2000);
      Serial.println(F("[INFO] Starting without WiFi!"));
    }
  }

  setupButton();

  if (USE_WIFI) {
    setupApp(); // blocks loop until connected to WiFi
  }
}

void loop() {
  if (_resetFlagged) {
    delay(5000);
    ESP.reset();
    delay(5000);
    return;
  }
  
  bool loopOK = true;

  if (USE_WIFI) {
    if (!app.loop()) {
      return; // this indicates a reboot is pending
    }

    loopOK = wiFiStatus == connected;
  }

  btn.update(); // update button state

  if (loopOK)
  {
    neoLoop(r, g, b, a, neo_mode, speed); // still connected or not using WiFi
  }
  else
  {
    neoLoop(_r, _g, _b, 100, breath_mode, 3); // lost WiFi connection
  }
}
