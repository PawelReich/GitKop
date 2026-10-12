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

/* Whether CC2 is currently driven. HAL_TIM_PWM_Start refuses a channel that
 * is already busy, so the flag keeps repeated pitch changes from calling it
 * for nothing. */
static uint8_t buzzerRunning = 0;

void Buzzer_Set(uint16_t freqHz)
{
    if (freqHz == 0)
    {
        /* Stop the channel rather than trimming CCR to zero. Zeroing the
         * compare and waiting for the preload to transfer did not actually
         * silence the tone, so gate the output instead: this clears CC2E and
         * stops the counter, which parks the pin at its inactive level no
         * matter what is left in the compare register. */
        HAL_TIM_PWM_Stop(&BUZZ_TIMER, BUZZ_CHANNEL);
        buzzerRunning = 0;
        return;
    }

    /* The wire carries a bare integer with nothing to frame it, so clamp
     * before it can reach the 16-bit autoreload and wrap. */
    if (freqHz < BUZZ_MIN_HZ)
        freqHz = BUZZ_MIN_HZ;
    else if (freqHz > BUZZ_MAX_HZ)
        freqHz = BUZZ_MAX_HZ;

    uint32_t apb1 = HAL_RCC_GetPCLK1Freq();
    uint32_t psc = BUZZ_TIMER.Instance->PSC;

    uint32_t newAutoreload = (apb1 / ((psc + 1) * freqHz)) - 1;

    __HAL_TIM_SET_AUTORELOAD(&BUZZ_TIMER, newAutoreload);
    __HAL_TIM_SET_COMPARE(&BUZZ_TIMER, BUZZ_CHANNEL, newAutoreload / 2);
    BUZZ_TIMER.Instance->EGR = TIM_EGR_UG;

    if (!buzzerRunning)
    {
        HAL_TIM_PWM_Start(&BUZZ_TIMER, BUZZ_CHANNEL);
        buzzerRunning = 1;
    }
}

/* Every command is the buzzer frequency in hertz, bare. Anything unparseable
 * comes back from atoi as 0, which is the off state. */
static void Execute_Command(void)
{
    /* No printf in here: this runs in the UART receive interrupt, and a
     * blocking HAL_UART_Transmit of a whole echo line keeps the receiver
     * unserviced long enough to overrun the next command. An echo added for
     * debugging turned "132" into "1132" this way. */
    Buzzer_Set((uint16_t) atoi(rxLine));
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

    /* TIM12 stays stopped until a frequency actually arrives; Buzzer_Set owns
     * both edges of the enable now. No update interrupt either, as before:
     * there is no TIM12 handler to service one. */
    Buzzer_Set(0);

    HAL_ADC_Start_DMA(&ADC, (uint32_t*)value, DMA_BUFFER_ENTRIES);

    /* No HAL_ADC_Start_IT here. It enables ADC_IT_EOC, which at this
     * conversion rate lands in ADC1_IRQHandler roughly 4.2 million times a
     * second -- more often than the core has cycles to service one -- and
     * ADC1_IRQn sits at priority 0 above USART1_IRQn. The result is that the
     * UART receive interrupt is starved and the buzzer commands never land.
     * Nothing wants the per-sample interrupt: there is no
     * HAL_ADC_ConvCpltCallback, the DMA already moves the samples, and the
     * analog watchdog interrupt is armed on its own by
     * HAL_ADC_AnalogWDGConfig. */
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

    uint32_t now = __HAL_TIM_GET_COUNTER(&PULSE_TIMER);
    uint32_t skip_ticks = 100000u / Pulse_Timer_Tick_Ns();
    if (now < skip_ticks)
        return;

    dmaIndex = __HAL_DMA_GET_COUNTER(ADC.DMA_Handle) / 2;
    timerIndex = now;
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

void GitKop_Loop()
{
    if (dmaIndex == 0)
        return;

    stabilizedCounter++;
    dmaIndex = 0;

    if (stabilizedCounter < 1000)
        return;

    /* AWD exit time in microseconds, derived from the live prescaler rather
     * than a hardcoded tick. The timestamp is the whole packet: the sample
     * window is not streamed, so the DMA buffer only has to feed the analog
     * watchdog. */
    uint32_t timeUs = (uint32_t) timerIndex * Pulse_Timer_Tick_Ns() / 1000u;

    streamCtr++;
    if (streamCtr < STREAM_THROTTLE_DIV)
        return;
    streamCtr = 0;

    printf("%lu\r\n", (unsigned long) timeUs);
}