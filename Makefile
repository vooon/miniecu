# -*- Makefile -*-
#

TARGETS = miniecu_v2
SERIAL = /dev/ttyUSB0

all: $(TARGETS)

miniecu_v2:

miniecu_v2_flash: miniecu_v2
	dfu-util -v --alt=0 -s 0x08000000 -D ./build/miniecu_v2/miniecu_v2.bin

%: ./boards/%
	make -C ./pb all python_msgs
	make -C ./fw/param
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
