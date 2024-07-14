#include <Arduino.h>

const int PULSE = 21;

hw_timer_t* pulseTimer = NULL;

uint pulseTickCtr = 0;
void IRAM_ATTR pulseTimerFired()
{
  if (pulseTickCtr == 0)
  {
    digitalWrite(PULSE, HIGH);
  }
  else
  {
    digitalWrite(PULSE, LOW);
  }
  
  pulseTickCtr++;
  
  if (pulseTickCtr == 2 * 10)
    pulseTickCtr = 0;
}


void setup()
{
  pinMode(PULSE, OUTPUT);

  pulseTimer = timerBegin(0, 40000, true);
  timerAttachInterrupt(pulseTimer, &pulseTimerFired, true);
  timerAlarmWrite(pulseTimer, 1, true);
  timerAlarmEnable(pulseTimer);
}

void loop()
{

}