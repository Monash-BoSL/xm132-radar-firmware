TOOLS_PREFIX := $(GNU_INSTALL_ROOT)/arm-none-eabi-
TOOLS_AR         := $(TOOLS_PREFIX)ar
TOOLS_AS         := $(TOOLS_PREFIX)as
TOOLS_CC         := $(TOOLS_PREFIX)gcc
TOOLS_OBJDUMP    := $(TOOLS_PREFIX)objdump
TOOLS_OBJCOPY    := $(TOOLS_PREFIX)objcopy
TOOLS_SIZE       := $(TOOLS_PREFIX)size
TOOLS_LD         := $(TOOLS_PREFIX)gcc

TARGET_ARCHITECTURE := cm0

CFLAGS += -mcpu=cortex-m0plus -mthumb -march=armv6-m -DTARGET_ARCH_cm0 -std=c99 -pedantic -Wall -Werror -Wextra -Wdouble-promotion -Wstrict-prototypes -Wcast-qual -Wmissing-prototypes -Winit-self -Wpointer-arith -MMD -MP -Os -g -fno-math-errno -ffunction-sections -fdata-sections -flto -ffat-lto-objects -DACC_SQRTF=__ieee754_sqrtf

# Override optimization level
ifneq ($(ACC_CFG_OPTIM_LEVEL),)
	CFLAGS  += $(ACC_CFG_OPTIM_LEVEL)
endif

ASFLAGS += -mcpu=cortex-m0plus -mthumb -march=armv6-m

LDFLAGS += -mcpu=cortex-m0plus -mthumb -march=armv6-m -L$(OUT_DIR) -Werror -Wl,--gc-sections -flto

LDLIBS += -lm

ARFLAGS := cr --plugin $(shell find $(GNU_INSTALL_ROOT)/../ -name liblto_plugin.so)
