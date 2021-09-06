################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/core_cm3.c \
../src/main.c \
../src/misc.c \
../src/stm32f10x_adc.c \
../src/stm32f10x_bkp.c \
../src/stm32f10x_can.c \
../src/stm32f10x_cec.c \
../src/stm32f10x_crc.c \
../src/stm32f10x_dac.c \
../src/stm32f10x_dbgmcu.c \
../src/stm32f10x_dma.c \
../src/stm32f10x_exti.c \
../src/stm32f10x_flash.c \
../src/stm32f10x_fsmc.c \
../src/stm32f10x_gpio.c \
../src/stm32f10x_i2c.c \
../src/stm32f10x_it.c \
../src/stm32f10x_iwdg.c \
../src/stm32f10x_pwr.c \
../src/stm32f10x_rcc.c \
../src/stm32f10x_rtc.c \
../src/stm32f10x_sdio.c \
../src/stm32f10x_spi.c \
../src/stm32f10x_tim.c \
../src/stm32f10x_usart.c \
../src/stm32f10x_wwdg.c \
../src/system_stm32f10x.c 

OBJS += \
./src/core_cm3.o \
./src/main.o \
./src/misc.o \
./src/stm32f10x_adc.o \
./src/stm32f10x_bkp.o \
./src/stm32f10x_can.o \
./src/stm32f10x_cec.o \
./src/stm32f10x_crc.o \
./src/stm32f10x_dac.o \
./src/stm32f10x_dbgmcu.o \
./src/stm32f10x_dma.o \
./src/stm32f10x_exti.o \
./src/stm32f10x_flash.o \
./src/stm32f10x_fsmc.o \
./src/stm32f10x_gpio.o \
./src/stm32f10x_i2c.o \
./src/stm32f10x_it.o \
./src/stm32f10x_iwdg.o \
./src/stm32f10x_pwr.o \
./src/stm32f10x_rcc.o \
./src/stm32f10x_rtc.o \
./src/stm32f10x_sdio.o \
./src/stm32f10x_spi.o \
./src/stm32f10x_tim.o \
./src/stm32f10x_usart.o \
./src/stm32f10x_wwdg.o \
./src/system_stm32f10x.o 

C_DEPS += \
./src/core_cm3.d \
./src/main.d \
./src/misc.d \
./src/stm32f10x_adc.d \
./src/stm32f10x_bkp.d \
./src/stm32f10x_can.d \
./src/stm32f10x_cec.d \
./src/stm32f10x_crc.d \
./src/stm32f10x_dac.d \
./src/stm32f10x_dbgmcu.d \
./src/stm32f10x_dma.d \
./src/stm32f10x_exti.d \
./src/stm32f10x_flash.d \
./src/stm32f10x_fsmc.d \
./src/stm32f10x_gpio.d \
./src/stm32f10x_i2c.d \
./src/stm32f10x_it.d \
./src/stm32f10x_iwdg.d \
./src/stm32f10x_pwr.d \
./src/stm32f10x_rcc.d \
./src/stm32f10x_rtc.d \
./src/stm32f10x_sdio.d \
./src/stm32f10x_spi.d \
./src/stm32f10x_tim.d \
./src/stm32f10x_usart.d \
./src/stm32f10x_wwdg.d \
./src/system_stm32f10x.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -DDEBUG -DUSE_STDPERIPH_DRIVER -I"D:/stm_workspace/spi_gpio_int/inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


