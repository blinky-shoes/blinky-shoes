
// Code for Blinky Shoe v2.4
// Inspired by 'Firewalker' code for Adafruit NeoPixels by Phillip Burgess
// To switch between color modes, turn power on and off within 2 seconds
// Each mode displays arbitrary user-defined colors

#include "Adafruit_NeoPixel_bs24.h"
#include "elapsedMillis_bs24.h"
#include <EEPROM.h>

// List of program mode names. You can add new modes here. Modes are defined in function setPalette().
typedef enum {MODE_MLPONY, MODE_BURN, MODE_WONKA, MODE_PRINPEACH, MODE_MOJITO, MODE_SKYWLKR, MODE_CANDY, MODE_TMNT, MODE_CONST, MODE_BLINK, MODE_RAINBOW} progmode;
#define NUMBEROFMODES 11

// User-defined colors. Add more if desired
const uint8_t PROGMEM black[] = {0, 0, 0};
const uint8_t PROGMEM red[] = {255, 0, 0};
const uint8_t PROGMEM green[] = {0, 255, 0};
const uint8_t PROGMEM blue[] = {0, 0, 255};
const uint8_t PROGMEM yellow[] = {255, 255, 0};
const uint8_t PROGMEM magenta[] = {255, 0, 255};
const uint8_t PROGMEM cyan[] = {0, 255, 255};
const uint8_t PROGMEM white[] = {255, 255, 255};
const uint8_t PROGMEM violet[] = {200, 0, 255};
const uint8_t PROGMEM azure[] = {0, 200, 255};
const uint8_t PROGMEM spring[] = {0, 255, 200};
const uint8_t PROGMEM chartreuse[] = {200, 255, 0};
const uint8_t PROGMEM orange[] = {255, 200, 0};
const uint8_t PROGMEM rose[] = {255, 0, 200};

#define N_LEDS 24 // number of LEDs in left and right strips
#define LED_BRIGHTNESS 127 // 0-255
#define MAXSTEPS 6 // Process (up to) this many concurrent steps
#define TIMESTEP_MICROSECONDS 5000 // waiting time between animation steps and sensor readings
#define BLINK_DELAY_MILLISECONDS 200 // waiting time between animation steps and sensor readings
#define TIMESTEP_GAME_OF_LIFE 2500 // miliseconds between game of life ticks

#define MODEADDRESS 300 // Byte address in EEPROM to use for the mode counter (0-1023).
#define SWITCHADDRESS 101 // Byte address in EEPROM to use for the mode switch flag (0-1023).
#define MODE_SWITCH_TIME 2000 // Number of milliseconds before power cycle doesn't change modes
#define DISPLAY_PALETTE_TIME 2000 // Number of milliseconds that the color palette is displayed

#define XACCEL_PIN A1
#define YACCEL_PIN A0
#define ZACCEL_PIN A5
#define LED_PIN_L 4
#define LED_PIN_R A3

#define V_REF 3.3
#define G_PER_V 3.3 // conversion factor between volts and g-force

#define LIGHT_OFF 0
#define LIGHT_DONT_WRITE 1
#define LIGHT_MAG_WAVES 2
#define LIGHT_CONSTANT_RAINBOW 3
#define LIGHT_BLINK_RAINBOW 4
#define LIGHT_GAME_OF_LIFE 5

#define STATE_ATTRACT 0
#define STATE_WALKING 1
#define STATE_CONSTANT 2
#define STATE_BLINK 3

#define FORWARD 0
#define BACKWARD 1

#define READY 0
#define TRIGGERED 1
#define SETTLING 2

#define NUM_SAMPLES_SETTLING 5
#define TRIGGER_LEVEL 0.2
#define RESET_LEVEL 0.05

#define MULTIPLIER 100
#define MIN_STEP_TIME 200
#define MAX_STEP_MAG 1000
#define MIN_STEP_MAG 200

#define SWITCH_MODES 1
#define DONT_SWITCH_MODES 0

