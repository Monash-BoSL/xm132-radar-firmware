LDFLAGS += --specs=nosys.specs --specs=nano.specs -Wl,--gc-sections
LDFLAGS += -TSTM32G071CBUx_FLASH.ld
LDFLAGS += -Llib -L"$(OUT_LIB_DIR)"

CFLAGS += -DSTM32G071xx -DUSE_HAL_DRIVER

STM32CUBE_FW_ROOT := $(STM32CUBE_FW_G0_ROOT)

vpath %.s $(STM32CUBE_FW_G0_ROOT)/Drivers/CMSIS/Device/ST/STM32G0xx/Source/Templates/gcc
