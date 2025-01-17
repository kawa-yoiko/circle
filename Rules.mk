#
# Rules.mk
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

AARCH	 ?= 32
RASPPI	 ?= 1
PREFIX	 ?= arm-none-eabi-
PREFIX64 ?= aarch64-elf-

# set this to "softfp" if you want to link specific libraries
FLOAT_ABI ?= hard

CC	= $(PREFIX)gcc
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

ifeq ($(strip $(AARCH)),32)
ifeq ($(strip $(RASPPI)),1)
ARCH	?= -DAARCH=32 -mcpu=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -DAARCH=32 -mcpu=cortex-a7 -marm -mfpu=neon-vfpv4 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7
else ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=32 -mcpu=cortex-a53 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel8-32
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=32 -mcpu=cortex-a72 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7l
else
$(error RASPPI must be set to 1, 2, 3 or 4)
endif
LOADADDR = 0x8000
else ifeq ($(strip $(AARCH)),64)
ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=64 -mcpu=cortex-a53 -mlittle-endian -mcmodel=small
TARGET	?= kernel8
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=64 -mcpu=cortex-a72 -mlittle-endian -mcmodel=small
TARGET	?= kernel8-rpi4
else
$(error RASPPI must be set to 3 or 4)
endif
PREFIX	= $(PREFIX64)
LOADADDR = 0x80000
else
$(error AARCH must be set to 32 or 64)
endif

MAKE_VERSION_MAJOR := $(firstword $(subst ., ,$(MAKE_VERSION)))
ifneq ($(filter 0 1 2 3,$(MAKE_VERSION_MAJOR)),)
$(error Requires GNU make 4.0 or newer)
endif

LIBM	  != $(CPP) $(ARCH) -print-file-name=libm.a
ifneq ($(strip $(LIBM)),libm.a)
EXTRALIBS += $(LIBM)
endif

OPTIMIZE ?= -O2

INCLUDE	+= -I $(CIRCLEHOME)/addon -I $(CIRCLEHOME)/addon/vc4
DEFINE	+= -D__circle__ -DRASPPI=$(RASPPI) \
	   -D__VCCOREVER__=0x04000000 -U__unix__ -U__linux__ #-DNDEBUG

AFLAGS	+= $(ARCH) $(DEFINE) $(INCLUDE) $(OPTIMIZE)
CFLAGS	+= $(ARCH) -Wall -fsigned-char -ffreestanding $(DEFINE) $(INCLUDE) $(OPTIMIZE) -g
LDFLAGS	+= --section-start=.init=$(LOADADDR)

%.o: %.S
	@echo "  AS    $@"
	@$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -std=gnu99 -c -o $@ $<

$(TARGET).img: $(OBJS) $(LIBS) $(CIRCLEHOME)/circle.ld
	@echo "  LD    $(TARGET).elf"
	@$(LD) -o $(TARGET).elf $(LDFLAGS) \
		-T $(CIRCLEHOME)/circle.ld $(CRTBEGIN) $(OBJS) \
		--start-group $(LIBS) $(EXTRALIBS) --end-group $(CRTEND)
	@echo "  COPY  $(TARGET).img"
	@$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	@echo -n "  WC    $(TARGET).img => "
	@wc -c < $(TARGET).img

clean:
	rm -f *.o *.a *.elf *.lst *.img *.hex *.cir *.map *~ $(EXTRACLEAN)
