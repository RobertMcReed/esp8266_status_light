#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "light.h"

#define LED_PIN  0 //D3
#define LED_COUNT 8
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t last_r = 0;
uint8_t last_g = 0;
uint8_t last_b = 0;
uint8_t last_a = 0;
uint8_t last_speed = 3;
int numStripPixels = strip.numPixels();
uint32_t stripColor = strip.Color(0, 0, 0);
bool in_delay = false;
uint8_t last_neo_mode = off_mode;
unsigned long neo_step_i = 0;
unsigned long neo_step_i_max = 0;
unsigned long neo_mode_delay = 10;
unsigned long last_delay_millis = 0;
int neo_step_j = 0;
int neo_step_j_max = 0;
int neo_step_k = 0;
int neo_step_k_max = 0;
int firstPixelHue = 0;
uint8_t minBreathBrightness = 5;
uint8_t BREATH_SPEED = 25; // larger number makes it slower, smaller number makes it faster. 25 is good
uint8_t MAX_ALPHA = 150;
bool _neo_off = false;

void neoSetup() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void updateValues(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t neo_mode) {
  if (r != last_r)
  {
    last_r = r;
  }
  if (g != last_g)
  {
    last_g = g;
  }
  if (b != last_b)
  {
    last_b = b;
  }
  if (a != last_a)
  {
    last_a = a;
  }
  if (neo_mode != last_neo_mode)
  {
    last_neo_mode = neo_mode;
  }
}

bool colorsChanged(uint8_t r, uint8_t g, uint8_t b) {
  return (!(r == last_r && g == last_g && b == last_b));
}

bool handleColorChange(uint8_t r, uint8_t g, uint8_t b) {
  if (colorsChanged(r, g, b)) {
    Serial.print("Colors change from: [");
    Serial.print(last_r);
    Serial.print(", ");
    Serial.print(last_g);
    Serial.print(", ");
    Serial.print(last_b);
    Serial.print("] to [");
    Serial.print(r);
    Serial.print(", ");
    Serial.print(g);
    Serial.print(", ");
    Serial.print(b);
    Serial.println("]");
    stripColor = strip.Color(r, g, b);
    return true;
  }

  return false;
}

bool handleBrightnessChange(uint8_t a, uint8_t neo_mode) {
  uint8_t alpha = min(a, MAX_ALPHA);

  if (alpha != last_a) {
    Serial.print("Alpha changed from ");
    Serial.print(last_a);
    Serial.print(" to ");
    Serial.println(alpha);

    if (neo_mode == breath_mode) {
      neo_step_i_max = (alpha - minBreathBrightness) * 2;
      neo_mode_delay = 50;
    } else {
      strip.setBrightness(alpha);
    }

    return true;
  }

  return false;
}

void handleResetNeoStep(uint8_t neo_mode) {
  if (neo_mode != last_neo_mode)
  {
    neo_step_i = 0; // reset step for animation change
    in_delay = false;
    Serial.print("neo_mode changed from ");
    Serial.print(last_neo_mode);
    Serial.print(" to ");
    Serial.println(neo_mode);
    
    if (neo_mode == breath_mode)
    {
      neo_step_i_max = (last_a - minBreathBrightness) * 2;
      neo_mode_delay = 50;
    }
    else if (neo_mode == marquee_mode)
    {
      neo_step_i_max = numStripPixels * 2;
      neo_mode_delay = 100;
    }
    else if (neo_mode == rainbow_marquee_mode)
    {
      neo_step_i_max = 5*65536;
      neo_step_j_max = numStripPixels;
      neo_mode_delay = 10;
    }
    else if (neo_mode == rainbow_mode)
    {
      neo_step_i_max = 65536;
      neo_mode_delay = 100;
    }
    else if (neo_mode == theater_mode)
    {
      neo_step_i_max = 10;
      neo_step_j_max = 3;
      neo_step_k_max = numStripPixels;
      neo_mode_delay = 100;
    }
    else if (neo_mode == rainbow_theater_mode)
    {
      neo_step_i_max = 30;
      neo_step_j_max = 3;
      neo_step_k_max = numStripPixels;
      neo_mode_delay = 100;
    }
  }
}

