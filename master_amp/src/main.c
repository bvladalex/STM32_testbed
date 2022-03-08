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
uint8_t btn_up_p=0, btn_down_p=0;
uint8_t lcd_vol_lvl=8;
uint8_t lcd_bal_lvl=8;
uint8_t i;
uint8_t check_temp=0;
uint16_t RPM_fan1=30000;
uint16_t RPM_fan2=640;
//char bal_symbol[2]={0x3c,0x3e};
char *bal_symbol="<>";
uint16_t DutyCycle=0;
uint32_t Frequency=0;
uint8_t tc74_sensor_add_7b=0x48;
uint8_t temp;

extern __IO uint32_t TIM2Freq,TIM3Freq;
extern uint8_t update_fan_screen;
extern uint8_t update_fan;

#define POT0_INC 		0x00
#define POT1_INC 		0x01
#define POT0_DEC 		0x02
#define POT1_DEC 		0x03

#define UP 				0x04
#define DOWN			0x03

#define SPI_DUMMY_BYTE	0xff

uint8_t pot_commands_8bit[]={0x04,0x14,0x08,0x18};
//uint16_t arr_val[]={30000, 25000, 20000, 17000};
uint16_t arr_val[]={200, 315, 470, 600};

typedef enum{
	t_state1,
	t_state2,
	t_state3,
	t_state4
}temp_states;

typedef enum{
	POT0_RD,
	POT1_RD,
	POT0_WR,
	POT1_WR
}spi_cmd_16b;

uint16_t spi_cmd_list[]={0x0c,0x1c,0x00,0x10};

//definitions for timer handlers
__IO uint16_t CCR1_Val_t1c1 = 800;
__IO uint16_t CCR3_Val_t4c3 = 150;

uint16_t PrescalerValue = 0;
uint32_t tim_freq;
uint16_t IC_PrescalerValue=1940;
//uint16_t IC_PrescalerValue=0; //for better precision, just for tests

uint8_t freq_to_print[5]={0x10,0x10,0x10,0x10,0}; //can support maximum 4 digit rpm, 0x10 is blank, 0 is null terminator

TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;
TIM_ICInitTypeDef  TIM_ICInitStructure;

