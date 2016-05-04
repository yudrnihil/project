################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/stm32f4xx_it.c 

CPP_SRCS += \
../src/CKey.cpp \
../src/ILI93xx.cpp \
../src/SDCard.cpp \
../src/UI.cpp \
../src/WAVPlayer.cpp \
../src/dac.cpp \
../src/delay.cpp \
../src/key.cpp \
../src/main.cpp \
../src/sound.cpp \
../src/timer_interrupt.cpp 

OBJS += \
./src/CKey.o \
./src/ILI93xx.o \
./src/SDCard.o \
./src/UI.o \
./src/WAVPlayer.o \
./src/dac.o \
./src/delay.o \
./src/key.o \
./src/main.o \
./src/sound.o \
./src/stm32f4xx_it.o \
./src/timer_interrupt.o 

C_DEPS += \
./src/stm32f4xx_it.d 

CPP_DEPS += \
./src/CKey.d \
./src/ILI93xx.d \
./src/SDCard.d \
./src/UI.d \
./src/WAVPlayer.d \
./src/dac.d \
./src/delay.d \
./src/key.d \
./src/main.d \
./src/sound.d \
./src/timer_interrupt.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_SEMIHOSTING_DEBUG -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4" -std=gnu++11 -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_SEMIHOSTING_DEBUG -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