Adafruit_NeoPixel_bs24 stripL = Adafruit_NeoPixel_bs24(N_LEDS, LED_PIN_L, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel_bs24 stripR = Adafruit_NeoPixel_bs24(N_LEDS, LED_PIN_R, NEO_GRB + NEO_KHZ800);

elapsedMillis_bs24 timer;
elapsedMillis_bs24 step_timer;

extern const uint8_t PROGMEM gamma[]; // Gamma correction table for LED brightness (defined at end of code)
extern const uint8_t PROGMEM SINES[]; // Fast 0-255 sine lookup table (defined at end of code)

int
  stepMag[MAXSTEPS], // Magnitude of steps
  stepX[MAXSTEPS], // Position of 'step wave' along strip
  mag[N_LEDS], // Brightness buffer (one side of shoe)
  light_state,
  mode_state,
  mode_state_next;

uint8_t
  triggerState = READY,
  settlingCounter = 0,
  stepNum = 0, // Current step number in stepMag/stepX tables
  color0[3], // current color palette (R,G,B)
  color1[3],
  color2[3],
  color3[3];

boolean
  modeSwitchTimeExceeded = false,
  stepDir[MAXSTEPS];

float
  x, // current x-acceleration
  y, // current y-acceleration
  z, // current z-acceleration
  xJerk, // change in x-acceleration
  yJerk, // change in y-acceleration
  zJerk, // change in z-acceleration
  jerkMag, // sum of squares of xJerk, yJerk, and zJerk
  maxJerk;

unsigned long 
  next_game_of_life_tick_time,
  next_exit_walking_mode_time;
boolean game_of_life_state[N_LEDS];
boolean game_of_life_state_old[N_LEDS];
const boolean GAME_OF_LIFE_EIGHTEEN[] = {false, true, false, false, true, false, false, false};

progmode currentmode;

// Define program modes here. Each mode specifies a palette of four colors.
void setPalette() {
  switch(currentmode) {
    default:
    case MODE_MLPONY:
      memcpy_P(color0, &black, 3);  // "darkest" color (use BLACK for fade-out)
      memcpy_P(color1, &cyan, 3);  // "second-darkest" color
      memcpy_P(color2, &yellow, 3); // "second-brightest" color
      memcpy_P(color3, &magenta, 3); // "brightest" color
      break;
    case MODE_BURN:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &red, 3);
      memcpy_P(color2, &yellow, 3);
      memcpy_P(color3, &white, 3);
      break;
    case MODE_WONKA:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &violet, 3);
      memcpy_P(color2, &green, 3);
      memcpy_P(color3, &yellow, 3);
      break;
    case MODE_PRINPEACH:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &rose, 3);
      memcpy_P(color2, &white, 3);
      memcpy_P(color3, &yellow, 3);
      break;
    case MODE_MOJITO:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &green, 3);
      memcpy_P(color2, &cyan, 3);
      memcpy_P(color3, &white, 3);
      break; 
    case MODE_SKYWLKR:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &cyan, 3);
      memcpy_P(color2, &white, 3);
      memcpy_P(color3, &yellow, 3);
      break;    
    case MODE_CANDY:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &rose, 3);
      memcpy_P(color2, &magenta, 3);
      memcpy_P(color3, &blue, 3);
      break;
    case MODE_TMNT:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &green, 3);
      memcpy_P(color2, &yellow, 3);
      memcpy_P(color3, &orange, 3);
      break;
    case MODE_BLINK:
      mode_state = STATE_BLINK;
      break;
    case MODE_CONST:
      mode_state = STATE_CONSTANT;
      break;
    case MODE_RAINBOW:
      memcpy_P(color0, &black, 3);
      memcpy_P(color1, &white, 3);
      memcpy_P(color2, &black, 3);
      memcpy_P(color3, &white, 3);
      break;
  }
}


