/*
 * main.c
 *
 *  Created on: Jul 21, 2021
 *      Author: Vlad
 */

/* Includes ------------------------------------------------------------------*/
//#include "stm32f10x.h"
//#include "stm32f10x_gpio.h"
//#include "stm32f10x_rcc.h"
//#include "stm32f10x_conf.h"
//#include "stm32_eval.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_i2c.h"

//#include "stm32f10x.h"

/** @addtogroup STM32F10x_StdPeriph_Examples
  * @{
  */

/** @addtogroup GPIO_IOToggle
  * @{
  */

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);


/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{


	/* System clocks configuration ---------------------------------------------*/
	  RCC_Configuration();


	  /* GPIO configuration ------------------------------------------------------*/
	  GPIO_Configuration();

  while (1)
  {

  }
}



/**
  * @}
  */

/**
  * @}
  */

void RCC_Configuration(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

}
void GPIO_Configuration(void){

	GPIO_InitTypeDef GPIO_InitStructure;

	//start configuration for SPI1 on PORTA

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
