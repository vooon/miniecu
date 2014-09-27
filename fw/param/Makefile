# -*- Makefile -*-
#

MINIECU ?= ../..
BUILDDIR ?= $(MINIECU)/build

PGENPY = $(MINIECU)/tools/pgen/pgen.py
PARAMDIR = $(BUILDDIR)/pgen
PARAMDEF = $(MINIECU)/fw/parameters.yaml

PYTHON = python

all: $(PARAMDIR)/param_table.c

$(PARAMDIR):
	mkdir -p $(PARAMDIR)

$(PARAMDIR)/param_table.c: $(PARAMDEF) $(PARAMDIR)
ifeq ($(USE_VERBOSE_COMPILE),yes)
	@echo
	$(PYTHON) $(PGENPY) $< -o $(PARAMDIR)
else
	@echo PGEN $(<F)
	@$(PYTHON) $(PGENPY) $< -o $(PARAMDIR)
endif