void setup() {
  uint8_t modevalue;
  boolean modeSwitch = EEPROM.read(SWITCHADDRESS); // read mode switch from EEPROM to decide whether to switch modes

  EEPROM.write(SWITCHADDRESS, SWITCH_MODES); // write to EEPROM so that we will switch modes next time (if power is interrupted before we change this)
  timer = 0; // when this counter reaches MODE_SWITCH_TIME, we will change the mode switch so that we don't switch modes
  
  modevalue = EEPROM.read(MODEADDRESS); // read mode counter from EEPROM (non-volatile memory)
    
  if (modevalue >= NUMBEROFMODES) modevalue = NUMBEROFMODES; // fix out-of-range mode counter (happens when the program is run for the first time)
  if (modeSwitch == SWITCH_MODES) {
    if (++modevalue >= NUMBEROFMODES) modevalue = 0; // increment the mode counter
    EEPROM.write(MODEADDRESS, modevalue);  
  }
  currentmode = (progmode)modevalue;

  mode_state = STATE_ATTRACT;
  setPalette();
  
  stripL.begin();
  stripR.begin();
  memset(stepMag, 0, sizeof(stepMag));
  memset(stepX, 0, sizeof(stepX));

  if (currentmode != MODE_CONST && currentmode != MODE_BLINK) {
    if (modeSwitch == SWITCH_MODES) { // if we are switching modes, display the color palette
      displayPalette();
    } else { // if we are not switching modes, trigger a step animation
      stepMag[0] = MAX_STEP_MAG;
      stepX[0] = -80;
      stepDir[0] = FORWARD;
      stepNum = 1;
    }
  }

  // IMPORTANT! The analog reference pin (AREF) is tied directly to 3.3V (UCAP).
  // If you don't set the analog reference to external, then AREF is connected to
  // the internal 5V voltage reference by default. This will cause a short circuit
  // anytime you do an analog read, potentially damaging the microcontroller.
  analogReference(EXTERNAL); 
  pinMode(XACCEL_PIN, INPUT);
  pinMode(YACCEL_PIN, INPUT);
  pinMode(ZACCEL_PIN, INPUT);

  mode_state_next = mode_state;
  resetGameOfLife();
  light_state = LIGHT_OFF;
  readAccel();
}

void loop() {  
  if (modeSwitchTimeExceeded == false) {
    if (timer > MODE_SWITCH_TIME) { // if it has been a long enough time since the program started...
      modeSwitchTimeExceeded = true;
      EEPROM.write(SWITCHADDRESS, DONT_SWITCH_MODES); // change the mode switch so that we don't switch modes next time
    }
  }

  serviceModeStateMachine();
  serviceLightStateMachine();

  delayMicroseconds(TIMESTEP_MICROSECONDS);  
}

void displayPalette() { // All 4 colors for the current mode are displayed using this function.
  uint8_t r, g, b, i;
  for (i=0; i<N_LEDS; i++) {
    r = rValue(i*1000/N_LEDS);
    g = gValue(i*1000/N_LEDS);
    b = bValue(i*1000/N_LEDS);
    stripL.setPixelColor(i, r, g, b);
    stripR.setPixelColor(i, r, g, b);
  }
  stripL.show();
  stripR.show();
  delay(DISPLAY_PALETTE_TIME);
  for(i=0; i<N_LEDS; i++) {
    stripL.setPixelColor(i, 0, 0, 0);
    stripR.setPixelColor(i, 0, 0, 0);
  }
  stripL.show();
  stripR.show();
}

