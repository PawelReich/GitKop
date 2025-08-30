#include <Arduino.h>

const int PULSE_OUT = 1;

hw_timer_t* pulseTimer = NULL;

uint pulseTickCtr = 0;

void IRAM_ATTR pulseTimerFired()
{
    if (pulseTickCtr == 0)
    {
        digitalWrite(PULSE_OUT, HIGH);
    }

    if (pulseTickCtr == 10 * 2)
    {
        digitalWrite(PULSE_OUT, LOW);
    }

    pulseTickCtr++;

    if (pulseTickCtr == 1 * 100)
    {
        pulseTickCtr = 0;
    }

}

void timer_setup() {
    pulseTimer = timerBegin(20000);
    timerAlarm(pulseTimer, 1, true, 0);
    timerAttachInterrupt(pulseTimer, &pulseTimerFired);
    timerStart(pulseTimer);
}

void setup()
{
    Serial.begin(250000);

    // Pulse generator setup
    pinMode(PULSE_OUT, OUTPUT);
    timer_setup();
}

void loop()
{
}
