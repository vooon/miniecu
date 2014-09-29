include ${MINIECU}/fw/param/param.mk
include ${MINIECU}/fw/hw/hw.mk

# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/alert_led.c \
	${PARAMSRC} \
	${HWSRC} \
	${MINIECU}/fw/lib/lib_crc16.c \
	${MINIECU}/fw/lib/ntc.c \
	${MINIECU}/fw/comm/pbstx.c \
	${MINIECU}/fw/comm/th_comm_pbstx.c \
	${MINIECU}/fw/adc/th_adc.c \
	${MINIECU}/fw/memdump.c \
	${MINIECU}/fw/th_rpm.c \
	${MINIECU}/fw/th_flash_log.c \
	${MINIECU}/fw/th_command.c

# Required include directories
FWINC = ${MINIECU}/fw \
	${MINIECU}/fw/adc \
	${MINIECU}/fw/comm \
	${MINIECU}/fw/lib \
	${HWINC} \
	${PARAMINC}
