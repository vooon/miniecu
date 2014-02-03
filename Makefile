# -*- Makefile -*-
#

TARGETS = miniecu_v1 miniecu_sim

all: $(TARGETS)

miniecu_v1:

miniecu_sim:


%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
