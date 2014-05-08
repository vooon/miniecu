# -*- Makefile -*-
#

MINIECU ?= ..
BUILDDIR ?= $(MINIECU)/build

NANOPBDIR = $(MINIECU)/ext/nanopb
PROTODIR = $(BUILDDIR)/pb

PROTOC = protoc
PROTOC_OPTS = --plugin=protoc-gen-nanopb=$(NANOPBDIR)/generator/protoc-gen-nanopb

all: $(PROTODIR)/miniecu.pb.c $(PROTODIR)/flash.pb.c

$(PROTODIR):
	mkdir -p $(PROTODIR)

$(PROTODIR)/%.pb.c: %.proto $(wildcard %.options) $(PROTODIR)
ifeq ($(USE_VERBOSE_COMPILE),yes)
	@echo
	$(PROTOC) $(PROTOC_OPTS) --nanopb_out="$(PROTODIR)" $<
else
	@echo Protoc $(<F)
	@$(PROTOC) $(PROTOC_OPTS) --nanopb_out="$(PROTODIR)" $<
endif
