#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o env.o coroutine.o common.o stub.o util.o sysinit.o

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a

include $(CIRCLEHOME)/Rules.mk
