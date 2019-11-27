#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o env.o coroutine.o common.o stub.o util.o sysinit.o startup.o

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
	  #$(CIRCLEHOME)/lib/sched/libsched.a \
	  #$(CIRCLEHOME)/lib/libcircle.a

EXTRACLEAN = sound.h

include $(CIRCLEHOME)/Rules.mk

main.o: sound.h

sound.h: sound.h.gz
	@echo "  UNZIP $@"
	@gunzip -c sound.h.gz > sound.h
