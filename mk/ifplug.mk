VERSION = 0.3

COPYRIGHT = ©2011 Ilya Petrov

$(ROOT)/include/ixp.h: $(ROOT)/config.mk
CFLAGS += '-DVERSION=\"$(VERSION)\"' '-DCOPYRIGHT=\"$(COPYRIGHT)\"'

