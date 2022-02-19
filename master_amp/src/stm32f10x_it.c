/**
  ******************************************************************************
  * @file    GPIO/IOToggle/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include "state_machine.h"
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
/* Private variables ---------------------------------------------------------*/
uint16_t capture = 0;
extern __IO uint16_t CCR1_Val;
extern __IO uint16_t CCR2_Val;
extern __IO uint16_t CCR3_Val;
extern __IO uint16_t CCR4_Val;
extern __IO uint16_t check_temp;
extern __IO uint16_t IC_PrescalerValue;
extern __IO uint16_t DutyCycle;
extern __IO uint32_t Frequency;
uint8_t update_fan_screen=0;
uint8_t update_fan=0;
extern uint8_t lcd_vol_lvl;

extern enum state_codes cur_state;
extern void get_temp(void);

__IO uint16_t IC2Value=0;
__IO uint16_t tmp_IC2Value=0;

__IO uint32_t TIM2Freq,TIM3Freq = 0;
//for tim2 input
__IO uint16_t IC2ReadValue1_tim2 = 0, IC2ReadValue2_tim2 = 0;
__IO uint16_t CaptureNumber_tim2 = 0;
__IO uint32_t Capture_tim2 = 0;
//for tim3 input
__IO uint16_t IC2ReadValue1_tim3 = 0, IC2ReadValue2_tim3 = 0;
__IO uint16_t CaptureNumber_tim3 = 0;
__IO uint32_t Capture_tim3 = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{

}

void TIM3_IRQHandler(void)
{
/*
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0)));
	}
	*/

	if (TIM_GetITStatus(TIM3, TIM_IT_CC3) != RESET){
		/* Clear TIM1 Capture compare interrupt pending	 bit */
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC3);

	    if(CaptureNumber_tim3 == 0)
	    {
	      /* Get the Input Capture value */
	      IC2ReadValue1_tim3 = TIM_GetCapture3(TIM3);
	      CaptureNumber_tim3 = 1;
	    }
	    else if(CaptureNumber_tim3 == 1)
	    {
	      /* Get the Input Capture value */
	      IC2ReadValue2_tim3 = TIM_GetCapture3(TIM3);

	      /* Capture computation */
	      if (IC2ReadValue2_tim3 > IC2ReadValue1_tim3)
	      {
	        Capture_tim3 = (IC2ReadValue2_tim3 - IC2ReadValue1_tim3);
	      }
	      else
	      {
	        Capture_tim3 = ((0xFFFF - IC2ReadValue1_tim3) + IC2ReadValue2_tim3)+1;
	      }
	      /* Frequency computation */
	      TIM3Freq = (uint32_t) SystemCoreClock/(IC_PrescalerValue+1) / Capture_tim3;
	      CaptureNumber_tim3 = 0;
	    }
	}


}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		/*Update temp value*/
		//if(lcd_vol_lvl>10)
		//	TIM1->ARR=25000;
		if(cur_state==fan)
		update_fan_screen=1;
		update_fan=1;
		//get temperature update
		get_temp();
	}

	if (TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET){
		/* Clear TIM1 Capture compare interrupt pending	 bit */
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);

	    if(CaptureNumber_tim2 == 0)
	    {
	      /* Get the Input Capture value */
	      IC2ReadValue1_tim2 = TIM_GetCapture2(TIM2);
	      CaptureNumber_tim2 = 1;
	    }
	    else if(CaptureNumber_tim2 == 1)
	    {
	      /* Get the Input Capture value */
	      IC2ReadValue2_tim2 = TIM_GetCapture2(TIM2);

	      /* Capture computation */
	      if (IC2ReadValue2_tim2 > IC2ReadValue1_tim2)
	      {
	        Capture_tim2 = (IC2ReadValue2_tim2 - IC2ReadValue1_tim2);
	      }
	      else
	      {
	        Capture_tim2 = ((0xFFFF - IC2ReadValue1_tim2) + IC2ReadValue2_tim2)+1;
	      }
	      /* Frequency computation */
	      TIM2Freq = (uint32_t) SystemCoreClock/(IC_PrescalerValue+1) / Capture_tim2;
	      CaptureNumber_tim2 = 0;
	    }
	}
}

void TIM1_CC_IRQHandler(void)
{

}

void TIM4_IRQHandler(void)
{

}
/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
/*
void SysTick_Handler(void)
{
}
*/


/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
