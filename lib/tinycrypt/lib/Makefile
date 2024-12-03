################################################################################
#
#      Copyright (C) 2017 by Intel Corporation, All Rights Reserved.
#
# 			Cryptographic Primitives Makefile.
#
################################################################################

include ../config.mk

# Edit the OBJS content to add/remove primitives needed from TinyCrypt library:
OBJS:=hmac.o \
	hmac_prng.o \
	sha256.o \
	utils.o

DEPS:=$(OBJS:.o=.d)

all: libtinycrypt.a

libtinycrypt.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

.PHONY: clean

clean:
	-$(RM) *.exe $(OBJS) $(DEPS) *~ libtinycrypt.a

-include $(DEPS)
