/** 
 * This implementation is based on Marian Hrinko's implementation for the 
 * AVR Atmega328p, all rights reserved, details below
 * The port for STM32F1 family was made by Vlad Barbalata, 14.09.2021
 * all rights reserved
 *
 * ---------------------------------------------------------------+ 
 * @desc        HD44780 with PCF8574 8-bit I/O expander for TWI 
 * ---------------------------------------------------------------+ 
 *              Copyright (C) 2020 Marian Hrinko.
 *              Written by Marian Hrinko (mato.hrinko@gmail.com)
 *
 * @author      Marian Hrinko
 * @datum       18.11.2020
 * @file        hd44780pcf8574.c
 * @tested      AVR Atmega328p
 *
 * @depend      twi, pcf8574
 * ---------------------------------------------------------------+
 */

// include libraries
//#include <stdio.h>
//#include <util/delay.h>
//#include <avr/io.h>
//#include "twi.h"
#include "lcd1602_i2c.h"
#include "stm32f10x_i2c.h"
#include "delay_stm32f1.h"

/*Private variables*/
uint32_t dummy_read;

// +---------------------------+
// |         Power on          |
// | Wait for more than 15 ms  |   // 15 ms wait
// | after VCC rises to 4.5 V  |
// +---------------------------+
//              |
// +---------------------------+ 
// |  RS R/W DB7 DB6 DB5 DB4   |   
// |   0   0   0   0   1   1   |   // Initial sequence 0x30
// | Wait for more than 4.1 ms |   // 4.1 ms us writing DATA into DDRAM or CGRAM
// +---------------------------+
//              |
// +---------------------------+
// |  RS R/W DB7 DB6 DB5 DB4   |   
// |   0   0   0   0   1   1   |   // Initial sequence 0x30
// | Wait for more than 0.1 ms |   // 100 us writing DATA into DDRAM or CGRAM
// +---------------------------+
//              |
// +---------------------------+
// |  RS R/W DB7 DB6 DB5 DB4   |   // Initial sequence 0x30
// |   0   0   0   0   1   1   |   // 37 us writing DATA into DDRAM or CGRAM 4 us tadd - time after busy flag disapeared
// | Wait for more than 45 us  |   // 37 us + 4 us = 41 us * (270/250) = 45us
// +---------------------------+  
//              |
// +---------------------------+   // 4bit mode 0x20 !!! MUST BE SET TIME, BF CHECK DOESN'T WORK CORRECTLY !!!
// |  RS R/W DB7 DB6 DB5 DB4   |   // 
// |   0   0   0   0   1   0   |   // 37 us writing DATA into DDRAM or CGRAM 4 us tadd - time after busy flag disapeared
// | Wait for more than 45 us  |   // !!! MUST BE SET DELAY TIME, BUSY FLAG CHECK DOESN'T WORK CORRECTLY !!!
// +---------------------------+
//              |
// +---------------------------+
// |  RS R/W DB7 DB6 DB5 DB4   |   // Display off 0x28
// |   0   0   0   0   1   0   |   // 
// |   0   0   1   0   0   0   |   // 
// |    Wait for BF Cleared    |   // Wait for 50us
// +---------------------------+
//              |
// +---------------------------+
// |  RS R/W DB7 DB6 DB5 DB4   |   // Display clear 0x01
// |   0   0   0   0   0   0   |   //
// |   0   0   0   0   0   1   |   //
// |    Wait for BF Cleared    |   // Wait for 50us
// +---------------------------+
//              |
// +---------------------------+
// |  RS R/W DB7 DB6 DB5 DB4   |   // Entry mode set 0x06
// |   0   0   0   0   0   0   |   // 
// |   0   0   0   1   1   0   |   // shift cursor to the left, without text shifting
// |    Wait for BF Cleared    |   // Wait for 50us
// +---------------------------+

/**
 * @desc    LCD init - initialisation routine
 *
 * @param   char
 *
 * @return  char
 */
