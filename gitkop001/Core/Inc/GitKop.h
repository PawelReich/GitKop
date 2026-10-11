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
#define PULSE_CHANNEL TIM_CHANNEL_3
#define UART huart1
#define ADC hadc1

/* Coil drive pulse width, in microseconds. This is the value that is actually
 * meaningful physically: the time the drive stage is on and current builds in
 * the coil. The CCR3 compare value is derived from it and the live prescaler,
 * so retuning the prescaler for resolution cannot silently desynchronise the
 * two. Everything here is integer, and the timestamp the board streams is in
 * the same unit. */
#define PULSE_WIDTH_US 30u

#define STREAM_THROTTLE_DIV 4
#define BUZZ_COMMAND "BUZZ "

void Buzzer_Set(uint16_t freq);
uint32_t Pulse_Timer_Tick_Hz(void);
uint32_t Pulse_Timer_Tick_Ns(void);
void Pulse_Set_Width_Us(uint32_t widthUs);
int _write(int file, char* ptr, int len);
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

void GitKop_Init();
void GitKop_Loop();
