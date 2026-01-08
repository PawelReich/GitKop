#include <stdint.h>

#include "GitKop.h"
#include "ema.h"
#include "main.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_rcc.h"
#include "stm32h5xx_hal_tim.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

static uint16_t debugOutputCtr = 0;
static uint16_t updateOledCtr = 0;
static uint16_t stabilizedCounter = 0;

volatile static uint16_t dmaIndex = 0;
volatile static uint16_t timerIndex = 0;
volatile static uint8_t outOfWindowTriggered = 1;

#define DMA_BUFFER_ENTRIES 2048

__attribute__((aligned(32))) volatile static uint16_t value[DMA_BUFFER_ENTRIES];

float detectionThreshold = 5;
EMA_t slowFilter;
EMA_t fastFilter;

uint8_t test = 0;
uint8_t emaSetUp = 0;

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

float Handle_Sample(uint16_t rawSample)
{
    float x = rawSample;
    if (!emaSetUp)
    {
        EMA_Init(&slowFilter, 0.0005, x);
        EMA_Init(&fastFilter, 0.1, x);
        emaSetUp = 1;
    }
    else
    {
        EMA_Update(&fastFilter, rawSample);
        EMA_Update(&slowFilter, rawSample);
    }

    float difference = fastFilter.out - slowFilter.out;

    if (difference < detectionThreshold)
    {
        Buzzer_Set(0);
    }
    else
    {
        float hz = difference;
        if (ENABLE_BUZZER)
            Buzzer_Set(hz);
    }
    return difference;
}

void GitKop_Init()
{
    printf("GitKop build %s %s\r\nCreated by Pawel Reich, https://gitmanik.dev\r\n", __TIME__, __DATE__);

    HAL_TIM_PWM_Start(&PULSE_TIMER, TIM_CHANNEL_3);
    HAL_TIM_Base_Start_IT(&PULSE_TIMER);
    HAL_TIM_Base_Start_IT(&BUZZ_TIMER);

    HAL_TIM_PWM_Start(&BUZZ_TIMER, BUZZ_CHANNEL);
    Buzzer_Set(0);

    HAL_ADC_Start_DMA(&ADC, (uint32_t*)value, DMA_BUFFER_ENTRIES);
    HAL_ADC_Start_IT(&ADC);
    HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, 0);

    SSD1306_Init();
    SSD1306_WriteString("GitKop", Font_11x18, White);
    SSD1306_UpdateScreen();
    HAL_Delay(500);

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

float Calculate_Slope(uint16_t* buffer, size_t sz)
{
    float delta_s = ((float) (buffer[sz-1] - buffer[0]));
    float delta_v = delta_s * (3.3f / 4095.0f);
    float delta_time = (sz - 1)/(4160000.f);
    return delta_v / delta_time;
}
#define CALCULATE_N(us)    ((((us) * 416) + 50) / 100)

uint16_t enc_s = 0;
void GitKop_Loop()
{
    if (dmaIndex != 0)
    {
        stabilizedCounter++;
        if (stabilizedCounter < 1000)
        {
            goto end;
        }

        debugOutputCtr++;
        updateOledCtr++;
        #define HISTORY_LEN CALCULATE_N(10)
        uint16_t linear_history[HISTORY_LEN];

        uint16_t head_ptr = DMA_BUFFER_ENTRIES - dmaIndex;

        Get_Last_N_Samples(value, linear_history, head_ptr, HISTORY_LEN, DMA_BUFFER_ENTRIES);

        float time = timerIndex * 0.02f;
        float val = Handle_Sample(time);
        float delta = Calculate_Slope(linear_history, HISTORY_LEN);

        if (debugOutputCtr > 100)
        {
            if (DEBUG_MODE)
            {
                printf("DATA[");
                for (int i = 0; i < HISTORY_LEN; i++)
                {
                    printf("%d ", linear_history[i]);
                }
                printf("] %d %f %f %f %f %f$\r\n", 0, time, val, delta, fastFilter.out, slowFilter.out);
            }
            debugOutputCtr = 0;
        }
        if (updateOledCtr > 250)
        {
            SSD1306_Fill(Black);
            char buf[60] = {0};
            snprintf(buf, 60, "T: %d us", (int)time);
            SSD1306_SetCursor(0,0);
            SSD1306_WriteString(buf, Font_11x18, White);
            SSD1306_SetCursor(0,20);
            snprintf(buf, 60, "S: %d V/s", (int) delta);
            SSD1306_WriteString(buf, Font_11x18, White);
            SSD1306_UpdateScreen();
            updateOledCtr = 0;
        }
        end:
        dmaIndex = 0;
    }
}
