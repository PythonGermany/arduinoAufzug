#include <Arduino.h>

#include "Inputs.hpp"
#include "Led.hpp"
#include "Memory.hpp"
#include "Motor.hpp"
#include "debug.hpp"

// Led timing macros
#define INITINTERVAL 2000
#define SENSORINTERVAL 4000
#define EMERGENCYINTERVAL 8000
#define LEDSTRIPDELAY 60000

// Floor data macros
#define FLOORCOUNT 4
#define FLOORBOTTOM 0
#define FlOORTOP FLOORCOUNT - 1
#define INITFLOOR 2

// Memory macros
#define SAVESLOT 0
#define UNCONNECTED 19

// In and out components
Inputs request(2, FLOORCOUNT);
Inputs sensor(8, FLOORCOUNT);  // DEV: Invert back for real sensor
Led ledStrip(12, LEDSTRIPDELAY);
Motor motor(6, 7, &ledStrip);
Led errorLed(LED_BUILTIN);
Inputs manual(14, 2);
Inputs motion(16, 1, true);  // DEV: Optional!
Inputs emergency(17, 1);     // DEV: Invert back for real sensor
Inputs reset(18, 1);

// Location memory
// Memory size of 100 means at least 100 * 100 000 writes = 10 000 000 writes so
// for 600 (100 times up and down every day) changes a day that would be ~45y
// years worst in the worst case.
uint16_t maxMemorySize = (EEPROM.length() - SAVESLOT) / 2;
Memory memory(min(100 + HEADERSIZE, maxMemorySize), SAVESLOT, 2);

// Motor stop delay variables
int16_t stopDelay[] = {1500, 1000, 500, 0};

// Runtime elevator state variables
int8_t locNow;
int8_t locStop = STOP;

// Generate based on value from unconnected pin
unsigned long generateSeed(uint8_t pin) {
  unsigned long seed = 0;
  for (uint8_t i = 0; i < 32; i++) seed |= (analogRead(pin) & 0x01) << i;
  return seed;
}

// Infinite loop due to unsolvable error
void errorState() {
  locStop = motor.stop(0);
  memory.write(NONE);
#ifdef DEBUG
  Serial.println("ERROR:   Error state!");
#endif
  while (true) {
    errorLed.blink(SENSORINTERVAL);
    ledStrip.blink(SENSORINTERVAL);
  }
}

// Loop while emergency button is pressed
void emergencyState() {
  locStop = motor.stop(0);
#ifdef DEBUG
  Serial.println("SPECIAL: Emergency state!");
#endif
  while (emergency.update() != NONE) {
    errorLed.blink(EMERGENCYINTERVAL);
    ledStrip.blink(EMERGENCYINTERVAL);
  }
  errorLed.off();
  ledStrip.delay(true);
}

// Update input states and write to memory if current location changed
void updateInputStates() {
  int8_t last = sensor.last();
  locNow = sensor.update(true);
  if (locNow != last) memory.write(locNow);
  if (sensor.error()) errorState();
  if (emergency.update() != NONE) emergencyState();
}

// Process manual request if there is one
void processManualRequest() {
  int8_t request = manual.update();
  int8_t blockingFloor = request == UP ? FlOORTOP : FLOORBOTTOM;
  if (request != NONE && locNow != blockingFloor) {
    request == UP ? motor.up() : motor.down();
    while (manual.update() == request && locNow != blockingFloor)
      updateInputStates();
  }
  if (request != NONE) locStop = motor.stop(0);
}

// Load location from memory or intialize elevator by moving to FLOORTOP and
// back to INITFLOOR
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  randomSeed(generateSeed(UNCONNECTED));
  bool resetMem = reset.update() != NONE;
  bool error = memory.init(resetMem);
  if (error == true) memory.init(true);
  int8_t curr = NONE;
  if (!error && !resetMem) curr = memory.read(error);
  sensor.setLast(curr);
#ifdef DEBUG
  memory.debug();
#endif
  locNow = sensor.update(true);
  if (locNow < FLOORBOTTOM || locNow > FlOORTOP) {
    while (request.update() == NONE) {
      updateInputStates();
      ledStrip.blink(INITINTERVAL);
    }
    motor.up();
    while (locNow != FlOORTOP) updateInputStates();
    motor.stop(stopDelay[locNow]);
    locStop = INITFLOOR;
  }
}

void loop() {
#ifdef DEBUG
  printDebug(motor, ledStrip, locNow, locStop, manual, sensor, request, motion);
#endif
  if (motor.state() == STOP) {
    if (ledStrip.state() == ON) ledStrip.delay();
    if (motion.update() != NONE) ledStrip.delay(true);
  }
  updateInputStates();
  if (locStop == STOP) {
    locStop = request.update();
    if (locStop == locNow) locStop = STOP;
  }
  if (motor.state() != STOP && locStop == locNow)
    locStop = motor.stop(stopDelay[locNow]);
  else if (motor.state() == STOP && locStop != STOP)
    locStop > locNow ? motor.up() : motor.down();
  processManualRequest();
}