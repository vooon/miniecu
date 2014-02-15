# -*- Makefile -*-
#

TARGETS = miniecu_v1 vl_discovery

all: $(TARGETS)

miniecu_v1:

vl_discovery:

vl_discovery_gdb: vl_discovery
	st-util -1 > ./build/st-util.log 2>&1 &
	sleep 2
	arm-none-eabi-gdb -ex "tar ext :4242" ./build/vl_discovery/vl_discovery.elf

%: ./boards/%
	make -C ./pb
	make -C ./boards/$@

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
