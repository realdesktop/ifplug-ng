ROOT=../..
include $(ROOT)/mk/hdr.mk
include $(ROOT)/mk/ifplug.mk

LDLIBS = -ldaemon
TARG =	ifplugd
OBJ =  \
       ifplugd \
       nlapi \
       ifmonitor \

LIB = $(ROOT)/lib/libinterface.a
INCLUDES += -D_GNU_SOURCE

include $(ROOT)/mk/one.mk

