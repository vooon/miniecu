include ${MINIECU}/fw/param/param.mk
include ${MINIECU}/fw/lib/lib.mk
include ${MINIECU}/fw/hw/hw.mk
include ${MINIECU}/fw/comm/comm.mk
include ${MINIECU}/fw/log/log.mk

# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/alert_led.c \
	${PARAMSRC} \
	${FWLIBSRC} \
	${HWSRC} \
	${COMMSRC} \
	${LOGSRC} \
	${MINIECU}/fw/adc/th_adc.c \
	${MINIECU}/fw/memdump.c \
	${MINIECU}/fw/command.c \
	${MINIECU}/fw/th_rpm.c \

# Required include directories
FWINC = ${MINIECU}/fw \
	${FWLIBINC} \
	${PARAMINC}
