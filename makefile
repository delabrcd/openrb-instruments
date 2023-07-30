#
#             LUFA Library
#     Copyright (C) Dean Camera, 2013.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = LEONARDO
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = adapter_common

INCLUDE_DIRS := -I./ \
				-IConfig \
				-Iarduino \
				-Imidi \
				-Iusbhost 

C_SRCS 		:= 	lufa_callbacks.c \
				adapter_descriptors.c \
				$(wildcard arduino/*.c)	

CPP_SRCS 	:= 	adapter_common.cpp \
				debug_helpers.cpp \
				packet_circ_buf.cpp \
				packet_helpers.cpp \
				pdp_drum_identifiers.cpp \
				$(wildcard midi/*.cpp)	\
				$(wildcard usbhost/*.cpp) \
				$(wildcard arduino/*.cpp)	

			
SRC          = $(C_SRCS) $(CPP_SRCS) $(LUFA_SRC_USB) $(LUFA_SRC_SERIAL) 
LUFA_PATH    = LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER $(INCLUDE_DIRS) -D__AVR_ATmega32U4__ -DARDUINO_AVR_LEONARDO
CPP_FLAGS 	 = -std=c++17 -DUSE_LUFA_CONFIG_HEADER $(INCLUDE_DIRS) -DARDUINO=100 -D__AVR_ATmega32U4__ -DARDUINO_AVR_LEONARDO
LD_FLAGS     =

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
