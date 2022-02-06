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
#include "lcd1602_i2c.h"
#include "delay_stm32f1.h"
#include "stm32f10x.h"
#include "state_machine.h"
#include "stm32f10x_tim.h"


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
//uint8_t addr=0xD0;
uint8_t addr=0x4E;
uint8_t data=0x69;
uint16_t pot_rd_data;
uint16_t tmp;
uint32_t dummy_read, wait;
uint8_t send_i2c_flag=1;
uint8_t change_state=0;
uint8_t btn_up_p=0, btn_down_p=0, update_fan=0;
uint8_t lcd_vol_lvl=8;
uint8_t lcd_bal_lvl=7;
uint8_t i;
//char bal_symbol[2]={0x3c,0x3e};
char *bal_symbol="<>";

#define POT0_INC 		0x00
#define POT1_INC 		0x01
#define POT0_DEC 		0x02
#define POT1_DEC 		0x03

#define UP 				0x04
#define DOWN			0x03

#define SPI_DUMMY_BYTE	0xff

uint8_t pot_commands_8bit[]={0x04,0x14,0x08,0x18};

typedef enum{
	POT0_RD,
	POT1_RD,
	POT0_WR,
	POT1_WR
}spi_cmd_16b;

uint16_t spi_cmd_list[]={0x0c,0x1c,0x00,0x10};

//definitions for timer handlers
__IO uint16_t CCR1_Val = 40961;
uint16_t capture = 0;

__IO uint16_t IC2Value = 0;
__IO uint16_t DutyCycle = 0;
__IO uint32_t Frequency = 0;
uint16_t PrescalerValue = 0;
uint32_t tim_freq;
uint8_t IC_PrescalerValue=1;

uint8_t freq_to_print[5]={0x10,0x10,0x10,0x10,0}; //can support maximum 4 digit rpm, 0x10 is blank, 0 is null terminator

TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;
TIM_ICInitTypeDef  TIM_ICInitStructure;

/* definitions related to the state machine*/
enum state_codes cur_state = start;
enum ret_codes rc;
int (*state_fun)(void);

struct transition state_transitions[] = {
	{start, ok,     vol},
	{vol,   ok,     bal},
	{bal,   ok,     fan},
	{fan,   ok, 	vol}};

int (*state[])(void) = { start_state, vol_state, bal_state, fan_state};



RCC_ClocksTypeDef RCC_Clocks;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);
void I2C_Configuration(void);
void TIM_Configuration(void);
void toggle_led(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t value);
void change_lcd_vol(uint8_t direction);
void change_pot_vol(uint8_t direction);
void change_lcd_bal(uint8_t direction);
void RCC_Configuration_HSI_64Mhz_without_USBclock(void);
void SPI_Configuration(void);
uint16_t spi_pot_16b_op(uint8_t cmd, uint8_t data);

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

	/* SPI configuration -------------------------------------------------------*/
	SPI_Configuration();

	/* EXTI configuration ------------------------------------------------------*/
	EXTI_Configuration();

	/*I2C configuration*/
	I2C_Configuration();

	/*Timers configuration*/
	TIM_Configuration();

	DelayInit();

/*Starting the state machine*/
	state_fun = state[cur_state];
	rc = state_fun();
	cur_state = lookup_transitions(cur_state, rc);

	DelayMs(2000);

	state_fun = state[cur_state];
	rc = state_fun();

	//set volume to half for both channels
	pot_rd_data=spi_pot_16b_op(POT0_WR,0x7f);
	pot_rd_data=spi_pot_16b_op(POT1_WR,0x7f);

  while (1)
  {
	  if(change_state==1){

		  pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);

		  cur_state = lookup_transitions(cur_state, rc);
		  state_fun = state[cur_state];
		  rc = state_fun();
		  change_state=0;
	  }
	  if(btn_up_p==1){
		  if(cur_state==vol){
			  change_pot_vol(UP);
			  change_lcd_vol(UP);
			  btn_up_p=0;
		  }
		  if(cur_state==bal){
			  change_lcd_bal(UP);
			  btn_up_p=0;
		  }
		  else
			  btn_up_p=0;
	  }
	  if(btn_down_p==1){
		  if(cur_state==vol){
			  change_pot_vol(DOWN);
			  change_lcd_vol(DOWN);
			  btn_down_p=0;
		  }
		  if(cur_state==bal){
			  change_lcd_bal(DOWN);
			  btn_down_p=0;
		  }
		  else
			  btn_down_p=0;
	  }
	  if(update_fan==1){
		  HD44780_PCF8574_PositionXY(addr, 0, 1);
		  HD44780_PCF8574_DrawString(addr, freq_to_print);
		  update_fan=0;
	  }


  }


}

