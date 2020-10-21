/* USER CODE BEGIN Header */
/*
 * Train3 Timer PWM
 * 目的：PWM制御の確認
 * 確認方法：LEDを2種類の明るさで点滅させる
 * 流れ：スイッチを押すと、50msで反転するPWMタイマがスタートする
 * 1秒ごとにデューティ比が100%と30%と切り替わる
 */
/*
 *
 ******************************************************************************
 * @file : main.c
 * @brief : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 * opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LED.h"
#include <string.h>
#include <stdbool.h>

#include "UART.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SW_ON 0
#define SW_OFF 1
#define MYDEBUG

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim21;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM21_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

#ifdef MYDEBUG
	uint8_t buffer[256] = "";
	uint8_t str_initmeseg[] = "Start My Program\r\n\r\n";
	uint8_t str_test[] = "test";
#endif

	bool SWState = SW_OFF;
	bool SWStateOld = SW_OFF;

	bool flg = false;

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
  MX_TIM2_Init();
  MX_TIM21_Init();
  /* USER CODE BEGIN 2 */

#ifdef MYDEBUG
	// デバッグ用
	HAL_UART_Transmit(&huart2, str_initmeseg, strlen(str_initmeseg), 0xffff);
	// UART_Reciev_DMA init
	msgrx_init(&huart2);
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		// スイッチオン
		SWState = HAL_GPIO_ReadPin(SW_GPIO_Port, SW_Pin);

		// スイッチを押した時だけ
		if (SWStateOld == SW_OFF && SWState == SW_ON) {

			if (flg == false) {			// 1回目のみ実行
				// タイマ スタート
				HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);	// PWMタイマ
				HAL_TIM_Base_Start_IT(&htim21);			// 1秒タイマ

#ifdef MYDEBUG
				// デバッグ用
				printUART("Start Timer : ", 0);
#endif
				flg = true;

			} else {								// 2回目以降実行
				// タイマ ストップ
				HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);
				HAL_TIM_Base_Stop_IT(&htim21);						// 1秒タイマ

#ifdef MYDEBUG
				// デバッグ用
				printUART("Stop Timer : ", 0);
#endif

				flg = false;

			}
		}

		// スイッチの過去状態を更新
		SWStateOld = SWState;

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 320-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM21 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM21_Init(void)
{

  /* USER CODE BEGIN TIM21_Init 0 */

  /* USER CODE END TIM21_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM21_Init 1 */

  /* USER CODE END TIM21_Init 1 */
  htim21.Instance = TIM21;
  htim21.Init.Prescaler = 32000-1;
  htim21.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim21.Init.Period = 250-1;
  htim21.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim21.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim21) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim21, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim21, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM21_Init 2 */

  /* USER CODE END TIM21_Init 2 */

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
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : SW_Pin */
  GPIO_InitStruct.Pin = SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SW_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

// タイマオーバーフロー 割込み コールバック
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	static uint8_t i = 0;

	// キャプチャーコンペアレジスタに設定する値
	static uint16_t l_duty = 0;
	static const uint16_t l_dutyList[] = { 0, 2, 5, 10, 25, 60, 5, 60, 5, 60, 5,
			60, 5, 60, 25, 10, 5, 2, 1, 0 };

	// 1秒タイマの処理
	if (htim == &htim21) {

#define WAY_TWO

#ifndef WAY_TWO
		// l_duty に デューティ比1割分のレジスタの値を増加させる
		l_duty += ((__HAL_TIM_GET_AUTORELOAD(&htim2) + 1) * 0.1);

		// AUTORELOADの値より上回った場合、1に戻す
		// ※変更時に1を引くため、0ではなく1
		if (l_duty - 1 > __HAL_TIM_GET_AUTORELOAD(&htim2)) {
			l_duty = 0;
			// PWMタイマのデューティを変更
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, l_duty);
		} else {
			// PWMタイマのデューティを変更
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, l_duty - 1);
		}

#endif

#ifdef WAY_TWO
		if (i < 20) {
			l_duty = (l_dutyList[i] * __HAL_TIM_GET_AUTORELOAD(&htim2)) / 100;
		} else {
			i = 0;
		}

		i++;

		// PWMタイマのデューティを変更
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, l_duty);

#endif

#ifdef MYDEBUG
		// デバッグ用
		printUART("Timer :  tim21  overflow cnt : ",
				(uint16_t) htim->Instance->CNT);
		// デバッグ用
//		printUART("Timer :  tim21  autoreload : ", __HAL_TIM_GET_AUTORELOAD((&htim21)));

#endif
	}

}

// PWM 割込み コールバック
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {

	// timer2
	if (htim == &htim2) {
		printUART("PWM timer2 :", (uint16_t) htim->Instance->CNT);
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
