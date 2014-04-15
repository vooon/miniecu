# -*- Makefile -*-
#

TARGETS = miniecu_v1 miniecu_v2
SERIAL = /dev/ttyUSB0

all: $(TARGETS)

miniecu_v1:

miniecu_v1_flash: miniecu_v1
	stm32flash -w ./build/miniecu_v1/miniecu_v1.bin $(SERIAL)

miniecu_v2:

miniecu_v2_flash: miniecu_v2
	dfu-util -l /* TODO */

%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