unsigned long getDelay(unsigned long currentDelay) {
  switch (last_speed)
  {
  case 1:
    return currentDelay * 3;
  case 2:
    return currentDelay * 2;
  case 4:
    return currentDelay / 2;
  case 5:
    return currentDelay / 3;
  default:
    return currentDelay;
  }
}

bool delayIsActive(uint8_t neoMode) {
  if (in_delay)
  {
    unsigned long currentMillis = millis();
    // delay is up, or mode is changed, so clear the delay either way
    unsigned long currentModeDelay = neo_mode_delay;

    // scale the delay so that the breath cycle is roughly the same duration regardless of the brightness
    if (neoMode == breath_mode) {
      int divisor = ((last_a - minBreathBrightness) / BREATH_SPEED);
      divisor = max(1, divisor); // ensure we don't have a 0 divisor
      currentModeDelay = neo_mode_delay / divisor;
    }

    unsigned long speedAdjustedDelay = getDelay(currentModeDelay);

    if ((currentMillis - last_delay_millis > speedAdjustedDelay) || (last_neo_mode != neoMode))
    {
      in_delay = false;
    }
  }

  return in_delay;
}

void beginDelay() {
  last_delay_millis = millis();
  in_delay = true;
}

// get last mode or solid (if last was off)
int getLastNeoMode() {
  return last_neo_mode == off_mode ? solid_mode : last_neo_mode;
}

void neoLoop(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t neo_mode, uint8_t neo_speed) {
  bool neoModeChanged = (neo_mode != last_neo_mode);

  if (neo_mode == off_mode) {
    if (_neo_off) {
      return;
    }
    strip.clear();
    strip.show();
    _neo_off = true;
    return;
  } else if (_neo_off) {
    _neo_off = false;
    neoModeChanged = true;
  }

  bool brightnessChanged = handleBrightnessChange(a, neo_mode); // set brightness if it has changed (if not breath mode)
  bool colorChanged = handleColorChange(r, g, b); // update stored strip color if it has changed
  handleResetNeoStep(neo_mode); // reset the defaults if the neo_mode changed, or set step to 0 if greater than neo_step_i_max

  if (last_speed != neo_speed) {
    Serial.print("Speed changed from ");
    Serial.print(last_speed);
    Serial.print(" to ");
    Serial.println(neo_speed);

    last_speed = neo_speed;
  }
  
  // if currently delaying, keep delaying (unless mode changed)
  if (delayIsActive(neo_mode)) {
    return;
  }

  if (neo_mode == solid_mode)
  {
    // only update if the mode, color, or brightness has changed 
    if (colorChanged || brightnessChanged || neoModeChanged) {
      strip.fill(stripColor);
      strip.show();
    } 
  }
  else if (neo_mode == breath_mode)
  {
    breathe();
  }
  else if (neo_mode == marquee_mode)
  {
    marquee();
  }
  else if (neo_mode == rainbow_marquee_mode)
  {
    rainbowMarquee();
  }
  else if (neo_mode == rainbow_mode)
  {
    rainbow();
  }
  else if (neo_mode == theater_mode)
  {
    theater();
  }
  else if (neo_mode == rainbow_theater_mode)
  {
    rainbowTheater();
  }

  updateValues(r, g, b, a, neo_mode); // store changed values and increment neo_step_i
}