void EXTI9_5_IRQHandler(void){
	EXTI_ClearITPendingBit(EXTI_Line5);
	EXTI_ClearFlag(EXTI_Line5);
	change_state=1;
}

void EXTI3_IRQHandler(void){
	EXTI_ClearITPendingBit(EXTI_Line3);
	EXTI_ClearFlag(EXTI_Line3);
	btn_down_p=1;
}

void EXTI4_IRQHandler(void){
	EXTI_ClearITPendingBit(EXTI_Line4);
	EXTI_ClearFlag(EXTI_Line4);
	btn_up_p=1;
}


void TIM2_IRQHandler(void){
	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
	  {
	    TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

	    /* Pin PC.06 toggling with frequency = 73.24 Hz */
	    GPIO_WriteBit(GPIOA, GPIO_Pin_6, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_6)));
	    tim_freq=Frequency;
	    int i=3;
	    while(tim_freq){
	    	*(freq_to_print+i)=tim_freq%10+0x30; //the 0x30 is an offset so that we get the ASCII value of the digit
	    	tim_freq/=10;
	    	i--;
	    }
	    if(i>=0)
	    	*(freq_to_print+i)=0x10;
	    if(cur_state==fan){
	    	//HD44780_PCF8574_PositionXY(addr, 0, 1);
	    	//HD44780_PCF8574_DrawString(addr, (char*)freq_to_print);
	    	//HD44780_PCF8574_DrawString(addr, freq_to_print);
	    	update_fan=1;
	    }
	    capture = TIM_GetCapture1(TIM2);
	    TIM_SetCompare1(TIM2, capture + CCR1_Val);
	  }
}

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM1_CC_IRQHandler(void)
{
  /* Clear TIM1 Capture compare interrupt pending bit */
  TIM_ClearITPendingBit(TIM1, TIM_IT_CC2);

  /* Get the Input Capture value */
  IC2Value = TIM_GetCapture2(TIM1);

  if (IC2Value != 0)
  {
    /* Duty cycle computation */
    DutyCycle = (TIM_GetCapture1(TIM1) * 100) / IC2Value;

    /* Frequency computation */
    Frequency = (SystemCoreClock/(IC_PrescalerValue+1)) / IC2Value;
  }
  else
  {
    DutyCycle = 0;
    Frequency = 0;
  }
}
/**
  * @}
  */

/**
  * @}
  */

void RCC_Configuration(void){

	/*Routing clock*/
	/*
	RCC_HSICmd(ENABLE);
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2,RCC_PLLMul_16);
	RCC_PLLCmd(ENABLE);
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	*/

	RCC_Configuration_HSI_64Mhz_without_USBclock();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}
void GPIO_Configuration(void){

	GPIO_InitTypeDef GPIO_InitStructure;

	//start configuration for I2C1 on PORTB

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//start config for mode, up and down buttons
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//Enable GPIO usage of PORTB pin 4 otherwise used as JTAG oin
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	/*
	//config for the tim1c2 pwm input read
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//2s timer monitor
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_Init(GPIOA, &GPIO_InitStructure);
	*/

	//Potentiometer SPI config
	//start configuration for SPI1 on PORTA

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


}

