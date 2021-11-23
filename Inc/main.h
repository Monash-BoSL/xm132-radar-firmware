/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "stm32g0xx_ll_system.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void SystemClock_Config(void);
void MX_USART1_UART_Init(uint32_t);
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MISC_GPIO2_Pin GPIO_PIN_0
#define MISC_GPIO2_GPIO_Port GPIOA
#define MISC_GPIO0_Pin GPIO_PIN_1
#define MISC_GPIO0_GPIO_Port GPIOA
#define MCU_INT_Pin GPIO_PIN_4
#define MCU_INT_GPIO_Port GPIOA
#define A111_SPI_SCK_Pin GPIO_PIN_5
#define A111_SPI_SCK_GPIO_Port GPIOA
#define A111_SPI_MISO_Pin GPIO_PIN_6
#define A111_SPI_MISO_GPIO_Port GPIOA
#define A111_SPI_MOSI_Pin GPIO_PIN_7
#define A111_SPI_MOSI_GPIO_Port GPIOA
#define A111_CS_N_Pin GPIO_PIN_0
#define A111_CS_N_GPIO_Port GPIOB
#define PMU_ENABLE_Pin GPIO_PIN_1
#define PMU_ENABLE_GPIO_Port GPIOB
#define I2C_ADDRESS_Pin GPIO_PIN_10
#define I2C_ADDRESS_GPIO_Port GPIOB
#define MISC_GPIO1_Pin GPIO_PIN_11
#define MISC_GPIO1_GPIO_Port GPIOB
#define A111_ENABLE_Pin GPIO_PIN_8
#define A111_ENABLE_GPIO_Port GPIOA
#define A111_SENSOR_INTERRUPT_Pin GPIO_PIN_3
#define A111_SENSOR_INTERRUPT_GPIO_Port GPIOB
#define A111_SENSOR_INTERRUPT_EXTI_IRQn EXTI2_3_IRQn
#define A111_CTRL_Pin GPIO_PIN_4
#define A111_CTRL_GPIO_Port GPIOB
#define WAKE_UP_Pin GPIO_PIN_5
#define WAKE_UP_GPIO_Port GPIOB
#define WAKE_UP_EXTI_IRQn EXTI4_15_IRQn
#define PS_ENABLE_Pin GPIO_PIN_9
#define PS_ENABLE_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define A111_SPI_HANDLE hspi1
#define DEBUG_UART_HANDLE huart2

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
