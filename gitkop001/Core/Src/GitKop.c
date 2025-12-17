#include "GitKop.h"
#include "main.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_gpio.h"

#define BUZZ_TIMER htim12
#define PULSE_TIMER htim2
#define UART huart1

volatile static uint32_t pulseTickCtr = 0;
volatile static uint16_t pulseTickAnalysisCtr = 0;
static const uint16_t PULSE_WIDTH = 15;
static const uint16_t TOTAL_WIDTH = 5000;

#define DMA_BUFFER_ENTRIES 2048
__attribute__((aligned(32))) volatile static uint16_t value[DMA_BUFFER_ENTRIES];

volatile static uint16_t dmaIndex;


// Pulse every 1us
void PulseOut_Handler() {
 
    if (pulseTickCtr == 0)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 1);
    }

    if (pulseTickCtr == PULSE_WIDTH)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 0);
    }

    if (pulseTickCtr == PULSE_WIDTH + 10)
    {
        pulseTickAnalysisCtr++;
        dmaIndex = __HAL_DMA_GET_COUNTER(hadc1.DMA_Handle) / 2;
    }

    pulseTickCtr++;
    if (pulseTickCtr == TOTAL_WIDTH)
    {
        pulseTickCtr = 0;
    }
}

int _write(int file, char* ptr, int len) {
    HAL_UART_Transmit(&UART, (uint8_t*) ptr, len, HAL_MAX_DELAY);
    return len;
}

void buzz(uint16_t freq) {
    if (freq == 0)
    {
        __HAL_TIM_SET_COMPARE(&BUZZ_TIMER, TIM_CHANNEL_1, 0);
        return;
    }

    uint16_t autoreload = (1000000UL / freq) - 1;

    __HAL_TIM_SET_AUTORELOAD(&BUZZ_TIMER, autoreload);

    __HAL_TIM_SET_COMPARE(&BUZZ_TIMER, TIM_CHANNEL_1, autoreload / 2);
}


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // do abs()

static bool initialized = false;

// --- Configuration ---
const float ALPHA_FAST = 0.1;    // Responsive: Smoothing for the signal
const float ALPHA_SLOW = 0.005;  // Sluggish: Tracks drifting background/ground
const float THRESHOLD  = 15.0;   // Trigger sensitivity (adjust based on your noise)

// --- State Variables ---
float fastAvg = 0;
float slowAvg = 0;
bool isInitialized = false;

float handle_data(uint16_t rawSample)
{
  if (!isInitialized) {
    fastAvg = rawSample;
    slowAvg = rawSample;
    isInitialized = true;
    return 0; // Skip the rest of this loop iteration
  }

  fastAvg = (ALPHA_FAST * rawSample) + ((1.0 - ALPHA_FAST) * fastAvg);

  float difference = fastAvg - slowAvg;

  uint8_t metalDetected = false;
  
  if (difference > THRESHOLD) {
    metalDetected = true;
  } else {
    slowAvg = (ALPHA_SLOW * rawSample) + ((1.0 - ALPHA_SLOW) * slowAvg);
  }
  return difference;
}

void GitKop_Init()
{
    printf("GitKop build %s %s\r\nCreated by Pawel Reich, https://gitmanik.dev\r\n", __TIME__, __DATE__);
    HAL_TIM_Base_Start_IT(&PULSE_TIMER); 
    // HAL_TIM_Base_Start_IT(&BUZZ_TIMER);

    // HAL_TIM_PWM_Start(&BUZZ_TIMER, TIM_CHANNEL_1);
    // buzz(0);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, DMA_BUFFER_ENTRIES);

    HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, 0);
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

static uint8_t DEBUG_MODE = 1;

void GitKop_Loop()
{
    if (dmaIndex != 0) 
    {
        #define HISTORY_LEN 42
        #define INTEREST_THRES 3500
        uint16_t linear_history[HISTORY_LEN]; 

        uint16_t head_ptr = DMA_BUFFER_ENTRIES - dmaIndex;

        Get_Last_N_Samples(value, linear_history, head_ptr, HISTORY_LEN, DMA_BUFFER_ENTRIES);
        
        size_t areaOfInterestIndex = 0;
        size_t smallestSampleIndex = 0;
        size_t smallestSample = 4095;
        for (size_t i = 0; i < HISTORY_LEN; i++)
        {
            uint16_t sample = linear_history[i];
            if (areaOfInterestIndex == 0 && sample > INTEREST_THRES)
            {
                areaOfInterestIndex = i;
                smallestSampleIndex = i;
                continue;
            }
            if (areaOfInterestIndex > 0)
            {
                if (sample < smallestSample)
                {
                    smallestSample = sample;
                    smallestSampleIndex = i;
                }
            }
        }

        float val = handle_data(smallestSample);

        if (pulseTickAnalysisCtr > 100)
        {
            if (DEBUG_MODE)
            {
                printf("DATA[");
                for (int i = 0; i < HISTORY_LEN; i++)
                {
                    printf("%d ", linear_history[i]);
                }
                printf("] %d %d %f$\r\n", smallestSampleIndex, areaOfInterestIndex, val);
                // printf("smallestSample: %d, val: %d\r\n", smallestSample, signal_diff);
            }
            pulseTickAnalysisCtr = 0;
        }

        dmaIndex = 0;
    }
}