void SPI_Configuration(void){
	SPI_InitTypeDef SPI_InitStruct;

	SPI_InitStruct.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_16;
	SPI_InitStruct.SPI_CPHA=SPI_CPHA_1Edge;
	SPI_InitStruct.SPI_CPOL=SPI_CPOL_Low;
	SPI_InitStruct.SPI_CRCPolynomial=7;
	SPI_InitStruct.SPI_DataSize=SPI_DataSize_8b;
	SPI_InitStruct.SPI_Direction=SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_FirstBit=SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_Mode=SPI_Mode_Master;
	SPI_InitStruct.SPI_NSS=SPI_NSS_Soft;

	//SPI_SSOutputCmd(SPI1,ENABLE);

	SPI_Init(SPI1,&SPI_InitStruct);

	SPI_Cmd(SPI1,ENABLE);
}

void NVIC_Configuration(void){

	NVIC_InitTypeDef NVIC_InitStruct;
	////////////
	NVIC_InitStruct.NVIC_IRQChannel=EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority=0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_Init(&NVIC_InitStruct);
	NVIC_SetPriority(SysTick_IRQn, 0);

	/*Enable core interrupts for up and down buttons*/

	NVIC_InitStruct.NVIC_IRQChannel=EXTI3_IRQn;
	NVIC_Init(&NVIC_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel=EXTI4_IRQn;
	NVIC_Init(&NVIC_InitStruct);

	/* Enable the TIM2 global Interrupt */
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	//NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	//NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;

	NVIC_Init(&NVIC_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = TIM1_CC_IRQn;

	NVIC_Init(&NVIC_InitStruct);
}
void EXTI_Configuration(void){

	EXTI_InitTypeDef EXTI_InitStruct;
	/*EXTI config for mode buton*/

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource5);
	  //continue with EXTI regs
	EXTI_InitStruct.EXTI_Line=EXTI_Line5;
	EXTI_InitStruct.EXTI_LineCmd=ENABLE;
	EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger=EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);

	/*EXTI config for up and down buttons*/
	//down button
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource3);
	//up button
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource4);
	//continue with EXTI regs
	EXTI_InitStruct.EXTI_Line=EXTI_Line3;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.EXTI_Line=EXTI_Line4;
	EXTI_Init(&EXTI_InitStruct);

}

void I2C_Configuration(void){
	I2C_InitTypeDef I2C_InitStruct;
	I2C_InitStruct.I2C_Ack=I2C_Ack_Disable;
	I2C_InitStruct.I2C_AcknowledgedAddress=I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed=100000;
	I2C_InitStruct.I2C_DutyCycle=I2C_DutyCycle_2;
	I2C_InitStruct.I2C_Mode=I2C_Mode_I2C;
	I2C_InitStruct.I2C_OwnAddress1=0;

	//commnet following line for custom values like above;
	//I2C_StructInit(&I2C_InitStruct);

	I2C_Init(I2C1,&I2C_InitStruct);
	I2C_Cmd(I2C1,ENABLE);
}

void TIM_Configuration(void){
	/*
	* This is where TIM1 config as PWM input begins
	*/

		/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = IC_PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
		////////////////////////////////////////////////////

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	TIM_PWMIConfig(TIM1, &TIM_ICInitStructure);

	// Select the TIM3 Input Trigger: TI2FP2
	TIM_SelectInputTrigger(TIM1, TIM_TS_TI2FP2);

	// Select the slave Mode: Reset Mode
	TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);

	// Enable the Master/Slave Mode
	TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable);

	// TIM3 enable counter
	TIM_Cmd(TIM1, ENABLE);

	// Enable the CC2 Interrupt Request
	TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);

/* ---------------------------------------------------------------
	TIM2 Configuration: Output Compare Timing Mode:
	TIM2 counter clock at 6 MHz
	CC1 update rate = TIM2 counter clock / CCR1_Val = 146.48 Hz
	--------------------------------------------------------------- */

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) (SystemCoreClock / 12000000) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);

	/* Output Compare Timing Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = CCR1_Val;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);

	/* TIM IT enable */
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);


}

void toggle_led(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t value){
	if (value == 1)
		GPIO_ResetBits(GPIOC, GPIO_Pin);
	else
		GPIO_SetBits(GPIOC, GPIO_Pin);

}

