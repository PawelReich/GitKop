#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "GitKop.h"
#include "main.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_rcc.h"
#include "stm32h5xx_hal_tim.h"

static uint16_t stabilizedCounter = 0;
static uint16_t streamCtr = 0;

volatile static uint16_t dmaIndex = 0;
volatile static uint16_t timerIndex = 0;
volatile static uint8_t outOfWindowTriggered = 1;

#define DMA_BUFFER_ENTRIES 2048

__attribute__((aligned(32))) volatile static uint16_t value[DMA_BUFFER_ENTRIES];

uint8_t test = 0;

#define RX_LINE_MAX 16
static char rxLine[RX_LINE_MAX];
static uint8_t rxLineLen = 0;
static uint8_t rxByte;

uint32_t Pulse_Timer_Tick_Hz(void)
{
    RCC_ClkInitTypeDef clocks = {0};
    uint32_t latency;

    /* Both out-params are written unconditionally by the HAL, so neither may be
     * NULL: passing NULL here faults before main() is ever reached. */
    HAL_RCC_GetClockConfig(&clocks, &latency);

    uint32_t timerClock = HAL_RCC_GetPCLK2Freq();

    /* TIM1 sits on APB2. When the APB prescaler is not 1 the timer runs at
     * twice PCLK2, which is the usual "timer clock doubles when the bus is
     * divided" rule on this family. */
    if (clocks.APB2CLKDivider != RCC_HCLK_DIV1)
        timerClock *= 2;

    return timerClock / (PULSE_TIMER.Instance->PSC + 1);
}

uint32_t Pulse_Timer_Tick_Ns(void)
{
    return 1000000000u / Pulse_Timer_Tick_Hz();
}

void Pulse_Set_Width_Us(uint32_t widthUs)
{
    uint32_t ticks = widthUs * 1000u / Pulse_Timer_Tick_Ns();

    uint32_t period = PULSE_TIMER.Instance->ARR + 1;
    if (ticks > period)
        ticks = period;
    if (ticks == 0)
        ticks = 1;

    __HAL_TIM_SET_COMPARE(&PULSE_TIMER, PULSE_CHANNEL, ticks);
}

int _write(int file, char* ptr, int len) {
    HAL_UART_Transmit(&UART, (uint8_t*) ptr, len, HAL_MAX_DELAY);
    return len;
}

void Buzzer_Set(uint16_t freqHz)
{
    if (freqHz == 0)
    {
        __HAL_TIM_SET_COMPARE(&BUZZ_TIMER, BUZZ_CHANNEL, 0);
        return;
    }

    uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
    uint32_t psc = BUZZ_TIMER.Instance->PSC;

    uint32_t newAutoreload = (apb1 / ((psc + 1) * freqHz)) - 1;

    __HAL_TIM_SET_AUTORELOAD(&BUZZ_TIMER, newAutoreload);
    __HAL_TIM_SET_COMPARE(&BUZZ_TIMER, BUZZ_CHANNEL, newAutoreload / 2);
}

static void Execute_Command(void)
{
    if (strncmp(rxLine, BUZZ_COMMAND, strlen(BUZZ_COMMAND)) != 0)
        return;

    Buzzer_Set((uint16_t) atoi(&rxLine[strlen(BUZZ_COMMAND)]));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &UART)
        return;

    if (rxByte == '\n' || rxByte == '\r')
    {
        if (rxLineLen > 0)
        {
            rxLine[rxLineLen] = '\0';
            rxLineLen = 0;
            Execute_Command();
        }
    }
    else if (rxLineLen < RX_LINE_MAX - 1)
    {
        rxLine[rxLineLen++] = (char) rxByte;
    }

    HAL_UART_Receive_IT(&UART, &rxByte, 1);
}

void GitKop_Init()
{
    printf("GitKop build %s %s\r\nCreated by Pawel Reich, https://gitmanik.dev\r\n", __TIME__, __DATE__);

    HAL_TIM_PWM_Start(&PULSE_TIMER, PULSE_CHANNEL);
    HAL_TIM_Base_Start_IT(&PULSE_TIMER);
    Pulse_Set_Width_Us(PULSE_WIDTH_US);
    HAL_TIM_Base_Start_IT(&BUZZ_TIMER);

    HAL_TIM_PWM_Start(&BUZZ_TIMER, BUZZ_CHANNEL);
    Buzzer_Set(0);

    HAL_ADC_Start_DMA(&ADC, (uint32_t*)value, DMA_BUFFER_ENTRIES);
    HAL_ADC_Start_IT(&ADC);
    HAL_UART_Receive_IT(&UART, &rxByte, 1);
    HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, 0);

    while(1)
    {
        GitKop_Loop();
    }
}


void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc)
{
    if (outOfWindowTriggered)
        return;

    dmaIndex = __HAL_DMA_GET_COUNTER(ADC.DMA_Handle) / 2;
    timerIndex = __HAL_TIM_GET_COUNTER(&PULSE_TIMER);
    outOfWindowTriggered = 1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &PULSE_TIMER)
    {
        if (!test)
        {
            test = 1;
            return;
        }
        outOfWindowTriggered = 0;
    }
}

void Get_Last_N_Samples(uint16_t *buffer, uint16_t *output, uint16_t head_idx, uint16_t n_samples, uint16_t buf_len)
{
    int16_t start_idx = (head_idx - n_samples + buf_len) % buf_len;

    for (uint16_t i = 0; i < n_samples; i++)
    {
        output[i] = buffer[start_idx];

        start_idx++;
        if (start_idx >= buf_len)
        {
            start_idx = 0;
        }
    }
}

#define CALCULATE_N(us)    ((((us) * 416) + 50) / 100)

void GitKop_Loop()
{
    if (dmaIndex == 0)
        return;

    stabilizedCounter++;
    dmaIndex = 0;

    if (stabilizedCounter < 1000)
        return;

    #define HISTORY_LEN CALCULATE_N(10)
    uint16_t linear_history[HISTORY_LEN];

    uint16_t head_ptr = DMA_BUFFER_ENTRIES - dmaIndex;

    Get_Last_N_Samples(value, linear_history, head_ptr, HISTORY_LEN, DMA_BUFFER_ENTRIES);

    /* AWD exit time in nanoseconds, derived from the live prescaler rather
     * than a hardcoded tick. Nanoseconds rather than microseconds because the
     * tick is 20 ns and rounding here would quantise away the very resolution
     * this measurement exists to capture. Python divides by 1e3 to get us. */
    uint32_t timeNs = (uint32_t) timerIndex * Pulse_Timer_Tick_Ns();

    streamCtr++;
    if (streamCtr < STREAM_THROTTLE_DIV)
        return;
    streamCtr = 0;

    printf("DATA[");
    for (int i = 0; i < HISTORY_LEN; i++)
    {
        printf("%d ", linear_history[i]);
    }
    printf("] %d %lu$\r\n", 0, (unsigned long) timeNs);
}