void stepCalculation() {
  uint8_t i, j;
  int mx1, m;

  readAccel(); // read the acceleration and calculate the jerk
  
  switch(triggerState) { // depending on the trigger state, we have different behavior:
    case READY:
      if ((jerkMag > TRIGGER_LEVEL) && (step_timer > MIN_STEP_TIME)) { // trigger a step if the jerk exceeds the trigger level and there is enough time since the last step.
        maxJerk = jerkMag; // keep a running tally of the maximum jerk encountered during this step.
        stepMag[stepNum] = getMag(maxJerk); // set the step magnitude according to the maximum jerk (this is updated if maxJerk changes).
        triggerState = TRIGGERED;
        if (xJerk > 0) stepDir[stepNum] = FORWARD; // if the vertical jerk is upwards, the animation moves forward.
        if (xJerk < 0) stepDir[stepNum] = BACKWARD; // if the vertical jerk is downwards, the animation moves backward.
        stepX[stepNum] = -80; // the wave starts just out of range.
        step_timer = 0; // reset the timer to prevent steps from being triggered too close to each other.
        break;
      }
      break; // if the jerk does not exceed the trigger level or there hasn't been enough time, do nothing.
    case TRIGGERED:
      mode_state_next = STATE_WALKING;
      next_exit_walking_mode_time = timer + 10 * TIMESTEP_GAME_OF_LIFE;
      if (jerkMag < RESET_LEVEL) { // if the jerk decreases below the reset level, begin settling.
        triggerState = SETTLING;
        break;
      }
      if (jerkMag > maxJerk) { // if the jerk is still increasing, update the step magnitude using the new value of the maximum jerk.
        maxJerk = jerkMag;
        stepMag[stepNum] = getMag(maxJerk);
      }
      break; // if the jerk decreases but doesn't decrease below the reset level, do nothing.
    case SETTLING:
      if (jerkMag > TRIGGER_LEVEL) { // if the jerk exceeds the trigger level, re-trigger.
        if (jerkMag > maxJerk) { // if the jerk has also exceeded its previous maximum value, update the step magnitude.
          maxJerk = jerkMag;
          stepMag[stepNum] = getMag(maxJerk);
        }
        triggerState = TRIGGERED;
        break;
      } else if (jerkMag < RESET_LEVEL) { // if the jerk has stayed below the reset level, increase the settling counter.
        settlingCounter++;
        if (settlingCounter == NUM_SAMPLES_SETTLING) { // if the settling counter reaches a certain value, the trigger becomes ready again.
          settlingCounter = 0;
          triggerState = READY;
          maxJerk = 0;
          if (++stepNum >= MAXSTEPS) stepNum = 0; // Increment stepNum counter
        }
        break;
      } else { // if the jerk is below the trigger level but above the reset level, reset the settling counter to 0.
        settlingCounter = 0;
        break;
      }
      break;
  } 

  // Render a 'brightness map' for all steps in flight. It's like a grayscale image; there's no color yet, just intensities.
  memset(mag, 0, sizeof(mag)); // Clear magnitude buffer
  for(i=0; i<MAXSTEPS; i++) { // For each step...
    if(stepMag[i] <= 0) continue; // Skip if inactive
    for(j=0; j<N_LEDS; j++) { // For each LED...
      // Each step has sort of a 'wave' that's part of the animation, moving from heel to toe if the direction is forward
      // and from toe to heel if the direction is backward. The wave position has sub-pixel resolution (4X), and is up to 80 units (20 pixels) long.
      mx1 = (j << 2) - stepX[i]; // Position of LED along wave
      if((mx1 <= 0) || (mx1 >= 80)) continue; // Out of range
      if(mx1 > 64) { // Rising edge of wave; ramp up fast (4 px)
        m = ((long)stepMag[i] * (long)(80 - mx1)) >> 4;
      } else { // Falling edge of wave; fade slow (16 px)
        m = ((long)stepMag[i] * (long)mx1) >> 6;
      }
      if(stepDir[i] == FORWARD) mag[j] += m; // Add magnitude to buffered sum for forward step
      if(stepDir[i] == BACKWARD) mag[N_LEDS-j-1] += m; // Add magnitude to buffered sum for backward step
    }
    stepX[i]++; // Update position of step wave
    if(stepX[i] >= (80 + (N_LEDS << 2)))
      stepMag[i] = 0; // Off end; disable step wave
    else
      stepMag[i] = ((long)stepMag[i] * 127L) >> 7; // Fade
  }
}

