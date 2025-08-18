#include <Arduino.h>
#include "esp_adc/adc_continuous.h"

const int PULSE_OUT = 1;

const int PULSE_IN = 2;

uint pulseTickCtr = 0;
unsigned long usc = 0;

void IRAM_ATTR pulseTimerFired()
{
    if (pulseTickCtr == 0)
    {
        digitalWrite(PULSE_OUT, HIGH);
    }
    else
    {
        digitalWrite(PULSE_OUT, LOW);
    }

    pulseTickCtr++;

    if (pulseTickCtr == 2 * 10)
    {
        pulseTickCtr = 0;
        usc = micros();
    }

}

void setup()
{
    // Pulse generator setup
    pinMode(PULSE_OUT, OUTPUT);

    // pulseTimer = timerBegin(2000);

    // timerAlarm(pulseTimer, 1, true, 0);

    // timerAttachInterrupt(pulseTimer, &pulseTimerFired);
    // timerStart(pulseTimer);

    // Pulse input setup

    // analogContinuousSetWidth(12);
    // analogContinuousSetAtten(ADC_11db);
    // analogContinuous({PULSE_IN}, adc_pins_count, CONVERSIONS_PER_PIN, 20000, &adcComplete);

    // Start ADC Continuous conversions
    // analogContinuousStart();

}

void loop()
{
  digitalWrite(PULSE_OUT, LOW);
  // delayMicroseconds(400);
  delay(10);
  // delayMicroseconds(500);
  digitalWrite(PULSE_OUT, HIGH);
  delayMicroseconds(300);
  // delay(5);
}
