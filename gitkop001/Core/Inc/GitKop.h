#pragma once

#include <stdint.h>
#include <stdio.h> 

#include "gpio.h"
#include "tim.h"
// #include "usart.h"
// #include "dma.h"
#include "adc.h"
#include "spi.h"

void buzz(uint16_t freq);
void PulseOut_Handler();
int _write(int file, char* ptr, int len);

void GitKop_Init();
void GitKop_Loop();