void serviceModeStateMachine() {
  switch (mode_state) {
    case STATE_ATTRACT:
      stepCalculation();
      light_state = LIGHT_GAME_OF_LIFE;
      break;
    
    case STATE_WALKING:
      stepCalculation();
      light_state = LIGHT_MAG_WAVES;
      if (timer > next_exit_walking_mode_time) {
        mode_state_next = STATE_ATTRACT;
      }
      if (mode_state_next == STATE_ATTRACT) {
        resetGameOfLife();
      }
      break;

    case STATE_BLINK:
      light_state = LIGHT_BLINK_RAINBOW;
      break;

    case STATE_CONSTANT:
      light_state = LIGHT_CONSTANT_RAINBOW;
      break;
  }
  mode_state = mode_state_next;
}

void serviceLightStateMachine() {
  uint8_t i, j;
  uint8_t r, g, b, dim;
  uint32_t c;
  long level;
  j = pgm_read_byte(&SINES[(timer / 64) % 255]);
  switch (light_state) {
    case LIGHT_DONT_WRITE:
      break;

    case LIGHT_MAG_WAVES:
      stripL.setBrightness(LED_BRIGHTNESS);
      stripR.setBrightness(LED_BRIGHTNESS);
      // Now the grayscale magnitude buffer is remapped to color for the LEDs. The colors are drawn from the color palette defined in the setPalette function.
      for(i=0; i<N_LEDS; i++) { // For each LED...
        level = mag[i]; // Pixel magnitude (brightness)

        if (currentmode != MODE_RAINBOW) {
          r = rValue(level); // use helper functions rValue, gValue, and bValue to compute colors from the pixel magnitude
          g = gValue(level);
          b = bValue(level);
        } else {
          c = Wheel(((i * 256 / stripL.numPixels()) + j) & 255);
          r = (uint8_t)(c >> 16);
          g = (uint8_t)(c >>  8);
          b = (uint8_t)c;
          
          r = (r * min(mag[i] >> 2, 255)) >> 8;
          g = (g * min(mag[i] >> 2, 255)) >> 8;
          b = (b * min(mag[i] >> 2, 255)) >> 8;      
        }
        
        stripL.setPixelColor(i, r, g, b);
        stripR.setPixelColor(i, r, g, b);
      }
      break;

    case LIGHT_CONSTANT_RAINBOW:
      stripL.setBrightness(LED_BRIGHTNESS);
      stripR.setBrightness(LED_BRIGHTNESS);
      for(i=0; i<N_LEDS; i++) { // For each LED...
        c = Wheel(((i * 256 / stripL.numPixels()) + j) & 255);
        r = (uint8_t)(c >> 16);
        g = (uint8_t)(c >>  8);
        b = (uint8_t)c;
  
        stripL.setPixelColor(i, r, g, b);
        stripR.setPixelColor(i, r, g, b);
      }
      break;

    case LIGHT_BLINK_RAINBOW:
      if ((timer >> 8) % 2 == 0) {
        stripL.setBrightness(LED_BRIGHTNESS);
        stripR.setBrightness(LED_BRIGHTNESS);
      } else {
        stripL.setBrightness(0);
        stripR.setBrightness(0);
      }
      for(i=0; i<N_LEDS; i++) { // For each LED...
        c = Wheel(((i * 256 / stripL.numPixels()) + j) & 255);
        r = (uint8_t)(c >> 16);
        g = (uint8_t)(c >>  8);
        b = (uint8_t)c;

        stripL.setPixelColor(i, r, g, b);
        stripR.setPixelColor(i, r, g, b);
      }
      break;
    
    case LIGHT_OFF:
      for(i=0; i<N_LEDS; i++) {
        stripL.setPixelColor(i, 0, 0, 0);
        stripR.setPixelColor(i, 0, 0, 0);
      }
      break;
           
    case LIGHT_GAME_OF_LIFE:
      serviceGameOfLife();
      float pct_remaining_in_step = ((float) min((unsigned long) next_game_of_life_tick_time - timer, TIMESTEP_GAME_OF_LIFE)) / TIMESTEP_GAME_OF_LIFE;

      for(i=0; i<N_LEDS; i++) {
        if (game_of_life_state_old[i] && !game_of_life_state[i]) {
          // * 127 to only scale through half the sine cycle
          // + 64 offset to get a cosine that starts from 1 instead of a sine from 0
          dim = 255 - pgm_read_byte(&SINES[(int) (pct_remaining_in_step * 127 + 64)]);
        } else if (!game_of_life_state_old[i] && game_of_life_state[i]) {
          dim = pgm_read_byte(&SINES[(int) (pct_remaining_in_step * 127 + 64)]);
        } else if (!game_of_life_state_old[i] && !game_of_life_state[i]) {
          dim = 0;
        } else {
          dim = 255;
        }

        if (currentmode == MODE_RAINBOW) {
          uint32_t c = Wheel(((i * 256 / stripL.numPixels()) + j) & 255);
          r = (uint8_t)(c >> 16);
          g = (uint8_t)(c >>  8);
          b = (uint8_t)c;
        } else {
          long c = ((i * 256 / stripL.numPixels()) + j) & 255;
          r = rValue(max(abs(c - 127) * 6 + 127, 255)); // abs(c-127) makes things mirror
          g = gValue(max(abs(c - 127) * 6 + 127, 255)); // *6 scales to the range of the xValue fxn
          b = bValue(max(abs(c - 127) * 6 + 127, 255)); // +255 offsets to remove black, min to create more color1
        } 

        r = (r * dim) >> 8;
        g = (g * dim) >> 8;
        b = (b * dim) >> 8;
        
        stripL.setBrightness(LED_BRIGHTNESS >> 1);
        stripR.setBrightness(LED_BRIGHTNESS >> 1);
        stripL.setPixelColor(i, r, g, b);
        stripR.setPixelColor(i, r, g, b);
      }
      break;
  }
  
  stripL.show();
  stripR.show();
}

