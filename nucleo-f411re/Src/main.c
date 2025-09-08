/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM10_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

    printf("GitKop build %s %s\n", __TIME__, __DATE__);
    HAL_TIM_Base_Start_IT(&htim10);
    
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    buzz(0);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    if (rdy)
    {
        // HAL_UART_Transmit(&huart2, ":)\n", 3, HAL_MAX_DELAY);
        if (val > 7)
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
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
