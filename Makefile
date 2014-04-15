# -*- Makefile -*-
#

TARGETS = miniecu_v1
SERIAL = /dev/ttyUSB0

all: $(TARGETS)

miniecu_v1:

miniecu_v1_flash: miniecu_v1
	stm32flash -w ./build/miniecu_v1/miniecu_v1.bin $(SERIAL)

%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
