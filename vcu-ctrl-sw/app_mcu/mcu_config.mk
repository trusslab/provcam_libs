# Utils
RM := rm -rf
CC := mb-gcc
OBJDUMP := mb-objdump
OBJCOPY := mb-objcopy
SIZE := mb-size
PLUGIN?=plugin

ROOT_BIN?=bin
BIN=$(ROOT_BIN)/mcu

# Mcu configuration
CFLAGS+=\
  -mxl-barrel-shift\
  -mxl-pattern-compare\
  -mno-xl-soft-div\
  -mno-xl-soft-mul\
  -mxl-multiply-high\
  -ffunction-sections\
  -fdata-sections\
  -mlittle-endian\

# Debug flags
#CFLAGS+=-g3
#LDFLAGS+=-g3
#LDFLAGS+= -Wl,-verbose

# Size optimization. Do not keep useless code and drop debug code
CFLAGS+=-DNDEBUG
CFLAGS+=-g0
CFLAGS+=-Wall -Wextra
# microblaze compiler has trouble with field initializers
CFLAGS+=-Wno-missing-field-initializers -Wno-missing-braces
#CFLAGS+=-Werror
CFLAGS+=-Os

LDFLAGS=-nostdlib -Wl,--no-relax -Wl,--gc-sections -lgcc
LDFLAGS+=-g0

