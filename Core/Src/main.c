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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>

#include "lwjson/lwjson.h"
#include "led_ws2813b.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	LED_OFF = 0,
	LED_ON
}led_state_t;

typedef struct
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} led_rgb_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t rx_buffer[256];
bool data_received = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
  /* USER CODE BEGIN 2 */
  led_state_t led_state = LED_OFF;
  uint32_t blink_time = 1000;
  led_rgb_t led[NUMBER_OF_LEDS] = {0};
  uint32_t stimer = 0;
  uint32_t nbr;

  static lwjson_token_t tokens[128];
  static lwjson_t lwjson;

  lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
  led_init();

  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, 256);

  stimer = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (data_received == true)
	  {
		  data_received = false;

		  /*
			{
			  "BLINK_TIME":100,
			  "LED_COLOR":
			  [
				{
				  "NUMBER":0,
				  "RED":255,
				  "GREEN":0,
				  "BLUE":0
				},
				{
				  "NUMBER":1,
				  "RED":0,
				  "GREEN":255,
				  "BLUE":0
				},
				{
				  "NUMBER":2,
				  "RED":0,
				  "GREEN":0,
				  "BLUE":255
				},
				{
				  "NUMBER":3,
				  "RED":255,
				  "GREEN":255,
				  "BLUE":255
				}
			  ]
			}

			{"BLINK_TIME":100,"LED_COLOR":[{"NUMBER":0,"RED":255,"GREEN":0,"BLUE":0},{"NUMBER":1,"RED":0,"GREEN":255,"BLUE":0},{"NUMBER":2,"RED":0,"GREEN":0,"BLUE":255},{"NUMBER":3,"RED":255,"GREEN":255,"BLUE":255}]}
		  */

		  if (lwjson_parse(&lwjson, (char*)rx_buffer) == lwjsonOK)
		  {
			  const lwjson_token_t* t;
			  const lwjson_token_t* tmp;

			  if ((t = lwjson_find(&lwjson, "BLINK_TIME")) != NULL)
			  {
				  blink_time = lwjson_get_val_int(t);
			  }

			  t = lwjson_find(&lwjson, "LED_COLOR");
			  tmp = lwjson_get_first_child(t);

			  for(uint32_t i=0; i<NUMBER_OF_LEDS; i++)
			  {
					t = lwjson_find_ex(&lwjson, tmp, "NUMBER");

					if (t != NULL)
					{
						nbr = lwjson_get_val_int(t);
					}
					else
					{
						break;
					}

					t = lwjson_find_ex(&lwjson, tmp, "RED");

					if (t != NULL)
					{
						led[nbr].red = lwjson_get_val_int(t);
					}

					t = lwjson_find_ex(&lwjson, tmp, "GREEN");

					if (t != NULL)
					{
						led[nbr].green = lwjson_get_val_int(t);
					}

					t = lwjson_find_ex(&lwjson, tmp, "BLUE");

					if (t != NULL)
					{
						led[nbr].blue = lwjson_get_val_int(t);
					}

					tmp = tmp->next;
			  }

			  lwjson_free(&lwjson);
		  }
	  }

	  if ((HAL_GetTick() - stimer) > blink_time)
	  {
			stimer = HAL_GetTick();

			if (led_state == LED_OFF)
			{
				led_state = LED_ON;

				for (int i=0; i<NUMBER_OF_LEDS; i++)
					led_set_one_led_colors(i, led[i].green, led[i].red, led[i].blue);

				led_send_led_colors();
			}
			else if (led_state == LED_ON)
			{
				led_state = LED_OFF;

				led_set_all_led_colors(0, 0, 0);
				led_send_led_colors();
			}
	  }

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART2)
	{
		rx_buffer[Size] = '\0';
		data_received = true;
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, 256);
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if(TIM2 == htim->Instance)
	{
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
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