void change_lcd_vol(uint8_t direction){
	if(direction==UP){
		if(lcd_vol_lvl<=15){
			lcd_vol_lvl+=1;
			HD44780_PCF8574_PositionXY(addr, lcd_vol_lvl-1, 1);
			HD44780_PCF8574_DrawChar(addr,0xFF);
		}
	}
	if(direction==DOWN){
		if(lcd_vol_lvl>=1){
			HD44780_PCF8574_PositionXY(addr, lcd_vol_lvl-1, 1);
			HD44780_PCF8574_DrawChar(addr,0x10);
			lcd_vol_lvl-=1;
		}
	}
}

void change_lcd_bal(uint8_t direction){
	if(direction==UP){
		if(lcd_bal_lvl<=13){
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl, 1);
			HD44780_PCF8574_DrawChar(addr,0x10);
			lcd_bal_lvl+=1;
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl, 1);
			HD44780_PCF8574_DrawString(addr, bal_symbol);
		}
	}
	if(direction==DOWN){
		if(lcd_bal_lvl>=1){
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl+1, 1);
			HD44780_PCF8574_DrawChar(addr,0x10);
			lcd_bal_lvl-=1;
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl, 1);
			HD44780_PCF8574_DrawString(addr, bal_symbol);
		}
	}
}

void change_pot_vol(uint8_t direction){
	if(direction==UP){
		if(lcd_vol_lvl<=15){
			for(i=1;i<=16;i++){
				GPIO_ResetBits(GPIOA,GPIO_Pin_4);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
				//SPI_I2S_SendData(SPI1,0x04);
				SPI_I2S_SendData(SPI1,pot_commands_8bit[POT0_INC]);
				SPI_I2S_SendData(SPI1,pot_commands_8bit[POT1_INC]);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY) == SET);
				GPIO_SetBits(GPIOA, GPIO_Pin_4);
			}
		}
	}
	else{
		if(lcd_vol_lvl>=1){
			for(i=1;i<=16;i++){
				GPIO_ResetBits(GPIOA,GPIO_Pin_4);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
				//SPI_I2S_SendData(SPI1,0x08);
				SPI_I2S_SendData(SPI1,pot_commands_8bit[POT0_DEC]);
				SPI_I2S_SendData(SPI1,pot_commands_8bit[POT1_DEC]);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY) == SET);
				GPIO_SetBits(GPIOA, GPIO_Pin_4);
			}
		}
	}
}

uint16_t spi_pot_16b_op(uint8_t cmd,uint8_t data){
	uint16_t spi_rx_data;
	uint8_t byte1,byte2;
	byte2=data;
	byte1=spi_cmd_list[cmd];
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1,byte1);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	spi_rx_data=SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1,byte2);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	spi_rx_data=SPI_I2S_ReceiveData(SPI1);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY) == SET);
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	return spi_rx_data;
}

void RCC_Configuration_HSI_64Mhz_without_USBclock(void) {
	ErrorStatus HSIStartUpStatus;

	/* RCC system reset(for debug purpose) */
	RCC_DeInit();

	/* activate HSI (Internal High Speed oscillator) */
	RCC_HSICmd(ENABLE);

	/* Wait until RCC_FLAG_HSIRDY is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

	/* Enable Prefetch Buffer */
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

	/* Flash 2 wait state */
	FLASH_SetLatency(FLASH_Latency_2);

	/* HCLK = SYSCLK */
	RCC_HCLKConfig(RCC_SYSCLK_Div1);// HCLK = 64 MHz, AHB

	/* PCLK1 = HCLK/2 */
	RCC_PCLK1Config(RCC_HCLK_Div2); // APB1 = 32 MHz

	/* PCLK2 = HCLK */
	RCC_PCLK2Config(RCC_HCLK_Div1); // APB2 = 64 MHz

	/* PLLCLK = (8MHz/2) * 16 = 64MHz */
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16); // 64 MHz

	/* Enable PLL */
	RCC_PLLCmd(ENABLE);

	/* Wait till PLL is ready */
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

	/* Select PLL as system clock source */
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

	/* Wait till PLL is used as system clock source */
	while(RCC_GetSYSCLKSource() != 0x08);
}



/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
