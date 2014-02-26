# -*- Makefile -*-
#

TARGETS = miniecu_v1 vl_discovery
SERIAL = /dev/ttyUSB0

all: $(TARGETS)

miniecu_v1:

vl_discovery:

vl_discovery_gdb: vl_discovery
	st-util -1 > ./build/st-util.log 2>&1 &
	sleep 2
	arm-none-eabi-gdb -ex "tar ext :4242" ./build/vl_discovery/vl_discovery.elf

miniecu_v1_flash: miniecu_v1
	stm32flash -w ./build/miniecu_v1/miniecu_v1.bin $(SERIAL)

%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
