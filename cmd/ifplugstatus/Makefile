ROOT=../..
include $(ROOT)/mk/hdr.mk
include $(ROOT)/mk/ifplug.mk

LDLIBS = -ldaemon
TARG =	ifplugstatus
OBJ = ifplugstatus

LIB = $(ROOT)/lib/libinterface.a

INCLUDES += -D_GNU_SOURCE

include $(ROOT)/mk/one.mk