void resetGameOfLife() {
  next_game_of_life_tick_time = timer + TIMESTEP_GAME_OF_LIFE;
  for (int i = 0; i < N_LEDS; i = i + 1) {
    game_of_life_state_old[i] = false;
    game_of_life_state[i] = (random(0,6) < 1);
  }
}

void serviceGameOfLife() {
  if (timer > next_game_of_life_tick_time) {
    next_game_of_life_tick_time = next_game_of_life_tick_time + TIMESTEP_GAME_OF_LIFE;

    memcpy(game_of_life_state_old, game_of_life_state, sizeof(game_of_life_state_old));

    game_of_life_state[0] = GAME_OF_LIFE_EIGHTEEN[
        (game_of_life_state_old[1] << 1) +
        game_of_life_state_old[2]];
    for (int i = 1; i < N_LEDS - 1; i = i + 1) {
      game_of_life_state[i] = GAME_OF_LIFE_EIGHTEEN[
        (game_of_life_state_old[i-1] << 2) +
        (game_of_life_state_old[i] << 1) +
        game_of_life_state_old[i+1]];
    }
    game_of_life_state[N_LEDS - 1] = GAME_OF_LIFE_EIGHTEEN[
        (game_of_life_state_old[N_LEDS - 2] << 2) +
        (game_of_life_state_old[N_LEDS - 1] << 1)];
  }
}

void readAccel() { // helper function to read the accelerometer and calculate the jerk.
  int xint = analogRead(XACCEL_PIN) - 512;
  int yint = analogRead(YACCEL_PIN) - 512;
  int zint = analogRead(ZACCEL_PIN) - 512;
  float xOld = x;
  float yOld = y;
  float zOld = z;
  
  x = ((float)xint) / 1024 * V_REF * G_PER_V;
  y = ((float)yint) / 1024 * V_REF * G_PER_V;
  z = ((float)zint) / 1024 * V_REF * G_PER_V;
   
  xJerk = x - xOld;
  yJerk = y - yOld;
  zJerk = z - zOld;

  jerkMag = xJerk * xJerk + yJerk * yJerk + zJerk * zJerk;
}

