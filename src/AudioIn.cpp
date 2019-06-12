
#include <Arduino.h>
#include "AudioIn.h"

static bool bIsRunning = false;

void aiBegin(GetSampleFn pfn, unsigned int sampleRate)
{
  bIsRunning = true;
  Serial.printf("timer - begin\n");

  timer1_isr_init();
  timer1_attachInterrupt(pfn);

  // 80Mhz/16 * 5

  int period = 1000000/sampleRate;
  timer1_write(clockCyclesPerMicrosecond() * period);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
}

bool aiIsRunning() 
{
  return bIsRunning;
}

void aiEnd()
{
  timer1_disable();
  bIsRunning = false;
}
