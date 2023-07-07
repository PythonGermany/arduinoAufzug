#include "debug.hpp"

#ifdef DEBUG
void printDebug(Motor &motor, Led &ledStrip, int8_t &locNow, int8_t &locStop,
                Inputs &manual, Inputs &sensor, Inputs &request,
                Inputs &motion) {
  static unsigned long prev;
  if (millis() - prev <= DEBUGINTERVAL) return;
  prev = millis();
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
  Serial.print("; TestLocNow: ");
  int8_t testLocNow = sensor.update();
  Serial.print(testLocNow > NONE ? String(testLocNow) + "   " : "None");
  Serial.print("; TestLocStop: ");
  int8_t testLocStop = request.update();
  Serial.print(testLocStop > NONE ? String(testLocStop) + "   " : "None");
  Serial.print("; Motion: ");
  Serial.println(motion.update() > NONE ? "Yes" : "No ");
}
#endif