/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "mpu6050.h"
#include "parcel_rules.h"
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

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch)
{
  uint8_t byte = (uint8_t)ch;
  if (HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    return EOF;
  }
  return ch;
}

static bool ReadLight(uint16_t *light)
{
  if (HAL_ADC_Start(&hadc1) != HAL_OK) {
    return false;
  }
  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
    HAL_ADC_Stop(&hadc1);
    return false;
  }
  *light = (uint16_t)HAL_ADC_GetValue(&hadc1);
  return HAL_ADC_Stop(&hadc1) == HAL_OK;
}

static void LogEvents(uint32_t now, uint32_t events)
{
  if (events & PARCEL_EVENT_SHOCK) {
    printf("[%lu ms] shock\r\n", (unsigned long)now);
  }
  if (events & PARCEL_EVENT_TILT) {
    printf("[%lu ms] tilt\r\n", (unsigned long)now);
  }
  if (events & PARCEL_EVENT_LIGHT_CHANGE) {
    printf("[%lu ms] light_change\r\n", (unsigned long)now);
  }
  if (events & PARCEL_EVENT_NEARBY_MOTION) {
    printf("[%lu ms] nearby_motion\r\n", (unsigned long)now);
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
  ParcelRules rules;
  ParcelSample sample = {0};
  bool adc_ready;
  bool imu_ready = false;
  bool light_was_valid = false;
  uint32_t last_sample;
  uint32_t last_status;
  uint32_t last_led;
  uint32_t last_imu_retry;
  uint32_t tick_count = 0;

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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  adc_ready = MX_ADC1_Init() == HAL_OK;
  ParcelRules_Init(&rules, PARCEL_TRANSIT);
  last_sample = HAL_GetTick() - 100u;
  last_status = HAL_GetTick();
  last_led = HAL_GetTick();
  last_imu_retry = HAL_GetTick() - 2000u;

  printf("ParcelLog ready; mode=transit\r\n");
  if (!adc_ready) {
    printf("light_sensor_unavailable\r\n");
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    if (now - last_sample >= 100u) {
      ParcelMode mode;
      uint32_t events;
      last_sample = now;
      sample.pir_motion = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET;
      mode = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET
                 ? PARCEL_STORAGE : PARCEL_TRANSIT;
      if (mode != rules.mode) {
        ParcelRules_SetMode(&rules, mode, sample.pir_motion);
        printf("[%lu ms] mode=%s\r\n", (unsigned long)now,
               mode == PARCEL_STORAGE ? "storage" : "transit");
      }

      if (!imu_ready && now - last_imu_retry >= 2000u) {
        last_imu_retry = now;
        imu_ready = Mpu6050_Init(&hi2c1) == HAL_OK;
        if (imu_ready) {
          printf("[%lu ms] mpu6050_ready\r\n", (unsigned long)now);
        } else {
          printf("[%lu ms] mpu6050_unavailable\r\n", (unsigned long)now);
        }
      }
      sample.imu_valid = false;
      if (imu_ready) {
        Mpu6050Acceleration accel;
        if (Mpu6050_ReadAcceleration(&hi2c1, &accel) == HAL_OK) {
          sample.ax = accel.ax;
          sample.ay = accel.ay;
          sample.az = accel.az;
          sample.imu_valid = true;
        } else {
          imu_ready = false;
          last_imu_retry = now;
          printf("[%lu ms] mpu6050_read_error\r\n", (unsigned long)now);
        }
      }

      sample.light_valid = adc_ready && ReadLight(&sample.light);
      if (adc_ready && sample.light_valid != light_was_valid) {
        printf("[%lu ms] light_sensor_%s\r\n", (unsigned long)now,
               sample.light_valid ? "ready" : "read_error");
        light_was_valid = sample.light_valid;
      }
      events = ParcelRules_Update(&rules, &sample);
      LogEvents(now, events);
    }

    if (now - last_status >= 1000u) {
      last_status = now;
      printf("tick %lu [%lu ms] status mode=%s imu=%s ax=%d ay=%d az=%d light=%s%u pir=%u\r\n",
             (unsigned long)++tick_count,
             (unsigned long)now,
             rules.mode == PARCEL_STORAGE ? "storage" : "transit",
             sample.imu_valid ? "ok" : "error",
             sample.imu_valid ? sample.ax : 0,
             sample.imu_valid ? sample.ay : 0,
             sample.imu_valid ? sample.az : 0,
             sample.light_valid ? "" : "error:",
             sample.light_valid ? sample.light : 0u,
             sample.pir_motion ? 1u : 0u);
    }
    if (now - last_led >= 500u) {
      last_led = now;
      HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
    }
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

static HAL_StatusTypeDef MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef channel = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_LOW;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    return HAL_ERROR;
  }

  channel.Channel = ADC_CHANNEL_0;
  channel.Rank = ADC_RANK_CHANNEL_NUMBER;
  channel.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  return HAL_ADC_ConfigChannel(&hadc1, &channel);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x20303E5D;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : User_Button_Pin */
  GPIO_InitStruct.Pin = User_Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(User_Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Led_Pin */
  GPIO_InitStruct.Pin = Led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(Led_GPIO_Port, &GPIO_InitStruct);

  /* PB0 selects storage mode; PB1 reads the PIR output. */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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

#ifdef  USE_FULL_ASSERT
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
