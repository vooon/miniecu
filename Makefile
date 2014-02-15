# -*- Makefile -*-
#

TARGETS = miniecu_v1 vl_discovery

all: $(TARGETS)

miniecu_v1:

vl_discovery:

%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
