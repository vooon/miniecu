# -*- Makefile -*-
#

MINIECU ?= ..
BUILDDIR ?= $(MINIECU)/build

NANOPBDIR = $(MINIECU)/ext/nanopb
NANOPBP2PY = $(NANOPBDIR)/generator/proto/nanopb_pb2.py
NANOPBPPLUGIN = $(NANOPBDIR)/generator/protoc-gen-nanopb
PROTODIR = $(BUILDDIR)/pb

PROTOC = protoc
PROTOC_OPTS = --plugin=protoc-gen-nanopb=$(NANOPBPPLUGIN)

all: $(PROTODIR)/miniecu.pb.c $(PROTODIR)/flash.pb.c

$(PROTODIR):
	mkdir -p $(PROTODIR)

$(NANOPBP2PY):
	make -C $(NANOPBDIR)/generator/proto

$(PROTODIR)/%.pb.c: %.proto $(wildcard %.options) $(PROTODIR) $(NANOPBP2PY)
ifeq ($(USE_VERBOSE_COMPILE),yes)
	@echo
	$(PROTOC) $(PROTOC_OPTS) --nanopb_out="$(PROTODIR)" $<
else
	@echo Protoc $(<F)
	@$(PROTOC) $(PROTOC_OPTS) --nanopb_out="$(PROTODIR)" $<
endif

python_msgs:
	$(PROTOC) *.proto --python_out="$(MINIECU)/tools/miniecu"