char HD44780_PCF8574_Init (char addr)
{
  // delay > 15ms
	DelayMs(16);

  // TWI: start
  // -------------------------
  //I2C_LCD_MT_Start();
  //For STM32: just issue start condition: void I2C_GenerateSTART;
  I2C_GenerateSTART(I2C1,ENABLE);
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_SB));

  // TWI: send SLAW
  // -------------------------
  //I2C_LCD_Transmit_SLAW(addr);
  //For STM32: I2C_Send7bitAddress
  I2C_Send7bitAddress(I2C1,addr,I2C_Direction_Transmitter);
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_ADDR));
  dummy_read=I2C1->SR1;
  dummy_read=I2C1->SR2;

  // DB7 BD6 DB5 DB4 P3 E RW RS 
  // DB4=1, DB5=1 / BF cannot be checked in these instructions
  // ---------------------------------------------------------------------
  HD44780_PCF8574_Send_4bits_M4b_I(PCF8574_PIN_DB4 | PCF8574_PIN_DB5);
  // delay > 4.1ms
  DelayMs(5);

  // DB4=1, DB5=1 / BF cannot be checked in these instructions
  // ---------------------------------------------------------------------
  HD44780_PCF8574_Send_4bits_M4b_I(PCF8574_PIN_DB4 | PCF8574_PIN_DB5);
  // delay > 100us
  DelayUs(110);

  // DB4=1, DB5=1 / BF cannot be checked in these instructions
  // ---------------------------------------------------------------------
  HD44780_PCF8574_Send_4bits_M4b_I(PCF8574_PIN_DB4 | PCF8574_PIN_DB5);
  // delay > 45us (=37+4 * 270/250)
  DelayUs(50);

  // DB5=1 / 4 bit mode 0x20 / BF cannot be checked in these instructions
  // ----------------------------------------------------------------------
  HD44780_PCF8574_Send_4bits_M4b_I(PCF8574_PIN_DB5);
  // delay > 45us (=37+4 * 270/250)
  DelayUs(50);

  // TWI Stop
  //I2C_LCD_Stop();
  //For STM32: use stop condition API: void I2C_GenerateSTOP
  I2C_GenerateSTOP(I2C1,ENABLE);

  // 4 bit mode, 2 rows, font 5x8
  HD44780_PCF8574_SendInstruction(addr, HD44780_4BIT_MODE | HD44780_2_ROWS | HD44780_FONT_5x8);

  // display off 0x08 - send 8 bits in 4 bit mode
  HD44780_PCF8574_SendInstruction(addr, HD44780_DISP_OFF);

  // display clear 0x01 - send 8 bits in 4 bit mode
  HD44780_PCF8574_SendInstruction(addr, HD44780_DISP_CLEAR);

  // entry mode set 0x06 - send 8 bits in 4 bit mode
  HD44780_PCF8574_SendInstruction(addr, HD44780_ENTRY_MODE);

  // return success
  return PCF8574_SUCCESS;
}

/**
 * @desc    LCD E pulse
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_E_pulse (char data)
{
  // E pulse
  // ----------------------------------
  //I2C_LCD_Transmit_Byte(data | PCF8574_PIN_E);
  //For STM32: use void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data) API
  /*wait until TX buf is empty than load DR reg with the data*/
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
  I2C_SendData(I2C1,data | PCF8574_PIN_E);
  // delay needs to be implemented
  // PWeh delay time > 450ns
  DelayUs(0.5);
  // E down
  //I2C_LCD_Transmit_Byte(data & ~PCF8574_PIN_E);
  //For STM32: use void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data) API
  /*wait until TX buf is empty than load DR reg with the data*/
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
  I2C_SendData(I2C1,data & ~PCF8574_PIN_E);
  //delay needs to be imlemented
  // PWeh delay time > 450ns
  DelayUs(0.5);
}

/**
 * @desc    LCD send 4bits in 4 bit mode
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_Send_4bits_M4b_I (char data)
{
  // Send upper nibble, E up
  // ----------------------------------
  //I2C_LCD_Transmit_Byte(data);
  /*wait until TX buf is empty than load DR reg with the data*/
  //For STM32:  use void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data) API
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
  I2C_SendData(I2C1,data);
  // E pulse
  HD44780_PCF8574_E_pulse(data);
}

/**
 * @desc    LCD send 8bits in 4 bit mode
 *
 * @param   char
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_Send_8bits_M4b_I (char addr, char data, char annex)
{
  // upper nible with backlight
  char up_nibble = (data & 0xF0) | annex;
  // lower nibble with backlight
  char low_nibble = (data << 4) | annex;

  // TWI: start
  // -------------------------
  //I2C_LCD_MT_Start();
  //For STM32: issue start condition: void I2C_GenerateSTART(I2C_TypeDef* I2Cx, FunctionalState NewState);
  I2C_GenerateSTART(I2C1,ENABLE);
  // TWI: send SLAW
  // -------------------------
  //I2C_LCD_Transmit_SLAW(addr);
  //For STM32: I2C_Send7bitAddress
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_SB));
  I2C_Send7bitAddress(I2C1,addr,I2C_Direction_Transmitter);
  // Send upper nibble, E up
  // ----------------------------------
  //I2C_LCD_Transmit_Byte(up_nibble);
  //For STM32: use void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data) API
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_ADDR));
  dummy_read=I2C1->SR1;
  dummy_read=I2C1->SR2;
  I2C_SendData(I2C1,up_nibble);
  // E pulse
  HD44780_PCF8574_E_pulse(up_nibble);

  // Send lower nibble, E up
  // ----------------------------------
  //I2C_LCD_Transmit_Byte(low_nibble);
  //For STM32: use void I2C_SendData(I2C_TypeDef* I2Cx, uint8_t Data) API
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));
  I2C_SendData(I2C1,low_nibble);
  // E pulse
  HD44780_PCF8574_E_pulse(low_nibble);
  //wait for last transfer to finish before sending stop condition
  while(!I2C_GetFlagStatus(I2C1,I2C_FLAG_TXE));

  // TWI Stop
  //I2C_LCD_Stop();
  //For STM32: use stop condition API: void I2C_GenerateSTOP
  I2C_GenerateSTOP(I2C1,ENABLE);
}

/**
 * @desc    LCD check BF
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_CheckBF (char addr)
{
}

/**
 * @desc    LCD Send instruction 8 bits in 4 bits mode
 *
 * @param   char
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_SendInstruction (char addr, char instruction)
{
  // send instruction
  HD44780_PCF8574_Send_8bits_M4b_I(addr, instruction, PCF8574_PIN_P3);
  // check BF
  //HD44780_PCF8574_CheckBF(addr);
  DelayMs(50);
}

/**
 * @desc    LCD Send data 8 bits in 4 bits mode
 *
 * @param   char
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_SendData (char addr, char data)
{
  // send data
  // data/command -> pin RS High
  // backlight -> pin P3
  HD44780_PCF8574_Send_8bits_M4b_I(addr, data, PCF8574_PIN_RS | PCF8574_PIN_P3);
  // check BF
  //HD44780_PCF8574_CheckBF(addr);
  DelayMs(50);
}

/**
 * @desc    LCD Go to position x, y
 *
 * @param   char
 * @param   char
 * @param   char
 *
 * @return  char
 */
