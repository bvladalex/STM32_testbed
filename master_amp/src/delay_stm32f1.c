#include "delay_stm32f1.h"
#include "misc.h"
#include "core_cm3.h"
 
// For store tick counts in us
static __IO uint32_t usTicks;
static __IO uint32_t systick_counter=0;
 
// SysTick_Handler function will be called every 1 us
void SysTick_Handler()
{
	/*
    if (usTicks != 0)
    {
        usTicks--;
    }
    */
	systick_counter++;
}
 
void DelayInit()
{
    // Update SystemCoreClock value
    SystemCoreClockUpdate();
    // Configure the SysTick timer to overflow every 1 us
    //SysTick_Config(SystemCoreClock/240000);
    //SysTick_Config(32);
    SysTick_Config(SystemCoreClock/10000);
}
 
void Delay100us(uint32_t hund_us){
	while(hund_us>systick_counter);
	systick_counter=0;
}

void DelayUs(uint32_t us)
{
	uint32_t micros;
	micros=us;
	/*
    // Reload us value
    usTicks = us;
    // Wait until usTick reach zero
    while (usTicks);
    */
	while (micros>systick_counter);
	systick_counter = 0;
}
 
void DelayMs(uint32_t ms)
{
    // Wait until ms reach zero
    while (ms--)
    {
        // Delay 1ms
        //DelayUs(1000);
    	Delay100us(10);
    }
}
