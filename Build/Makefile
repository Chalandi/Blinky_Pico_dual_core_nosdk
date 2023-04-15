# ******************************************************************************************
#   Filename    : Makefile
#
#   Author      : Chalandi Amine
#
#   Owner       : Chalandi Amine
#
#   Date        : 22.11.2022
#
#   Description : Build system
#
# ******************************************************************************************

############################################################################################
# Defines
############################################################################################

PRJ_NAME   = Blinky_Pico_dual_core_nosdk
OUTPUT_DIR = ../Output
OBJ_DIR    = $(OUTPUT_DIR)/Obj
LD_SCRIPT  = $(SRC_DIR)/Memory_Map.ld
PIO_OUT_DIR= $(SRC_DIR)/pio
SRC_DIR    = ../Code
ELF2UF2    = ../Tools/elf2uf2
PIOASM     = ../Tools/pioasm
ERR_MSG_FORMATER_SCRIPT = ../Tools/Scripts/CompilerErrorFormater.py
############################################################################################
# Toolchain
############################################################################################

AS      = arm-none-eabi-as
CC      = arm-none-eabi-gcc
CPP     = arm-none-eabi-g++
LD      = arm-none-eabi-gcc
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
READELF = arm-none-eabi-readelf

PYTHON = python

############################################################################################
# Optimization Compiler flags
############################################################################################

OPT_MODIFIED_O2 = -O2                               \
                  -fno-reorder-blocks-and-partition \
                  -fno-reorder-functions

NO_OPT = -O0

OPT = $(NO_OPT)

############################################################################################
# GCC Compiler verbose flags
############################################################################################

VERBOSE_GCC = -frecord-gcc-switches -fverbose-asm

############################################################################################
# Target's Compiler flags
############################################################################################

ARCH = -mcpu=cortex-m0plus

############################################################################################
# C Compiler flags
############################################################################################

COPS  = -mlittle-endian                               \
        -mlong-calls                                  \
        $(OPT)                                        \
        $(ARCH)                                       \
        -mthumb                                       \
        -mabi=aapcs                                   \
        -ffast-math                                   \
        -MD                                           \
        -Wa,-adhln=$(OBJ_DIR)/$(basename $(@F)).lst   \
        -g3                                           \
        -Wconversion                                  \
        -Wsign-conversion                             \
        -Wunused-parameter                            \
        -Wuninitialized                               \
        -Wmissing-declarations                        \
        -Wshadow                                      \
        -Wunreachable-code                            \
        -Wmissing-include-dirs                        \
        -x c                                          \
        -std=c11                                      \
        -Wall                                         \
        -Wextra                                       \
        -fomit-frame-pointer                          \
        -gdwarf-2                                     \
        -fno-exceptions

############################################################################################
# C++ Compiler flags
############################################################################################

CPPOPS  = -mlittle-endian                               \
          -mlong-calls                                  \
          $(OPT)                                        \
          $(ARCH)                                       \
          -mthumb                                       \
          -mabi=aapcs                                   \
          -ffast-math                                   \
          -Wa,-adhln=$(OBJ_DIR)/$(basename $(@F)).lst   \
          -g3                                           \
          -Wconversion                                  \
          -Wsign-conversion                             \
          -Wunused-parameter                            \
          -Wuninitialized                               \
          -Wmissing-declarations                        \
          -Wshadow                                      \
          -Wunreachable-code                            \
          -Wmissing-include-dirs                        \
          -Wall                                         \
          -Wextra                                       \
          -fomit-frame-pointer                          \
          -gdwarf-2                                     \
          -fno-exceptions                               \
          -x c++                                        \
          -fno-rtti                                     \
          -fno-use-cxa-atexit                           \
          -fno-nonansi-builtins                         \
          -fno-threadsafe-statics                       \
          -fno-enforce-eh-specs                         \
          -ftemplate-depth=128                          \
          -Wzero-as-null-pointer-constant

############################################################################################
# Assembler flags
############################################################################################

ASOPS =  $(ARCH)          \
         -mlittle-endian  \
         -mthumb          \
         -alh 

############################################################################################
# Linker flags
############################################################################################

ifeq ($(LD), arm-none-eabi-ld)
  LOPS = -nostartfiles                          \
         -nostdlib                              \
         $(ARCH)                                \
         -mthumb                                \
         -ffast-math                            \
         -e Startup_Init                        \
         --print-memory-usage                   \
         --print-map                            \
         -dT $(LD_SCRIPT)                       \
         -Map=$(OUTPUT_DIR)/$(PRJ_NAME).map     \
         --specs=nano.specs                     \
         --specs=nosys.specs
else
  LOPS = -nostartfiles                          \
         -e Startup_Init                        \
         $(ARCH)                                \
         -mthumb                                \
         -mabi=aapcs                            \
         -ffast-math                            \
         -Wl,--print-memory-usage               \
         -Wl,--print-map                        \
         -Wl,-dT $(LD_SCRIPT)                   \
         -Wl,-Map=$(OUTPUT_DIR)/$(PRJ_NAME).map \
         --specs=nano.specs                     \
         --specs=nosys.specs
endif

############################################################################################
# Source Files
############################################################################################

