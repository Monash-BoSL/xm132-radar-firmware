CFLAGS += \
    -I$(STM32CUBE_FW_G0_ROOT)/Drivers/CMSIS/Include \
    -I$(STM32CUBE_FW_G0_ROOT)/Drivers/STM32G0xx_HAL_Driver/Inc \
    -I$(STM32CUBE_FW_G0_ROOT)/Drivers/STM32G0xx_HAL_Driver/Inc/Legacy \
    -I$(STM32CUBE_FW_G0_ROOT)/Drivers/CMSIS/Device/ST/STM32G0xx/Include

vpath %.c $(STM32CUBE_FW_G0_ROOT)/Drivers/STM32G0xx_HAL_Driver/Src

STM32_CUBE_DRIVER_SOURCES := \
    stm32g0xx_hal_gpio.c \
    stm32g0xx_hal_adc.c \
    stm32g0xx_hal_adc_ex.c \
    stm32g0xx_ll_adc.c \
    stm32g0xx_hal_rcc.c \
    stm32g0xx_hal_rcc_ex.c \
    stm32g0xx_ll_rcc.c \
    stm32g0xx_hal_flash.c \
    stm32g0xx_hal_flash_ex.c \
    stm32g0xx_hal_dma.c \
    stm32g0xx_hal_dma_ex.c \
    stm32g0xx_hal_pwr.c \
    stm32g0xx_hal_pwr_ex.c \
    stm32g0xx_hal_cortex.c \
    stm32g0xx_hal.c \
    stm32g0xx_hal_exti.c \
    stm32g0xx_hal_i2c.c \
    stm32g0xx_hal_i2c_ex.c \
    stm32g0xx_hal_lptim.c \
    stm32g0xx_hal_rtc.c \
    stm32g0xx_hal_rtc_ex.c \
    stm32g0xx_hal_spi.c \
    stm32g0xx_hal_spi_ex.c \
    stm32g0xx_hal_tim.c \
    stm32g0xx_hal_tim_ex.c \
    stm32g0xx_hal_uart.c \
    stm32g0xx_hal_uart_ex.c

STM32_CUBE_DRIVER_OBJECTS = $(addprefix $(OUT_OBJ_DIR)/,$(patsubst %.c,%.o,$(STM32_CUBE_DRIVER_SOURCES)))

ifneq (,$(findstring gcc,$(TOOLS_CC)))
$(foreach object, $(STM32_CUBE_DRIVER_OBJECTS), $(eval CFLAGS-$(object) += -Wno-unused-parameter -Wno-pedantic))
endif

$(OUT_LIB_DIR)/libstm32cube_fw_g0.a : $(STM32_CUBE_DRIVER_OBJECTS)
	@echo "    Creating archive $(notdir $@)"
	$(SUPPRESS)rm -f $@
	$(SUPPRESS)$(TOOLS_AR) $(ARFLAGS) $@ $^

BUILD_LIBS += $(OUT_LIB_DIR)/libstm32cube_fw_g0.a

LDLIBS += -lstm32cube_fw_g0
