#include <Arduino.h>
#include "driver/adc.h"

const int PULSE_OUT = 21;

const int PULSE_IN = 1;

hw_timer_t* pulseTimer = NULL;

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

    pulseTimer = timerBegin(0, 40000, true);
    timerAttachInterrupt(pulseTimer, &pulseTimerFired, true);
    timerAlarmWrite(pulseTimer, 1, true);
    timerAlarmEnable(pulseTimer);

    // Pulse input setup
    Serial.begin(250000);

    analogContinuousSetWidth(12);
    analogContinuousSetAtten(ADC_11db);
    analogContinuous({PULSE_IN}, adc_pins_count, CONVERSIONS_PER_PIN, 20000, &adcComplete);

    // Start ADC Continuous conversions
    analogContinuousStart();

}

void loop()
{
    if (usc + 10 > micros()) {
        ulong microsbefore = micros();
        float volt = analogRead(PULSE_IN);
        ulong microsafter = micros();
        Serial.println(volt + String(" ") +  " " + (microsafter - microsbefore));
    }
}
