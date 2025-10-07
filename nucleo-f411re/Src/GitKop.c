#include "GitKop.h"
#include "ili9341/ili9341.h"
#include "ili9341/ili9341_fonts.h"

volatile uint16_t pulseTickCtr = 0;

volatile static uint16_t value[1];
volatile static uint8_t rdy = 0;

#define STAGE0_SAMPLES 4
#define STAGE1_SAMPLES 16
#define STAGE2_SAMPLES 48

volatile static uint16_t stage0_ctr = 0;
volatile static uint16_t stage1_ctr = 0;
volatile static uint16_t stage2_ctr = 0;

volatile static uint16_t stage1_values[STAGE1_SAMPLES] = {0};
volatile static uint16_t stage2_values[STAGE2_SAMPLES] = {0};

volatile static uint16_t stage0_val = 0;

volatile static uint16_t val = 0;

// Pulse every 1.5ns
void PulseOut_Handler() {
 
    if (pulseTickCtr == 0)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 1);
    }

    if (pulseTickCtr == 251/5)
    {
        HAL_GPIO_WritePin(PULSE_OUT_GPIO_Port, PULSE_OUT_Pin, 0);
    }

    pulseTickCtr++;
    if (pulseTickCtr == 251/5+4)
    {
        stage0_val += value[0];
        stage0_ctr++;

        if (stage0_ctr == STAGE0_SAMPLES)
        {
            stage1_values[stage1_ctr++] = stage0_val / STAGE0_SAMPLES;
            stage0_val = 0;
            stage0_ctr = 0;

            if (stage1_ctr == STAGE1_SAMPLES)
                stage1_ctr = 0;

            uint32_t stage1_avg = 0;
            for (int i = 0; i < STAGE1_SAMPLES; i++)
                stage1_avg += stage1_values[i];

            stage1_avg = stage1_avg / STAGE1_SAMPLES;
            stage2_values[stage2_ctr++] = stage1_avg;

            if (stage2_ctr == STAGE2_SAMPLES)
                stage2_ctr = 0;

            uint32_t stage2_avg = 0;
            for (int i = 0; i < STAGE2_SAMPLES; i++)
                stage2_avg += stage2_values[i];

            stage2_avg = stage2_avg / STAGE2_SAMPLES;

            if (stage1_avg > stage2_avg)
                val = stage1_avg - stage2_avg;
            else
                val = stage2_avg - stage1_avg;
            
            rdy = 1;
        }
    }

    if (pulseTickCtr == 2500/5)
    {
        pulseTickCtr = 0;
    }
}

int _write(int file, char* ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*) ptr, len, HAL_MAX_DELAY);
    return len;
}

void buzz(uint16_t freq) {
    if (freq == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        return;
    }

    uint16_t autoreload = (1000000UL / freq) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, autoreload);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, autoreload / 2);
}

void GitKop_Init()
{
    printf("GitKop build %s %s\n", __TIME__, __DATE__);
    HAL_TIM_Base_Start_IT(&htim10); 
    HAL_TIM_Base_Start_IT(&htim2);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    buzz(0);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, 1);
    
    ILI9341_HandleTypeDef ili9341 = ILI9341_Init(
        &hspi5,
        LCD_CS_GPIO_Port,
        LCD_CS_Pin,
        LCD_DC_GPIO_Port,
        LCD_DC_Pin,
        LCD_RESET_GPIO_Port,
        LCD_RESET_Pin,
        ILI9341_ROTATION_HORIZONTAL_2,
        240,
        320
    );
    ILI9341_FillScreen(&ili9341, ILI9341_COLOR_RED);
    ILI9341_WriteString(
        &ili9341,
        5,                          // x
        60,                         // y
        "gitkop",            // string
        ILI9341_Font_Spleen12x24,  // font
        ILI9341_COLOR_WHITE,        // foreground color
        ILI9341_COLOR_BLACK,        // background color
        false,                      // line wrap
        1,                          // scale
        0,                          // tracking
        0                           // leading
    );
}

void GitKop_Loop()
{
    if (rdy)
    {
        // HAL_UART_Transmit(&huart2, ":)\n", 3, HAL_MAX_DELAY);
        if (val > 2)
        {
            printf("[%lu] %u\n", HAL_GetTick(), val);
            buzz(10+val*3);
            // __HAL_TIM_SET_AUTORELOAD(&htim2, 500+val*100);
            // __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 5000);
        }
        else
        {
            buzz(0);
            // __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        }
        rdy = 0;
    }
}
