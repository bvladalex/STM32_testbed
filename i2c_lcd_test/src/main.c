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
uint8_t a = 1;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);
void toggle_led(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t value);


/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{


	/* System clocks configuration ---------------------------------------------*/
	RCC_Configuration();

	/* NVIC configuration ------------------------------------------------------*/
	NVIC_Configuration();

	/* GPIO configuration ------------------------------------------------------*/
	GPIO_Configuration();

	/* EXTI configuration ------------------------------------------------------*/
	EXTI_Configuration();

  while (1)
  {

  }
}

void EXTI0_IRQHandler(void){
	EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI_ClearFlag(EXTI_Line0);
	toggle_led(GPIOC,GPIO_Pin_13,a);
	a ^= 1;
}

/**
  * @}
  */

/**
  * @}
  */

void RCC_Configuration(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

}
void GPIO_Configuration(void){

	GPIO_InitTypeDef GPIO_InitStructure;

	//start configuration for I2C1 on PORTB

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//start config for LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//start config for btn

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

}

void NVIC_Configuration(void){

	NVIC_InitTypeDef NVIC_InitStruct;
//testt
	GPIOC->BSRR = 0x20000000;
	////////////
	NVIC_InitStruct.NVIC_IRQChannel=EXTI0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_Init(&NVIC_InitStruct);


}
void EXTI_Configuration(void){

	EXTI_InitTypeDef EXTI_InitStruct;

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource0);
	  //continue with EXTI regs
	  //EXTI_StructInit(EXTI_InitStruct);
	EXTI_InitStruct.EXTI_Line=EXTI_Line0;
	EXTI_InitStruct.EXTI_LineCmd=ENABLE;
	EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger=EXTI_Trigger_Rising;
	EXTI_Init(&EXTI_InitStruct);


}
void toggle_led(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t value){
	if (value == 1)
		GPIO_ResetBits(GPIOC, GPIO_Pin);
	else
		GPIO_SetBits(GPIOC, GPIO_Pin);

}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
