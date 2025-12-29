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
#include "fatfs.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define X_STEP 20
#define Y_STEP 20
#define X_MAX (4440 / X_STEP)
#define Y_MAX (4440 / Y_STEP)
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

FATFS fs;
FIL fil;
uint8_t rv;
volatile uint8_t usart_flag = 0;
volatile uint8_t x_motor_flag=0, y_motor_flag=0;
uint16_t wh[2];
volatile uint32_t x_go_step, y_go_step;
uint32_t x_steps, y_steps;
uint32_t fnum;
uint32_t buf_size;
char buf[1024];
uint8_t line[500];


void x_go(uint32_t steps /* steps */, uint8_t dir /* 0: forward 1: backward */)
{
	if (0 == dir)
	{
		if (X_MAX == x_steps) return ;
		x_steps += steps;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	}
	else
	{
		if (0 == x_steps) return ;
		x_steps -= steps;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	}
	
	x_go_step = steps * X_STEP;
	
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	while (0 == x_motor_flag);
	x_motor_flag = 0;
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
}

void y_go(uint32_t steps, uint8_t dir)
{
	if (0 == dir)
	{
		if (Y_MAX == y_steps) return ;
		y_steps += steps;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	}
	else
	{
		if (0 == y_steps) return ;
		y_steps -= steps;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	}
	
	y_go_step = steps * Y_STEP;
	
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	while (0 == y_motor_flag);
	y_motor_flag = 0;
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
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
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
	
	__HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	
	rv = f_mount(&fs, "", 1);
	
//	__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE); // test
//	x_steps = X_STEP; // test
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); // test
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		outer_loop :
//		y_go(1, 0); // test
		HAL_UART_Transmit(&huart1, (uint8_t *)"ENTER FILE NAME\n", 16, HAL_MAX_DELAY);
		HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)buf, 1023);
		while (0 == usart_flag);
		usart_flag = 0;
		
		if (0 == strncmp("modify_light", buf, 12))
		{
			HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
			
			while (1)
			{
				HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)buf, 1023);
				
				if (0 == strncmp("quit_light", buf, 10))
				{
					goto outer_loop;
				}
			}
		}
		
		HAL_UART_Transmit(&huart1, (uint8_t *)buf, buf_size, HAL_MAX_DELAY);

		rv = f_open(&fil, buf, FA_READ);
		switch (rv)
		{
			case FR_NO_FILE :
				HAL_UART_Transmit(&huart1, (uint8_t *)"No such file!\n", 14, HAL_MAX_DELAY);
				break;
			case FR_OK :
				break;
			default :
				HAL_UART_Transmit(&huart1, (uint8_t *)"SOME ERROR, check link then restart\n", 36, HAL_MAX_DELAY);
				while (1);
		}

		f_read(&fil, wh, sizeof(uint16_t) * 2, &fnum);
		
		if (X_MAX < wh[0] || Y_MAX < wh[1])
		{
			fnum = sprintf(buf, "%hd %hd too big, can't print\n", wh[0], wh[1]);
			HAL_UART_Transmit(&huart1, (uint8_t *)buf, fnum, HAL_MAX_DELAY);
			continue;
		}

		fnum = sprintf((char *)line, "%s width and height : %hd %hd\r\n[Y/N]*\r\n", buf, wh[0], wh[1]);
		HAL_UART_Transmit(&huart1, line, fnum, HAL_MAX_DELAY);
		HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)buf, 1023);
		while (0 == usart_flag);
		usart_flag = 0;
		if ('Y' != buf[0]) continue;
		HAL_UART_Transmit(&huart1, (uint8_t *)"LET'S GO!!!!\r\n", 14, HAL_MAX_DELAY);
		
		__HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
		for (int i = 0; i < wh[1]; ++i)
		{
			f_read(&fil, line, wh[0] * sizeof(uint8_t), &fnum);
			
			for (int j = 0; j < wh[0]; ++j)
			{
				if (line[j])
				{
					HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
				}
				x_go(1, 0);
				HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
			}
			
			for (int j = wh[0]-1; 0 <= j; --j)
			{
				if (line[j])
				{
					HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
				}
				x_go(1, 1);
				HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
			}
			y_go(1, 0);
		}
		y_go(y_steps, 1);
		__HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
		
		f_close(&fil);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