/* definitions related to the state machine*/
enum state_codes cur_state = start;
enum ret_codes rc;
int (*state_fun)(void);
temp_states t_state=t_state1;

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
uint8_t* convert2pr(uint16_t freq);
uint16_t spi_pot_16b_op(uint8_t cmd, uint8_t data);
void update_fan_set(TIM_TypeDef* TIMx1, TIM_TypeDef* TIMx2, uint8_t t_state_no);
void update_cool_state(void);
void get_temp(void);

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{

	uint16_t master_vol_lvl,new_pot_lvl; //for balance control

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
	pot_rd_data=spi_pot_16b_op(POT0_WR,0x80);
	pot_rd_data=spi_pot_16b_op(POT1_WR,0x80);

  while (1)
  {
	  if(change_state==1){

		  //pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);

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
			  //increase balance to right logic starts here
			  if (lcd_bal_lvl>8){
				  master_vol_lvl=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				  new_pot_lvl=(master_vol_lvl*(15-lcd_bal_lvl))/7;
				  spi_pot_16b_op(POT1_WR,new_pot_lvl);
			  }
			  else if(lcd_bal_lvl<8){
				  master_vol_lvl=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);
				  new_pot_lvl=(master_vol_lvl*((lcd_bal_lvl-1))/7);
				  spi_pot_16b_op(POT0_WR,new_pot_lvl);
			  }
			  else if(lcd_bal_lvl==8){
				  master_vol_lvl=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);
				  spi_pot_16b_op(POT0_WR,master_vol_lvl);
			  }
			  btn_up_p=0;
		  }
		  /////////////////////////////////////////
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
			  //increase balance to left logic starts here
			  if (lcd_bal_lvl>8){
				  master_vol_lvl=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				  new_pot_lvl=(master_vol_lvl*((15-lcd_bal_lvl))/7);
				  spi_pot_16b_op(POT1_WR,new_pot_lvl);
			  }
			  else if(lcd_bal_lvl<8){
				  master_vol_lvl=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);
				  new_pot_lvl=(master_vol_lvl*((lcd_bal_lvl-1))/7);
				  spi_pot_16b_op(POT0_WR,new_pot_lvl);
			  }
			  else if(lcd_bal_lvl==8){
				  master_vol_lvl=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				  spi_pot_16b_op(POT1_WR,master_vol_lvl);
			  }
			  btn_down_p=0;
		  }
			  /////////////////////////////////////////
		  else
			  btn_down_p=0;
	  }
	  if(update_fan_screen==1){
		  HD44780_PCF8574_PositionXY(addr, 8, 0);
		  //HD44780_PCF8574_DrawString(addr, convert2pr(lcd_vol_lvl));
		  HD44780_PCF8574_DrawString(addr, convert2pr(temp));
		  HD44780_PCF8574_PositionXY(addr, 0, 1);
		  HD44780_PCF8574_DrawString(addr, convert2pr(TIM2Freq*30));
		  HD44780_PCF8574_PositionXY(addr, 6, 1);
		  HD44780_PCF8574_DrawString(addr, convert2pr(TIM3Freq*30));
		  //update_cool_state();
		  update_fan_screen=0;
	  }
	  if(update_fan==1){
		  update_cool_state();
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

	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);
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

	//PORTB bit8 will TIM4C3 PWM output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*2s timer control output -> temporary
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	*/

	//pwm input for tim3c3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
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

	//PORTA pin 4 is SPI NSS and pin 8 is being toggled by TIM1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//pwm config for tim1c
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//pwm input for tim2c2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
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

	NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;

	NVIC_Init(&NVIC_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = TIM4_IRQn;

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
	* This is where TIM2C2 config as IC and timebase begins
	*/

		/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = IC_PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
		////////////////////////////////////////////////////

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	TIM_ICInit(TIM2,&TIM_ICInitStructure);

	// Enable the TIM2 counter overflow Interrupt Request
	TIM_ITConfig(TIM2, TIM_IT_CC2 | TIM_IT_Update, ENABLE);

	// TIM2 enable counter
	TIM_Cmd(TIM2, ENABLE);

	/* ---------------------------------------------------------------
	TIM3 Configuration: Output Compare Timing Mode:
	TIM3 counter clock at xx MHz
	CC1 update rate = TIM1 counter clock / Tim_per = xxkHz
		--------------------------------------------------------------- */

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	TIM_ICInit(TIM3,&TIM_ICInitStructure);

	// Enable the TIM2 counter overflow Interrupt Request
	TIM_ITConfig(TIM3, TIM_IT_CC3, ENABLE);

	// TIM2 enable counter
	TIM_Cmd(TIM3, ENABLE);

/* ---------------------------------------------------------------
	TIM1 Configuration: Output Compare Timing Mode:
	TIM1 counter clock at 64 MHz
	CC1 update rate = TIM1 counter clock / Tim_per = 25kHz
	--------------------------------------------------------------- */

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) (SystemCoreClock / 64000000) - 1;

	/* Time base configuration */
	//TIM_TimeBaseStructure.TIM_Period = 2560; //value for fan pwm
	TIM_TimeBaseStructure.TIM_Period = RPM_fan1; //value for fan rpm
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* Prescaler configuration */
	//TIM_PrescalerConfig(TIM1, PrescalerValue, TIM_PSCReloadMode_Immediate);

	/* Output Compare Timing Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	//TIM_OCInitStructure.TIM_Pulse = CCR1_Val_t1c1; //value for fan pwm
	TIM_OCInitStructure.TIM_Pulse = 15000; //value for fan rpm
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	/* enable the Brake and Dead Zone Registers for some reason :/ */
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
	///////////////////start config for 2s isr///
	/* Output Compare Timing Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = CCR1_Val_t1c1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC2Init(TIM1, &TIM_OCInitStructure);

	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable);

	/* TIM IT enable */
	//TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);

	/* TIM1 enable counter */
	TIM_Cmd(TIM1, ENABLE);