long getMag(float jerk) { // this turns a jerk value into a pixel magnitude using a predefined multiplier and bounds.
  return max(min(jerk * MULTIPLIER, MAX_STEP_MAG), MIN_STEP_MAG);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return stripL.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return stripL.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return stripL.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// The following three helper functions do the math to smoothly change between any
// arbitrary set of colors depending on the "brightness" level.
// level = 0: color0
// level = 256: color1
// level = 512: color2
// level = 768+: color3

uint8_t rValue(long level) {
  uint8_t r;
  if (level < 256) {
    r = pgm_read_byte(&gamma[(color0[0] * (255L - level) + color1[0] * (level) + 128L) >> 8]);
  } else if (level < 512) {
    r = pgm_read_byte(&gamma[(color1[0] * (255L - (level - 256L)) + color2[0] * (level - 256L) + 128L) >> 8]);
  } else if (level < 768) {
    r = pgm_read_byte(&gamma[(color2[0] * (255L - (level - 512L)) + color3[0] * (level - 512L) + 128L) >> 8]);
  } else {
    r = pgm_read_byte(&gamma[color3[0]]);
  }
  return r;
}

uint8_t gValue(long level) {
  uint8_t g;
  if (level < 256) {
    g = pgm_read_byte(&gamma[(color0[1] * (255L - level) + color1[1] * (level) + 128L) >> 8]);
  } else if (level < 512) {
    g = pgm_read_byte(&gamma[(color1[1] * (255L - (level - 256L)) + color2[1] * (level - 256L) + 128L) >> 8]);
  } else if (level < 768) {
    g = pgm_read_byte(&gamma[(color2[1] * (255L - (level - 512L)) + color3[1] * (level - 512L) + 128L) >> 8]);
  } else {
    g = pgm_read_byte(&gamma[color3[1]]);
  }
  return g;
}

uint8_t bValue(long level) {
  uint8_t b;
  if (level < 256) {
    b = pgm_read_byte(&gamma[(color0[2] * (255L - level) + color1[2] * (level) + 128L) >> 8]);
  } else if (level < 512) {
    b = pgm_read_byte(&gamma[(color1[2] * (255L - (level - 256L)) + color2[2] * (level - 256L) + 128L) >> 8]);
  } else if (level < 768) {
    b = pgm_read_byte(&gamma[(color2[2] * (255L - (level - 512L)) + color3[2] * (level - 512L) + 128L) >> 8]);
  } else {
    b = pgm_read_byte(&gamma[color3[2]]);
  }
  return b;
}

const uint8_t PROGMEM gamma[] = { // gamma correction table
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
  2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
  5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

const uint8_t PROGMEM SINES[] = {
  127, 130, 133, 136, 139, 142, 145, 148, 151, 154, 157, 161, 164, 166, 169, 172, 175, 
  178, 181, 184, 187, 189, 192, 195, 197, 200, 202, 205, 207, 210, 212, 214, 217, 219, 
  221, 223, 225, 227, 229, 231, 232, 234, 236, 237, 239, 240, 242, 243, 244, 245, 246, 
  247, 248, 249, 250, 251, 251, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 
  253, 252, 252, 251, 251, 250, 249, 249, 248, 247, 246, 245, 243, 242, 241, 239, 238, 
  236, 235, 233, 231, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 209, 206, 204, 
  201, 199, 196, 193, 191, 188, 185, 182, 180, 177, 174, 171, 168, 165, 162, 159, 156, 
  153, 150, 147, 144, 141, 137, 134, 131, 128, 126, 123, 120, 117, 113, 110, 107, 104, 
  101, 98, 95, 92, 89, 86, 83, 80, 77, 74, 72, 69, 66, 63, 61, 58, 55, 53, 50, 48, 45, 
  43, 41, 39, 36, 34, 32, 30, 28, 26, 24, 23, 21, 19, 18, 16, 15, 13, 12, 11, 9, 8, 7, 
  6, 5, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 
  10, 11, 12, 14, 15, 17, 18, 20, 22, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 
  49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 
  106, 109, 112, 115, 118, 121, 124, 127};

