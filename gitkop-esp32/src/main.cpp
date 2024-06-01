#include <Arduino.h>

const int PULSE = 21;

void setup() {
  pinMode(PULSE, OUTPUT);
}

void loop() {
  digitalWrite(PULSE, LOW);
  // delayMicroseconds(400);
  delay(10);
  // delayMicroseconds(500);
  digitalWrite(PULSE, HIGH);
  delayMicroseconds(300);
  // delay(5);
}