SRC_FILES := $(SRC_DIR)/Appli/main.c                      \
             $(SRC_DIR)/Mcal/Clock/Clock.c                \
             $(SRC_DIR)/Mcal/Cpu/Cpu.c                    \
             $(SRC_DIR)/Mcal/SysTickTimer/SysTickTimer.c  \
             $(SRC_DIR)/Mcal/USB/USB.c                    \
             $(SRC_DIR)/JTAG/jtag.c                       \
             $(SRC_DIR)/Startup/IntVect.c                 \
             $(SRC_DIR)/Startup/SecondaryBoot.c           \
             $(SRC_DIR)/Startup/Startup.c                 \
             $(SRC_DIR)/Startup/util.s                    \
             $(SRC_DIR)/SWD/swd.c


PIO_SRC_FILES := $(SRC_DIR)/pio/jtag_pio.pio  \
                 $(SRC_DIR)/pio/swd_pio.pio

############################################################################################
# Include Paths
############################################################################################
INC_FILES := $(SRC_DIR)                    \
             $(SRC_DIR)/Appli              \
             $(SRC_DIR)/pio                \
             $(SRC_DIR)/Mcal               \
             $(SRC_DIR)/Mcal/Clock         \
             $(SRC_DIR)/Mcal/Cmsis         \
             $(SRC_DIR)/Mcal/Cpu           \
             $(SRC_DIR)/Mcal/Gpio          \
             $(SRC_DIR)/Mcal/SysTickTimer  \
             $(SRC_DIR)/Mcal/USB           \
             $(SRC_DIR)/JTAG               \
             $(SRC_DIR)/Startup            \
             $(SRC_DIR)/Std                \
             $(SRC_DIR)/SWD

############################################################################################
# Rules
############################################################################################

VPATH := $(subst \,/,$(sort $(dir $(SRC_FILES)) $(OBJ_DIR) $(PIO_OUT_DIR) $(sort $(dir $(PIO_SRC_FILES)))))

FILES_O := $(addprefix $(OBJ_DIR)/, $(notdir $(addsuffix .o, $(basename $(SRC_FILES)))))

PIO_OUTPUT_FILES = $(addprefix $(PIO_OUT_DIR)/, $(notdir $(addsuffix .h, $(basename $(PIO_SRC_FILES)))))

ifeq ($(MAKECMDGOALS),build)
-include $(subst .o,.d,$(FILES_O))
endif

build : PIO_SRC_GEN $(OUTPUT_DIR)/$(PRJ_NAME).elf

all : PIO_SRC_GEN $(OUTPUT_DIR)/$(PRJ_NAME).elf

.PHONY : PIO_SRC_GEN
PIO_SRC_GEN: $(PIO_OUTPUT_FILES)

$(PIO_OUT_DIR)/%.h : %.pio
	@-echo +++ compile pio assembly: $(subst \,/, $<) to $(subst \,/, $@)
	@$(PIOASM)  $<  $@

.PHONY : clean
clean :
	@-rm -rf $(OUTPUT_DIR) *.o    2>/dev/null || true
	@-rm -rf $(OUTPUT_DIR) *.hex  2>/dev/null || true
	@-rm -rf $(OUTPUT_DIR) *.elf  2>/dev/null || true
	@-rm -rf $(OUTPUT_DIR) *.list 2>/dev/null || true
	@-rm -rf $(OUTPUT_DIR) *.map  2>/dev/null || true
	@-rm -rf $(OUTPUT_DIR) *.txt  2>/dev/null || true
	@-mkdir -p $(subst \,/,$(OBJ_DIR))

$(OBJ_DIR)/%.o : %.c
	@-echo +++ compile: $(subst \,/,$<) to $(subst \,/,$@)
	@-$(CC) $(COPS) $(addprefix -I, $(INC_FILES)) -c $< -o $(OBJ_DIR)/$(basename $(@F)).o 2> $(OBJ_DIR)/$(basename $(@F)).err
	@-$(PYTHON) $(ERR_MSG_FORMATER_SCRIPT) $(OBJ_DIR)/$(basename $(@F)).err -COLOR

$(OBJ_DIR)/%.o : %.s
	@-echo +++ compile: $(subst \,/,$<) to $(subst \,/,$@)
	@$(AS) $(ASOPS) $< -o $(OBJ_DIR)/$(basename $(@F)).o 2> $(OBJ_DIR)/$(basename $(@F)).err >$(OBJ_DIR)/$(basename $(@F)).lst
	@-$(PYTHON) $(ERR_MSG_FORMATER_SCRIPT) $(OBJ_DIR)/$(basename $(@F)).err -COLOR

$(OBJ_DIR)/%.o : %.cpp
	@-echo +++ compile: $(subst \,/,$<) to $(subst \,/,$@)
	@$(CPP) $(CPPOPS) $(addprefix -I, $(INC_FILES)) -c $< -o $(OBJ_DIR)/$(basename $(@F)).o 2> $(OBJ_DIR)/$(basename $(@F)).err
	@-$(PYTHON) $(ERR_MSG_FORMATER_SCRIPT) $(OBJ_DIR)/$(basename $(@F)).err -COLOR

$(OUTPUT_DIR)/$(PRJ_NAME).elf : $(FILES_O) $(LD_SCRIPT)
	@-echo +++ link : $(subst \,/,$@)
	@$(LD) $(LOPS) $(FILES_O) -o $(OUTPUT_DIR)/$(PRJ_NAME).elf
	@-echo +++ generate : $(OUTPUT_DIR)/$(PRJ_NAME).readelf
	@$(READELF) --wide -a $(OUTPUT_DIR)/$(PRJ_NAME).elf > $(OUTPUT_DIR)/$(PRJ_NAME).readelf
	@$(ELF2UF2) $(OUTPUT_DIR)/$(PRJ_NAME).elf $(OUTPUT_DIR)/$(PRJ_NAME).uf2

