ROOT=.
include $(ROOT)/mk/hdr.mk
include $(ROOT)/mk/ifplug.mk

DIRS =	lib	\
	cmd	\
	include	\
	man


.PHONY: doc
include $(ROOT)/mk/dir.mk

