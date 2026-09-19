/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "config.h"
#include "AT25xF2561C.h"
#include "ina219.h"
#include "ov7670.h"

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

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOE
#define SPI3_CS_Pin GPIO_PIN_14
#define SPI3_CS_GPIO_Port GPIOC
#define FMC_NR_Pin GPIO_PIN_0
#define FMC_NR_GPIO_Port GPIOB
#define FMC_RNB_Pin GPIO_PIN_1
#define FMC_RNB_GPIO_Port GPIOB
#define DCMI_GPIO0_Pin GPIO_PIN_8
#define DCMI_GPIO0_GPIO_Port GPIOJ
#define DCMI_GPIO1_Pin GPIO_PIN_9
#define DCMI_GPIO1_GPIO_Port GPIOJ
#define DCMI_GPIO2_Pin GPIO_PIN_10
#define DCMI_GPIO2_GPIO_Port GPIOJ
#define DCMI_GPIO3_Pin GPIO_PIN_11
#define DCMI_GPIO3_GPIO_Port GPIOJ
#define DCMI_MCLK_Pin GPIO_PIN_8
#define DCMI_MCLK_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
