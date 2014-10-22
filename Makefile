# -*- Makefile -*-
#

TARGETS = miniecu_v2
SERIAL = /dev/ttyUSB0
CHIBIOS_REV = 7418

all: $(TARGETS)

miniecu_v2:

miniecu_v2_flash: miniecu_v2
	dfu-util -v --alt=0 -s 0x08000000 -D ./build/miniecu_v2/miniecu_v2.bin

miniecu_v2_oocd: miniecu_v2
	openocd -f ./boards/miniecu_v2/openocd.cfg

%: ./boards/% ext/chibios
	make -C ./pb all python_msgs
	make -C ./fw/param
	make -C ./boards/$@

sync:
	( cd ./ext/chibios && svn up -r $(CHIBIOS_REV) )
	git submodule update

ext/chibios:
	svn co http://svn.code.sf.net/p/chibios/svn/trunk $@ -r $(CHIBIOS_REV)

clean:
	for target in $(TARGETS); do \
		make -C ./boards/$$target clean; \
	done
