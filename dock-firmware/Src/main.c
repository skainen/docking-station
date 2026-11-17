/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 1 = Time-multiplexed (L -> R -> N, 81 ms full cycle)
 * 2 = OR-able simultaneous overlap (27 ms full cycle) */
#define IR_SCHEME     1

#define DUTY_ON_CCR   140U  /* ~33% duty cycle at ARR = 420 (38.005 kHz) */
#define FRAME_TICKS   32U   /* 8 bits * 4 slots (0.5 ms/slot) = 16.0 ms */
#define GAP_TICKS     22U   /* 22 * 0.5 ms = 11.0 ms silence (> 10 ms spec) */
#define TOTAL_TICKS   (FRAME_TICKS + GAP_TICKS) /* 54 ticks = 27.0 ms/frame */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static const uint8_t beacon_code[3] = {
    0xA8U, /* CH1 (PA8 / D9) : Left  (1010 1000) */
    0xA4U, /* CH2 (PA9 / A4) : Right (1010 0100) */
    0xA1U  /* CH3 (PB6 / D1) : Near  (1010 0001) */
};

static volatile uint8_t tick_idx = 0;
static volatile uint8_t active_ch = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM16)
    {
        return;
    }

    uint16_t ccr[3] = {0, 0, 0};

    if (tick_idx < FRAME_TICKS)
    {
        uint8_t bit  = 7U - (tick_idx / 4U); /* MSB first */
        uint8_t slot = tick_idx % 4U;        /* 0..3 inside 2 ms bit */

        for (uint8_t ch = 0; ch < 3U; ch++)
        {
            bool ch_enabled = (IR_SCHEME == 2) ? true : (ch == active_ch);
            bool bit_is_one = ((beacon_code[ch] >> bit) & 1U) != 0U;
            bool carrier_on = ch_enabled && (slot == 0U || (bit_is_one && slot < 3U));

            ccr[ch] = carrier_on ? DUTY_ON_CCR : 0U;
        }
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr[0]);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr[1]);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr[2]);

    tick_idx++;
    if (tick_idx >= TOTAL_TICKS)
    {
        tick_idx = 0;
        if (IR_SCHEME == 1)
        {
            active_ch = (active_ch + 1U) % 3U; /* Cycle 0 (L) -> 1 (R) -> 2 (N) */
        }
    }
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
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM14_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  /* Freeze timers on debug halt so LEDs don't stay stuck ON */
  __HAL_DBGMCU_FREEZE_TIM1();
  __HAL_DBGMCU_FREEZE_TIM16();

  /* Initialize all 3 PWM channels at 0% duty before starting outputs */
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); /* PA8 / D9 (Left)  */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); /* PA9 / A4 (Right) */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); /* PB6 / D1 (Near)  */

  /* Start 0.5 ms slot tick interrupt */
  HAL_TIM_Base_Start_IT(&htim16);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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