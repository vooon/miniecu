include ${MINIECU}/fw/param/param.mk
include ${MINIECU}/fw/lib/lib.mk
include ${MINIECU}/fw/hw/hw.mk

# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/alert_led.c \
	${PARAMSRC} \
	${FWLIBSRC} \
	${HWSRC} \
	${MINIECU}/fw/comm/pbstx.c \
	${MINIECU}/fw/comm/th_comm_pbstx.c \
	${MINIECU}/fw/adc/th_adc.c \
	${MINIECU}/fw/memdump.c \
	${MINIECU}/fw/command.c \
	${MINIECU}/fw/th_rpm.c \
	${MINIECU}/fw/log/th_log.c

# Required include directories
FWINC = ${MINIECU}/fw \
	${FWLIBINC} \
	${PARAMINC}
