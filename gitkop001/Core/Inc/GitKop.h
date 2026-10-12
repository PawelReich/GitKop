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

/* Buzzer pitch limits, in hertz, mirroring the Python side. The band sits high
 * because the 12 mm magnetic buzzer on the board is resonant: 4 kHz is plainly
 * audible, 1.3 kHz is not, so anything below roughly 3 kHz is wasted range. The
 * floor is also not cosmetic -- the autoreload is 16-bit, so the divisor in
 * Buzzer_Set wraps for anything under roughly 16 Hz and the tone goes silent
 * instead of low. */
#define BUZZ_MIN_HZ 3000
#define BUZZ_MAX_HZ 6000

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
