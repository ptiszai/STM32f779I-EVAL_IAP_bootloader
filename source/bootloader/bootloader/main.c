/**
  ******************************************************************************
  * @file    IAP/IAP_Main/Src/main.c 
  * @author  MCD Application Team. Modified by Tiszai Istvan
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016-2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

//http://www.onarm.com/forum/17025/

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "main.h"
#include "menu.h"

/** @addtogroup STM32F7xx_IAP_Main
  * @{
  */

/* Exported variables --------------------------------------------------------*/
/* UART handler declaration */
UART_HandleTypeDef UartHandle;
uint8_t iap_inited = 0;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FLASH_OBProgramInitTypeDef OBInit;
extern pFunction JumpToApplication;
extern uint32_t JumpAddress;

/* Private function prototypes -----------------------------------------------*/
static void IAP_Init(void);
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
void ConfigError_Handler(void);
void Jump_to_app_flash(void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Enable the CPU Cache */
	CPU_CACHE_Enable();

	/* STM32F7xx HAL library initialization:
		- Configure the Flash ART accelerator on ITCM interface
		- Configure the Systick to generate an interrupt each 1 msec
		- Set NVIC Group Priority to 4
		- Global MSP (MCU Support Package) initialization
		*/
	HAL_Init();

	/* Configure the system clock to 216 MHz */
	SystemClock_Config();
  
	/* Configure LED3 */
	BSP_LED_Init(LED3);
  
	/* Get FLASH bank confiuration (single or dual) */
	HAL_FLASHEx_OBGetConfig(&OBInit);
#ifdef USE_DUAL_BANK_FLASH
	if((OBInit.USERConfig & OB_NDBANK_SINGLE_BANK) != OB_NDBANK_DUAL_BANK)
	{
	ConfigError_Handler();
	}
#else /* Use a Single Bank Flash */
	if((OBInit.USERConfig & OB_NDBANK_SINGLE_BANK) != OB_NDBANK_SINGLE_BANK)
	{
	ConfigError_Handler();
	}
#endif /* USE_DUAL_BANK_FLASH */

	/* Initialize Tamper Button mounted on STM32F769I-EVAL board */
	BSP_PB_Init(BUTTON_TAMPER, BUTTON_MODE_GPIO);

	 /* Test if Tamper push-button on STM32F769I-EVAL Board is pressed */
	if (BSP_PB_GetState(BUTTON_TAMPER) == GPIO_PIN_SET)
	{ 
	/* Execute the IAP driver in order to reprogram the Flash */
		IAP_Init();
		/* Display main menu */
		Main_Menu();
	} 
	else
	{
		/* Keep the user application running */
		Jump_to_app_flash();
		ConfigError_Handler();	
	}

	while (1)
	{}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
	
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USART1	| RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;	
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;	
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		ConfigError_Handler();
	}
}

/**
  * @brief  Initialize the IAP: Configure USART.
  * @param  None
  * @retval None
  */
static void IAP_Init(void)
{
  /* USART resources configuration (Clock, GPIO pins and USART registers) ----*/
  /* USART configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
	UartHandle.Init.BaudRate = 115200;
	UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits = UART_STOPBITS_1;
	UartHandle.Init.Parity = UART_PARITY_NONE;
	UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandle.Init.Mode = UART_MODE_RX | UART_MODE_TX;
	BSP_COM_Init(COM1, &UartHandle);	
}

/**
  * @brief  This function is executed in case of Flash configuration error.
  * @param  None
  * @retval None
  */
void ConfigError_Handler(void)
{
  /* Turn ON LED2 */
	BSP_LED_On(LED2);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  Jumping to application FLASH part.
  * @param  None
  * @retval None
  */

void Jump_to_app_flash(void)
{	
	SCB_DisableICache();
	SCB_DisableDCache();

	HAL_MspDeInit();
	HAL_DeInit();

	HAL_RCC_DeInit();
	__HAL_RCC_PWR_CLK_DISABLE();
	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOG_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOI_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
	__HAL_RCC_GPIOF_CLK_DISABLE();

	// Test if user code is programmed starting from address "APPLICATION_ADDRESS"   
	if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000) == 0x20080000)
	{					
		SCB->VTOR = APPLICATION_ADDRESS;
		// Jump to user application 
		JumpAddress = *(__IO uint32_t*)(APPLICATION_ADDRESS + 4);		
		JumpToApplication = (pFunction) JumpAddress;
		// Initialize user application's Stack Pointer 
		__set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);			
		JumpToApplication();
	}	
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