char HD44780_PCF8574_PositionXY (char addr, char x, char y)
{
  if (x > HD44780_COLS || y > HD44780_ROWS) {
    // error
    return PCF8574_ERROR;
  }
  // check which row
  if (y == 0) {
    // send instruction 1st row
    HD44780_PCF8574_SendInstruction(addr, (HD44780_POSITION | (HD44780_ROW1_START + x)));
  } else if (y == 1) {
    // send instruction 2nd row
    HD44780_PCF8574_SendInstruction(addr, (HD44780_POSITION | (HD44780_ROW2_START + x)));
  }
  // success
  return PCF8574_SUCCESS;
}

/**
 * @desc    LCD display clear
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_DisplayClear (char addr)
{
  // Diplay clear
  HD44780_PCF8574_SendInstruction(addr, HD44780_DISP_CLEAR);
}

/**
 * @desc    LCD display on
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_DisplayOn (char addr)
{
  // send instruction - display on
  HD44780_PCF8574_SendInstruction(addr, HD44780_DISP_ON);
}

/**
 * @desc    LCD cursor on, display on
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_CursorOn (char addr)
{
  // send instruction - cursor on
  HD44780_PCF8574_SendInstruction(addr, HD44780_CURSOR_ON);
}

/**
 * @desc    LCD cursor blink, cursor on, display on
 *
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_CursorBlink (char addr)
{
  // send instruction - Cursor blink
  HD44780_PCF8574_SendInstruction(addr, HD44780_CURSOR_BLINK);
}

/**
 * @desc    LCD draw char
 *
 * @param   char
 * @param   char
 *
 * @return  void
 */
void HD44780_PCF8574_DrawChar (char addr, char character)
{
  // Draw character
  HD44780_PCF8574_SendData(addr, character);
}

/**
 * @desc    LCD draw string
 *
 * @param   char
 * @param   char *
 *
 * @return  void
 */
void HD44780_PCF8574_DrawString (char addr, char *str)
{
  unsigned short int i = 0;
  // loop through chars
  while (str[i] != '\0') {
    // draw individual chars
    HD44780_PCF8574_DrawChar(addr, str[i++]);
  }
}

/**
 * @desc    Shift cursor / display to left / right
 *
 * @param   char addr
 * @param   char item {HD44780_CURSOR; HD44780_DISPLAY}
 * @param   char direction {HD44780_RIGHT; HD44780_LEFT}
 *
 * @return  char
 */
char HD44780_PCF8574_Shift (char addr, char item, char direction)
{
  // check if item is cursor or display or direction is left or right
  if ((item != HD44780_DISPLAY) && (item != HD44780_CURSOR)) {
    // error
    return PCF8574_ERROR;
  }
  // check if direction is left or right
  if ((direction != HD44780_RIGHT) && (direction != HD44780_LEFT)) {
    // error
    return PCF8574_ERROR;
  }
  // cursor shift
  if (item == HD44780_CURSOR) {
    // right shift
    if (direction == HD44780_RIGHT) {
      // shit cursor to right
      HD44780_PCF8574_SendInstruction(addr, HD44780_SHIFT | HD44780_CURSOR | HD44780_RIGHT);
    } else {
      // shit cursor to left
      HD44780_PCF8574_SendInstruction(addr, HD44780_SHIFT | HD44780_CURSOR | HD44780_LEFT);
    }
  // display shift
  } else {
    // right shift
    if (direction == HD44780_RIGHT) {
      // shit display to right
      HD44780_PCF8574_SendInstruction(addr, HD44780_SHIFT | HD44780_DISPLAY | HD44780_RIGHT);
    } else {
      // shit display to left
      HD44780_PCF8574_SendInstruction(addr, HD44780_SHIFT | HD44780_DISPLAY | HD44780_LEFT);
    }
  }
  // success
  return PCF8574_SUCCESS;
}
