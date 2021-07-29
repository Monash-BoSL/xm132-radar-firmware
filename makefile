# Copyright (c) Acconeer AB, 2020
# All rights reserved

.DEFAULT_GOAL := all

TARGETS := \
	main
	# radar_velocity_stm_firmware
    # example_assembly_test \
    # example_detector_distance \
    # example_detector_distance_recorded \
    # example_detector_presence \
    # example_get_next_by_reference \
    # example_low_power_service_sparse \
    # example_multiple_service_usage \
    # example_service_envelope \
    # example_service_iq \
    # example_service_power_bins \
    # example_service_sparse \
    # ref_app_rf_certification_test \
    # ref_app_parking \
    # ref_app_smart_presence \
    # ref_app_tank_level \

# Variable to modify output messages
ifneq ($(V),)
SUPPRESS :=
else
SUPPRESS := @
endif

OUT_DIR     := out
OUT_OBJ_DIR := $(OUT_DIR)/obj
OUT_LIB_DIR := $(OUT_DIR)/lib

INC_FOLDERS := -I. -IInc 

vpath %.c Src
vpath %.s Src

LDLIBS := \
    -lacc_detector_distance \
    -lacc_detector_presence \
    -lacc_rf_certification_test \
    -lacconeer

# Target specific defines
example_low_power_service_sparse: CFLAGS+=-DUSE_SERVICE_SPARSE -DRANGE_LENGTH=1.0f -DSERVICE_PROFILE=3

# Important to have startup_stm32g071xx.s first when using -flto
# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83967
STM32_CUBE_GENERATED_FILES := \
    startup_stm32g071xx.s \
    main.c \
    stm32g0xx_hal_msp.c \
    stm32g0xx_it.c \
    system_stm32g0xx.c \
    sysmem.c 

STM32_CUBE_INTEGRATION_FILES := \
    acc_hal_integration_stm32cube_xm132.c \
    syscalls.c 

RSS_INTEGRATION_FILES := \
    acc_integration_stm32.c

SOURCES_MAIN := \
    main.c \
	reg_interface.c

# SOURCES_RADAR_VELOCITY_STM_FIRMWARE := \
    # radar_velocity_stm_firmware.c

# SOURCES_EXAMPLE_ASSEMBLY_TEST := \
    # example_assembly_test.c

# SOURCES_EXAMPLE_DETECTOR_DISTANCE := \
    # example_detector_distance.c

# SOURCES_EXAMPLE_DETECTOR_DISTANCE_RECORDED := \
    # example_detector_distance_recorded.c

# SOURCES_EXAMPLE_DETECTOR_PRESENCE := \
    # example_detector_presence.c

# SOURCES_EXAMPLE_GET_NEXT_BY_REFERENCE := \
    # example_get_next_by_reference.c

# SOURCES_EXAMPLE_LOW_POWER_SERVICE_SPARSE := \
    # example_low_power.c

# SOURCES_EXAMPLE_MULTIPLE_SERVICE_USAGE := \
    # example_multiple_service_usage.c

# SOURCES_EXAMPLE_SERVICE_ENVELOPE := \
    # example_service_envelope.c

# SOURCES_EXAMPLE_SERVICE_IQ := \
    # example_service_iq.c

# SOURCES_EXAMPLE_SERVICE_POWER_BINS := \
    # example_service_power_bins.c

# SOURCES_EXAMPLE_SERVICE_SPARSE := \
    # example_service_sparse.c

# SOURCES_REF_APP_RF_CERTIFICATION_TEST := \
    # ref_app_rf_certification_test.c

# SOURCES_REF_APP_PARKING := \
    # ref_app_parking.c

# SOURCES_REF_APP_SMART_PRESENCE := \
    # ref_app_smart_presence.c

# SOURCES_REF_APP_TANK_LEVEL := \
    # ref_app_tank_level.c

_SOURCES := $(STM32_CUBE_INTEGRATION_FILES) $(STM32_CUBE_GENERATED_FILES) $(RSS_INTEGRATION_FILES) 

include $(sort $(wildcard rule/makefile_target_*.inc))
include $(sort $(wildcard rule/makefile_define_*.inc))
include $(sort $(wildcard rule/makefile_build_*.inc))

CFLAGS-$(OUT_OBJ_DIR)/syscalls.o += -fno-lto
CFLAGS-$(OUT_OBJ_DIR)/sysmem.o = -std=gnu11 -Wno-strict-prototypes -Wno-pedantic -Wno-missing-prototypes -fno-lto
CFLAGS-$(OUT_OBJ_DIR)/printf.o = -DPRINTF_DISABLE_SUPPORT_FLOAT -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL -fno-lto

.PHONY: all
all: $(TARGETS)

$(BUILD_LIBS) : | $(OUT_LIB_DIR)

$(OUT_OBJ_DIR)/%.o: %.c | $(OUT_OBJ_DIR)
	@echo "Compiling $(notdir $<)"
	$(SUPPRESS)$(TOOLS_CC) $^ -c $(CFLAGS) $(CFLAGS-$@) $(INC_FOLDERS) -o $@

$(OUT_OBJ_DIR)/%.o: %.s | $(OUT_OBJ_DIR)
	@echo "Assembling $(notdir $<)"
	$(SUPPRESS)$(TOOLS_AS) $^ $(ASFLAGS) -o $@

$(OUT_LIB_DIR):
	$(SUPPRESS)mkdir -p $@

$(OUT_OBJ_DIR):
	$(SUPPRESS)mkdir -p $@

$(OUT_DIR):
	$(SUPPRESS)mkdir -p $@

# Function to convert string to uppercase
# Parameter 1: string to convert to uppercase
uc = $(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,$(subst f,F,$(subst g,G,$(subst \
	h,H,$(subst i,I,$(subst j,J,$(subst k,K,$(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,$(subst \
	p,P,$(subst q,Q,$(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,$(subst \
	x,X,$(subst y,Y,$(subst z,Z,$1))))))))))))))))))))))))))

# Create all target recipes
$(foreach target, $(TARGETS), $(eval SOURCES := $(_SOURCES) $($(addsuffix $(call uc,$(target)),SOURCES_))) $(call define_target,$(target)))

# Create jlink flash targets
$(foreach target, $(TARGETS), $(eval $(call define_jlink_flash_target, $(target))))

.PHONY : clean
clean:
	$(SUPPRESS)rm -rf out/*