/* ---------------------------------------------------------------
	TIM4 Configuration: PWM output mode on C3:
	TIM2 counter clock at 8 MHz
	ARR val is 500
	Freq = Timer freq/arr = 8Mhz / 500 = 16khz
	Duty cycle = ccrx/arr value = 300 / 500 *100 = 60%
	--------------------------------------------------------------- */

	/* Time base configuration */
	//PrescalerValue=63;
	PrescalerValue = (uint16_t) (64000000 / 16000000) - 1;
	//TIM_TimeBaseStructure.TIM_Period = 25600; //value for fan pwm
	TIM_TimeBaseStructure.TIM_Period = RPM_fan2; //value for fan rpm-> loop test
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	/* Output Compare Timing Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	//TIM_OCInitStructure.TIM_Pulse = CCR3_Val_t4c3; //value for fan pwm
	TIM_OCInitStructure.TIM_Pulse = 200; //value for fan rpm -> loop test
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC3Init(TIM4, &TIM_OCInitStructure);

	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	/*
	////////////////////finish pwm out/////
	/////////begin 2 second fan update counter///
	// Output Compare Timing Mode configuration: Channel1 //
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 50;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);

	TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);
	*/
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	//TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

	/* TIM4 enable counter */
	TIM_Cmd(TIM4, ENABLE);

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
		if(lcd_bal_lvl<=14){
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl-1, 1);
			HD44780_PCF8574_DrawChar(addr,0x10);
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl, 1);
			HD44780_PCF8574_DrawString(addr, bal_symbol);
			lcd_bal_lvl+=1;
		}
	}
	if(direction==DOWN){
		if(lcd_bal_lvl>=2){
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl, 1);
			HD44780_PCF8574_DrawChar(addr,0x10);
			lcd_bal_lvl-=1;
			HD44780_PCF8574_PositionXY(addr, lcd_bal_lvl-1, 1);
			HD44780_PCF8574_DrawString(addr, bal_symbol);
		}
	}
}


void change_pot_vol(uint8_t direction){
	uint8_t decrement=16;
	if(direction==UP){
		if(lcd_vol_lvl<=15){
			if(lcd_bal_lvl==8){
				GPIO_ResetBits(GPIOA,GPIO_Pin_4);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
				pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT0_WR,pot_rd_data+16);
				spi_pot_16b_op(POT1_WR,pot_rd_data+16);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY) == SET);
				GPIO_SetBits(GPIOA, GPIO_Pin_4);
			}
			else if(lcd_bal_lvl<8){
				pot_rd_data=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT1_WR,pot_rd_data+16);
				spi_pot_16b_op(POT0_WR,((pot_rd_data+16)*(lcd_bal_lvl-1))/7);
			}
			else if(lcd_bal_lvl>8){
				pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT0_WR,pot_rd_data+16);
				spi_pot_16b_op(POT1_WR,((pot_rd_data+16)*(15-lcd_bal_lvl))/7);
			}
		}
	}
	else{
		if(lcd_vol_lvl>=1){
			if(lcd_bal_lvl==8){
				GPIO_ResetBits(GPIOA,GPIO_Pin_4);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) == RESET);
				pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT0_WR,pot_rd_data-decrement);
				spi_pot_16b_op(POT1_WR,pot_rd_data-decrement);
				while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY) == SET);
				GPIO_SetBits(GPIOA, GPIO_Pin_4);
			}
			else if(lcd_bal_lvl<8){
				pot_rd_data=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);//
				spi_pot_16b_op(POT1_WR,pot_rd_data-decrement);
				//pot_rd_data=spi_pot_16b_op(POT1_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT0_WR,((pot_rd_data-16)*(lcd_bal_lvl-1))/7);
			}
			else if(lcd_bal_lvl>8){
				pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);//
				spi_pot_16b_op(POT0_WR,pot_rd_data-decrement);
				//pot_rd_data=spi_pot_16b_op(POT0_RD,SPI_DUMMY_BYTE);
				spi_pot_16b_op(POT1_WR,((pot_rd_data-16)*(15-lcd_bal_lvl))/7);
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


