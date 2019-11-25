#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= vchiqsoundbasedevice.o

libvchiqsound.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
