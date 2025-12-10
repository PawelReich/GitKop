#include "GitKop.h"
#include "main.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_gpio.h"

#define BUZZ_TIMER htim12
#define PULSE_TIMER htim2
#define UART huart1

volatile static uint16_t pulseTickCtr = 0;
volatile static uint16_t pulseTickAnalysisCtr = 0;

#define DMA_BUFFER_ENTRIES 2048
__attribute__((aligned(32))) volatile static uint16_t value[DMA_BUFFER_ENTRIES];

volatile static uint16_t dmaIndex;


// Pulse every 1us
void PulseOut_Handler() {
 
    if (pulseTickCtr == 0)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 1);
    }

    if (pulseTickCtr == 25)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 0);
    }

    if (pulseTickCtr == 35)
    {
        pulseTickAnalysisCtr++;
        dmaIndex = __HAL_DMA_GET_COUNTER(hadc1.DMA_Handle) / 2;
    }

    pulseTickCtr++;
    if (pulseTickCtr == 500)
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

// Definicje rozmiarów buforów (najlepiej potęgi 2 dla szybkości dzielenia)
#define FAST_WINDOW 16   // Szybki filtr (np. 16 próbek)
#define SLOW_WINDOW 256  // Wolny filtr - tło (np. 256 próbek)

// Bufory i sumy
static uint16_t buf_fast[FAST_WINDOW];
static uint16_t buf_slow[SLOW_WINDOW];

static uint32_t sum_fast = 0;
static uint32_t sum_slow = 0;

static uint16_t idx_fast = 0;
static uint16_t idx_slow = 0;

static bool initialized = false;

// Zmienne wynikowe dostępne "na zewnątrz"
volatile int16_t signal_diff = 0; // Różnica (Sygnał - Tło)
volatile uint16_t background_level = 0;

int handle_data(uint16_t raw_sample)
{
    // --- 1. Inicjalizacja przy pierwszym uruchomieniu ---
    if (!initialized) {
        // Wypełniamy bufory pierwszą wartością, żeby średnia nie startowała od 0
        for (int i = 0; i < FAST_WINDOW; i++) buf_fast[i] = raw_sample;
        for (int i = 0; i < SLOW_WINDOW; i++) buf_slow[i] = raw_sample;
        
        sum_fast = raw_sample * FAST_WINDOW;
        sum_slow = raw_sample * SLOW_WINDOW;
        
        initialized = true;
    }

    // --- 2. Obsługa szybkiego filtra (FAST - Signal) ---
    // Odejmij najstarszą, dodaj nową
    sum_fast -= buf_fast[idx_fast];
    buf_fast[idx_fast] = raw_sample;
    sum_fast += raw_sample;

    idx_fast++;
    if (idx_fast >= FAST_WINDOW) idx_fast = 0;

    // --- 3. Obsługa wolnego filtra (SLOW - Background) ---
    // Odejmij najstarszą, dodaj nową
    sum_slow -= buf_slow[idx_slow];
    buf_slow[idx_slow] = raw_sample;
    sum_slow += raw_sample;

    idx_slow++;
    if (idx_slow >= SLOW_WINDOW) idx_slow = 0;

    // --- 4. Obliczenie średnich ---
    // Kompilator zamieni to na przesunięcia bitowe (bit-shift), jeśli dzielimy przez potęgę 2
    uint16_t avg_fast = sum_fast / FAST_WINDOW;
    uint16_t avg_slow = sum_slow / SLOW_WINDOW;

    // Zapisz poziom tła (do debugowania lub wyświetlania)
    background_level = avg_slow;

    // --- 5. Wykrycie zmiany (Sygnał użyteczny) ---
    // Obliczamy różnicę. 
    // Wynik dodatni = nagły wzrost sygnału.
    // Wynik ujemny = nagły spadek sygnału.
    signal_diff = (int16_t)avg_fast - (int16_t)avg_slow;

    return signal_diff;

    /*
    Przykład użycia (pseudokod):
    if (abs(signal_diff) > THRESHOLD) {
        // Wykryto dotyk / zdarzenie / pik!
    }
    */
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

        int val = handle_data(smallestSample);

        if (pulseTickAnalysisCtr > 250)
        {
            if (DEBUG_MODE)
            {
                printf("DATA[");
                for (int i = 0; i < HISTORY_LEN; i++)
                {
                    printf("%d ", linear_history[i]);
                }
                printf("] %d %d$\r\n", smallestSampleIndex, areaOfInterestIndex);
                printf("smallestSample: %d, val: %d\r\n", smallestSample, val);
            }
            pulseTickAnalysisCtr = 0;
        }

        dmaIndex = 0;
    }
}