void breathe() {
  uint8_t newBrightness;

  if (neo_step_i * 2 >= neo_step_i_max) {
    // we are above the halfway point, so need to start decreasing the value
    newBrightness = neo_step_i_max - neo_step_i;
  } else {
    // we are still below the halfway point, so are still increasing the brightness
    newBrightness = neo_step_i;
  }

  newBrightness += minBreathBrightness; // pad for min of minBreathBrightness
  strip.setBrightness(newBrightness);
  strip.fill(stripColor);
  strip.show();
  beginDelay();

  neo_step_i++;

  if (neo_step_i >= neo_step_i_max) {
    neo_step_i = 0;
  }
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void marquee() {
  if (neo_step_i == 0)
  {
    strip.clear();
  }

  if (neo_step_i * 2 >= neo_step_i_max) {
    strip.setPixelColor(neo_step_i % (neo_step_i_max / 2), strip.Color(0,0,0));
  } else {
    strip.setPixelColor(neo_step_i, stripColor);  //  Set pixel's color (in RAM)
  }

  strip.show();                          //  Update strip to match
  beginDelay();                           //  Pause for a moment
  neo_step_i++;

  if (neo_step_i >= neo_step_i_max) {
    neo_step_i = 0;
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theater() {
  if (neo_step_k >= neo_step_k_max) { // increment j, reset k to new j, clear strip
    neo_step_j++;
    neo_step_k = neo_step_j;
    strip.show();
    beginDelay();
    strip.clear(); // clear on increment j
  }

  if (neo_step_j >= neo_step_j_max) { // increment i, reset j and k
    neo_step_i++;
    neo_step_j = 0;
    neo_step_k = 0;
  }

  if (neo_step_i >= neo_step_i_max) { //reset everything
    neo_step_i = 0;
    neo_step_j = 0;
    neo_step_k = 0;
  }

  strip.setPixelColor(neo_step_k, stripColor);
  neo_step_k += 3;
}

void rainbow() {
  if (neo_step_i >= neo_step_i_max) { // reset everything
    neo_step_i = 0;
  }

  int pixelHue = neo_step_i + 65536L;

  uint32_t color = strip.gamma32(strip.ColorHSV(pixelHue));
  strip.fill(color);
  neo_step_i += 256;
  strip.show();
  beginDelay();
}

// Rainbow cycle along whole strip.
void rainbowMarquee() {
  // Hue of first pixel (neo_step_j) runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  if (neo_step_j >= neo_step_j_max) { // increment i and reset j
    neo_step_i += 256;
    neo_step_j = 0;
    strip.show(); // Update strip with new contents after each full loop of neo_step_j
    beginDelay();  // Pause for a moment
  }

  if (neo_step_i >= neo_step_i_max) { // reset everything
    neo_step_i = 0;
    neo_step_j = 0;
  }

  // For each pixel in strip...
  // Offset pixel hue by an amount to make one full revolution of the
  // color wheel (range of 65536) along the length of the strip (neo_step_j)
  // (numStripPixels steps):
  int pixelHue = neo_step_i + (neo_step_j * 65536L / numStripPixels);
  // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
  // optionally add saturation and value (brightness) (each 0 to 255).
  // Here we're using just the single-argument hue variant. The result
  // is passed through strip.gamma32() to provide 'truer' colors
  // before assigning to each pixel:
  int pixelNum = numStripPixels - neo_step_j;
  strip.setPixelColor(pixelNum, strip.gamma32(strip.ColorHSV(pixelHue)));
  neo_step_j++;
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void rainbowTheater() {
  // this fires every 3 steps
  if (neo_step_k >= neo_step_k_max) { // increment j, reset k to new j, clear strip
    neo_step_j++;
    neo_step_k = neo_step_j;
    strip.show();
    beginDelay();
    firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    // putting this here is equivalent to placing it at the beginning of the first nested for loop
    strip.clear(); // clear on increment j
  }

  // this fires every 30 * 3 steps
  if (neo_step_j >= neo_step_j_max) { // increment i, reset j and k
    neo_step_i++;
    neo_step_j = 0;
    neo_step_k = 0;
  }

  // this fires every 30 * 3 * numStripPixels steps
  if (neo_step_i >= neo_step_i_max) { //reset everything
    neo_step_i = 0;
    neo_step_j = 0;
    neo_step_k = 0;
    firstPixelHue = 0;     // First pixel starts at red (hue 0)
  }

  int hue   = firstPixelHue + neo_step_k * 65536L / numStripPixels;
  uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
  strip.setPixelColor(neo_step_k, color);
  neo_step_k += 3;
}

void noWiFiSolidOrange() {
  strip.setBrightness(100);
  strip.fill(strip.Color(240, 100, 0));
  strip.show();
}

void inConfigSolidBlue() {
  strip.fill(strip.Color(0, 100, 255));
  strip.setBrightness(100);
  strip.show();
}

void clearStrip() {
  strip.clear();
  strip.show();
}
