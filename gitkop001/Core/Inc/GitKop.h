#pragma once

#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "tim.h"
#include "usart.h"
#include "adc.h"

#define BUZZ_TIMER htim12
#define BUZZ_CHANNEL TIM_CHANNEL_2
#define PULSE_TIMER htim1
#define UART huart1
#define ADC hadc1

static const uint8_t DEBUG_MODE = 0;
static const uint8_t ENABLE_BUZZER = 1;

void Buzzer_Set(uint16_t freq);
int _write(int file, char* ptr, int len);
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

void GitKop_Init();
void GitKop_Loop();
