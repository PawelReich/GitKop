#pragma once

#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "tim.h"
#include "usart.h"
#include "adc.h"

void buzz(uint16_t freq);
int _write(int file, char* ptr, int len);
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

void GitKop_Init();
void GitKop_Loop();
