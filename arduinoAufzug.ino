
#include "Inputs.hpp"
#include "Led.hpp"
#include "Motor.hpp"

// Debug macros
#define DEBUG
#define DEBUGINTERVAL 2500

// Led timing macros
#define ERRORLEDINTERVAL 250
#define LEDSTRIPINTERVAL 6000
#define LEDSTRIPTIMER 60000

// Floor data macros
#define FLOORCOUNT 4
#define FlOORTOP FLOORCOUNT - 1
#define FLOORBOTTOM 0
#define INITFLOOR 2

// Sensor save address location
#define SAVESLOTADDRESS 0

// In and out components
Inputs request(2, FLOORCOUNT);
Inputs sensor(8, FLOORCOUNT);  // DEV: Invert back for real sensor
Led ledStrip(12, LEDSTRIPINTERVAL);
Motor motor(6, 7, &ledStrip);
Led errorLed(LED_BUILTIN, ERRORLEDINTERVAL);
Inputs manual(14, 2);

// Runtime state variables
int8_t locNow = NONE;
int8_t locStop = NONE;
bool error = false;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  ledStrip.updateTimer(false, LEDSTRIPTIMER);
  sensor.setSaveAddress(SAVESLOTADDRESS);
  locNow = sensor.update(true);
}

void loop() {
#ifdef DEBUG
  static unsigned long prev;
  if (millis() - prev >= DEBUGINTERVAL) {
    prev = millis();
    printDebug();
  }
#endif
  if (!error) {
    if (ledStrip.state() == ON && motor.state() == STOP) ledStrip.updateTimer();
    processManualRequest();
    if (locStop == NONE) locStop = request.update();
    locNow = sensor.update(true);
    error = sensor.error();
  } else {
    errorLed.blink();
    ledStrip.blink();
  }
  if (motor.state() != STOP && (locStop == locNow || error))
    locStop = motor.stop();
  else if (motor.state() == STOP && locStop != NONE && !error)
    locStop > locNow ? motor.up() : motor.down();
}

void processManualRequest() {
  int8_t manualRequest = manual.update();
  if (manualRequest != NONE) {
    locStop = motor.stop();
    if (manualRequest == DOWN && locNow != FLOORBOTTOM) {
      motor.down();
#ifdef DEBUG
      printDebug();
#endif
      while (manual.update() == DOWN && locNow != FLOORBOTTOM && !error) {
        locNow = sensor.update(true);
        error = sensor.error();
      }
    } else if (manualRequest == UP && locNow != FlOORTOP) {
      motor.up();
#ifdef DEBUG
      printDebug();
#endif
      while (manual.update() == UP && locNow != FlOORTOP && !error) {
        locNow = sensor.update(true);
        error = sensor.error();
      }
    }
    motor.stop();
  }
}

#ifdef DEBUG
void printDebug() {
  Serial.print("Motor state: ");
  Serial.print(motor.state() > STOP
                   ? motor.state() > DOWN ? "Up     " : "Down   "
                   : "Stopped");
  Serial.print("; LedStrip: ");
  Serial.print(ledStrip.state() == ON ? "On " : "Off");
  Serial.print("; Goal: ");
  Serial.print(locNow == NONE ? "None" : "   " + String(locNow));
  Serial.print("->");
  Serial.print(locStop == NONE ? "None" : String(locStop) + "   ");
  Serial.print("; Manual: ");
  int8_t manualRequest = manual.update();
  Serial.print(manualRequest > NONE ? manualRequest > DOWN ? "Up  " : "Down"
                                    : "None");
  Serial.print("; Error: ");
  Serial.print(error ? "Yes" : "No ");
  Serial.print("; Current Error: ");
  Serial.print(sensor.error() ? "Yes" : "No ");
  Serial.print("; TestLocNow: ");
  int8_t testLocNow = sensor.update();
  Serial.print(testLocNow > NONE ? String(testLocNow) + "   " : "None");
  Serial.print("; TestLocStop: ");
  int8_t testLocStop = request.update();
  Serial.println(testLocStop > NONE ? String(testLocStop) + "   " : "None");
}
#endif