uint8_t* convert2pr(uint16_t freq){

	//uint8_t freq_to_print[5]={0x10,0x10,0x10,0x10,0}; //can support maximum 4 digit rpm, 0x10 is blank, 0 is null terminator
	for (i=0;i<=3;i++)
		freq_to_print[i]=0x10;
	i=3;
    while(freq){
    	*(freq_to_print+i)=freq%10+0x30; //the 0x30 is an offset so that we get the ASCII value of the digit
    	freq/=10;
    	i--;
    }
    if(i>=0)
    	*(freq_to_print+i)=0x10;

	return freq_to_print;
}

void update_fan_set(TIM_TypeDef* TIMx1, TIM_TypeDef* TIMx2, uint8_t t_state_no){
	TIM_OCInitStructure.TIM_Pulse = arr_val[t_state_no];
	TIM_OC3Init(TIMx2, &TIM_OCInitStructure);
	//TIMx1->ARR=arr_val[t_state_no];
	//TIMx2->ARR=arr_val[t_state_no];
}

void update_cool_state(void){
	if (t_state==t_state1){
		if(lcd_vol_lvl>5){
			t_state=t_state2;
			update_fan_set(TIM1, TIM4, t_state2);
		}
	}
	if (t_state==t_state2){
		if(lcd_vol_lvl>8){
			t_state=t_state3;
			update_fan_set(TIM1, TIM4, t_state3);
		}
		if(lcd_vol_lvl<4){
			t_state=t_state1;
			update_fan_set(TIM1, TIM4, t_state1);
		}
	}
	if (t_state==t_state3){
		if(lcd_vol_lvl>11){
			t_state=t_state4;
			update_fan_set(TIM1, TIM4, t_state4);
		}
		if(lcd_vol_lvl<6){
			t_state=t_state2;
			update_fan_set(TIM1, TIM4, t_state2);
		}
	}
	if (t_state==t_state4){
		if(lcd_vol_lvl<9){
			t_state=t_state3;
			update_fan_set(TIM1, TIM4, t_state3);
		}
	}
}

void get_temp(void){
	//while(I2C_GetFlagStatus(I2C1,I2C_FLAG_BUSY));
	I2C_GenerateSTART(I2C1,ENABLE);
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_SB));

	//phase1 -> send add byet with write bit
	I2C_Send7bitAddress(I2C1,(tc74_sensor_add_7b<<1),I2C_Direction_Transmitter);
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_ADDR));
	dummy_read=I2C1->SR1;
	dummy_read=I2C1->SR2;

	//phase2 -> send data byte representing temp reg
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
	I2C_SendData(I2C1,0x00);
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_BTF));
	//stop for dev and reg select
	I2C_GenerateSTOP(I2C1, ENABLE);

	//start for read portion
	while(I2C_GetFlagStatus(I2C1,I2C_FLAG_BUSY));
	I2C_GenerateSTART(I2C1,ENABLE);
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_SB));

	//send another address byte due to flow direction change
	I2C_Send7bitAddress(I2C1,(tc74_sensor_add_7b<<1),I2C_Direction_Receiver);
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_ADDR));
	I2C_AcknowledgeConfig(I2C1,DISABLE);
	dummy_read=I2C1->SR1;
	dummy_read=I2C1->SR2;
	I2C_GenerateSTOP(I2C1, ENABLE);
	//phase4-send dummy byte for clocking in temp read
	//I2C_SendData(I2C1,0x00);
	//while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
	//I2C_ReceiveData(I2C1);


	//receive, first wait for buff to have something
	while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_RXNE));
	//*temp=I2C_ReceiveData(I2C1);
	temp=I2C_ReceiveData(I2C1);
	//generate stop
	//I2C_GenerateSTOP(I2C1,ENABLE);
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
